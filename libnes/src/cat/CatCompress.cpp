#include "cat/CatCompress.h"
#include <algorithm>
#include <iostream>

using namespace BlackT;

namespace Nes {


void CatCompress::decmpCatRle(BlackT::TStream& src, BlackT::TStream& dst) {
  // first 2 bytes specify initial subcmd and maincmd, respectively
  TByte subcmd = (BlackT::TByte)src.get();
  TByte maincmd = (BlackT::TByte)src.get();
  // run mode is initially off
  bool runMode = initialRunFlag;
  
  while (true) {
    TByte next = src.get();
    
    if (next == maincmd) {
      // decompression terminates if there are two consecutive maincmds
      if ((TByte)src.peek() == maincmd) {
        // don't actually need to do this, but it makes reporting the
        // compressed size cleaner
        src.get();
        break;
      }
      
      // otherwise, fetch new values for subcmd and maincmd, respectively
      subcmd = src.get();
      maincmd = src.get();
    }
    else if (next == subcmd) {
      // toggle run flag
      runMode = !runMode;
    }
    else {
      // do RLE decoding if run flag set
      if (runMode) {
        TByte runLen = src.get();
        do {
          dst.put(next);
          --runLen;
        } while (runLen != 0);
      }
      // otherwise, place next absolute value
      else {
        dst.put(next);
      }
    }
    
  }
}

void CatCompress::cmpCatRle(BlackT::TStream& src, BlackT::TStream& dst) {
  // maximally compress source data
  CompressedData cmpData;
  preCompressRle(src, cmpData);
  
  // optimal cmd values: the ones placed furthest from the current position
  // (not used after this point == infinity)
//  std::cout << cmpData.size() << std::endl;
  CmdOptimalityList optimals;
  analyzeCmdOptimality(cmpData, 0, optimals);
//  for (int i = 0; i < optimals.size(); i++) {
//    std::cout << std::hex << optimals[i].key
//      << " " << std::dec << optimals[i].value << std::endl;
//  }

  TByte subcmd = optimals[0].key;
  TByte maincmd = optimals[1].key;
  
  dst.put(subcmd);
  dst.put(maincmd);
  
//  std::cout << std::hex << (int)subcmd << " " << (int)maincmd << std::endl;
  
  for (int i = 0; i < cmpData.size(); i++) {
    CompressionToken token = cmpData[i];
    
    if (token.type == cmpTokenSwitch) {
      dst.put(subcmd);
      continue;
    }
    
    // if the target value is the same as one of the current commands, we
    // have to switch to new cmd values to deal with it
    if ((token.value == maincmd) || (token.value == subcmd)) {
      // write the switch command
      dst.put(maincmd);
    
      // at any point that we are forced to insert a cmd switch, reevaluate
      // the remaining data for the new optimal values
      analyzeCmdOptimality(cmpData, i, optimals);
      
      // assign the new main and sub cmd values, ensuring neither is equal
      // to the new target value
      int pos = 0;
      
      do {
        subcmd = optimals[pos++].value;
        // the new subcmd value cannot be the same as the current maincmd value
        // (otherwise this will be detected as end of input)
      } while ((subcmd == token.value)
               || (subcmd == maincmd));
      
      do {
        maincmd = optimals[pos++].value;
      } while (maincmd == token.value);
      
      // write updated values
      dst.put(subcmd);
      dst.put(maincmd);
    }
    
    if (token.type == cmpTokenAbsolute) {
      dst.put(token.value);
    }
    else {
      dst.put(token.value);
      dst.put(token.count);
    }
    
  }
  
  // write terminator
  dst.put(maincmd);
  dst.put(maincmd);
  
}

void CatCompress::preCompressRle(BlackT::TStream& src,
                           CompressedData& dst) {
  // run mode is initially off
  bool runMode = initialRunFlag;
  
  while (!src.eof()) {
    int nextRunLen = findRunLength(src);
    
    // in run mode, next run is long enough to be guaranteed optimal
    if (runMode
        && (nextRunLen >= minRunLengthNoSwitch)) {
      doRunMode:
      // add run compression token
      CompressionToken token;
      token.type = cmpTokenRun;
      token.value = (BlackT::TByte)src.peek();
      token.count = nextRunLen;
      dst.push_back(token);
      
      src.seekoff(nextRunLen);
    }
    // not in run mode, next run is long enough to be guaranteed optimal
    // (even taking into account space needed for switch)
    else if (!runMode
             && (nextRunLen >= minRunLengthWithSwitch)) {
      // switch to run mode
      CompressionToken token;
      token.type = cmpTokenSwitch;
      dst.push_back(token);
      runMode = !runMode;
      
      // add run compression token
      token.type = cmpTokenRun;
      token.value = (BlackT::TByte)src.peek();
      token.count = nextRunLen;
      dst.push_back(token);
      
      src.seekoff(nextRunLen);
    }
    // in run mode, next run is not long enough to be guaranteed optimal
    else if (runMode) {
      // check the length of the next run after this one (if not at end of
      // input)
      if (src.tell() + nextRunLen < src.size()) {
        int oldpos = src.tell();
        src.seekoff(nextRunLen);
        int nextNextRunLen = findRunLength(src);
        
        // rewind stream
        src.clear();
        src.seek(oldpos);
        
        // if run is long enough not to justify a switch to absolute mode,
        // use run mode (with a goto solely because I don't feel like splitting
        // this out into separate functions)
        if (nextNextRunLen > maxNextRunLengthForAbsSwitch) goto doRunMode;
      }
      
      
    
      // switch to absolute mode
      CompressionToken token;
      token.type = cmpTokenSwitch;
      dst.push_back(token);
      runMode = !runMode;
      
      // add absolute compression tokens
      token.type = cmpTokenAbsolute;
      for (int i = 0; i < nextRunLen; i++) {
        token.value = (BlackT::TByte)src.get();
        dst.push_back(token);
      }
    }
    else {
      // add absolute compression tokens
      CompressionToken token;
      token.type = cmpTokenAbsolute;
      for (int i = 0; i < nextRunLen; i++) {
        token.value = (BlackT::TByte)src.get();
        dst.push_back(token);
      }
    }
  }
}
  
int CatCompress::findRunLength(BlackT::TStream& src) {
  int startpos = src.tell();
  int value = src.get();
  int length = 1;
  while (!(src.eof())
         && (length < maxRunLength)
         && (src.peek() == value)) {
    src.get();
    ++length;
  }
  
  // rewind stream to initial position
  src.clear();
  src.seek(startpos);
  
  return length;
}

void CatCompress::analyzeCmdOptimality(CompressedData& src,
                                 int srcpos,
                                 CmdOptimalityList& optimals) {
  // initialize results to infinity
  optimals.resize(cmdOptimalityListSize);
  for (int i = 0; i < optimals.size(); i++) {
    optimals[i].key = i;
    optimals[i].value = cmdOptimalityInf;
  }
  
  int pos = srcpos;
  int sz = src.size();
  while (pos < sz) {
    CompressionToken next = src[pos++];
    
    // do not analyze mode flag swap tokens
    if (next.type == cmpTokenSwitch) continue;
//    if (next == cmpFlagSwapToken) continue;
    
    // if this value has not yet been encountered, record its position
    // (distance is from end of source data, so when we sort, the best
    // results come first)
    if (optimals[next.value].value == cmdOptimalityInf) {
//      optimals[next] = pos - srcpos;
      optimals[next.value].value = (sz - pos - 1);
    }
  }
  
  // sort results
  std::sort(optimals.begin(), optimals.end());
  
  // reset stream to initial position
//  src.seek(startpos);
}


}
