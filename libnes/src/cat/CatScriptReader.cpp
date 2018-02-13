#include "cat/CatScriptReader.h"
#include "util/TBufStream.h"
#include "util/TStringConversion.h"
#include "nes/UxRomBanking.h"
#include "exception/TGenericException.h"
#include <cctype>
#include <algorithm>
#include <string>
#include <iostream>

using namespace BlackT;

const static int scriptBufferCapacity = 0x10000;

namespace Nes {


CatScriptReader::CatScriptReader(
                  BlackT::TStream& src__,
                  BlackT::TStream& dst__,
                  const BlackT::TThingyTable& thingy__)
  : src(src__),
    dst(dst__),
    thingy(thingy__),
    lineNum(0),
    currentScriptBuffer(scriptBufferCapacity),
    blockRemaining(0),
    blockIsInFixedBank(false),
    currentPointerTablePos(-1),
    freeBlockPos(-1),
    freeBlockRemaining(-1),
    freeBlockIsInFixedBank(false) {
  loadThingy(thingy__);
}

void CatScriptReader::operator()() {
  while (!src.eof()) {
    std::string line;
    src.getLine(line);
    ++lineNum;
    
    if (line.size() <= 0) continue;
    
    // discard lines containing only ASCII spaces and tabs
    bool onlySpace = true;
    for (int i = 0; i < line.size(); i++) {
      if ((line[i] != ' ')
          && (line[i] != '\t')) {
        onlySpace = false;
        break;
      }
    }
    if (onlySpace) continue;
    
    TBufStream ifs(line.size());
    ifs.write(line.c_str(), line.size());
    ifs.seek(0);
    
    // check for comments
    if ((ifs.size() >= 2)
        && (ifs.peek() == '/')) {
      ifs.get();
      if (ifs.peek() == '/') continue;
      else ifs.unget();
    }
    
    // check for special stuff
    if (ifs.peek() == '#') {
      // directives
      ifs.get();
      processDirective(ifs);
      continue;
    }
    
    while (!ifs.eof()) {
      outputNextSymbol(ifs);
    }
  }
}
  
void CatScriptReader::loadThingy(const BlackT::TThingyTable& thingy__) {
  thingy = thingy__;

  // To efficiently unmap script values, we order the thingy's strings
  // by length
  thingiesBySize.clear();
  thingiesBySize.resize(thingy.entries.size());
  int pos = 0;
  for (TThingyTable::RawTable::iterator it = thingy.entries.begin();
       it != thingy.entries.end();
       ++it) {
    ThingyValueAndKey t = { it->second, it->first };
    thingiesBySize[pos++] = t;
  }
  std::sort(thingiesBySize.begin(), thingiesBySize.end());
}
  
void CatScriptReader::outputNextSymbol(TStream& ifs) {
  // literal value
  if ((ifs.remaining() >= 5)
      && (ifs.peek() == '<')) {
    int pos = ifs.tell();
    
    ifs.get();
    if (ifs.peek() == '$') {
      ifs.get();
      std::string valuestr = "0x";
      valuestr += ifs.get();
      valuestr += ifs.get();
      
      if (ifs.peek() == '>') {
        ifs.get();
        int value = TStringConversion::stringToInt(valuestr);
        
//        dst.writeu8(value);
        currentScriptBuffer.writeu8(value);

        return;
      }
    }
    
    // not a literal value
    ifs.seek(pos);
  }

  for (int i = 0; i < thingiesBySize.size(); i++) {
    if (checkSymbol(ifs, thingiesBySize[i].value)) {
      int symbolSize;
      if (thingiesBySize[i].key <= 0xFF) symbolSize = 1;
      else if (thingiesBySize[i].key <= 0xFFFF) symbolSize = 2;
      else if (thingiesBySize[i].key <= 0xFFFFFF) symbolSize = 3;
      else symbolSize = 4;
      
//      dst.writeInt(thingiesBySize[i].key, symbolSize,
//        EndiannessTypes::big, SignednessTypes::nosign);
      currentScriptBuffer.writeInt(thingiesBySize[i].key, symbolSize,
        EndiannessTypes::big, SignednessTypes::nosign);
        
      // when terminator reached, flush script to ROM stream
      if (thingiesBySize[i].key == 0) {
        flushActiveScript();
      }
      
      return;
    }
  }
  
  std::string remainder;
  ifs.getLine(remainder);
  
  // if we reached end of file, this is not an error: we're done
  if (ifs.eof()) return;
  
  throw TGenericException(T_SRCANDLINE,
                          "CatScriptReader::outputNextSymbol()",
                          "Line "
                            + TStringConversion::intToString(lineNum)
                            + ":\n  Couldn't match symbol at: '"
                            + remainder
                            + "'");
}
  
void CatScriptReader::flushActiveScript() {
  int outputSize = currentScriptBuffer.size();
  // try to put script in target block
  if (outputSize <= blockRemaining) {
    if (currentPointerTablePos != -1) {
      int curBlockPos = dst.tell();
      
      dst.seek(currentPointerTablePos);
      int address;
      if (blockIsInFixedBank) {
        address = UxRomBanking::directToBankedAddressFixed(curBlockPos);
      }
      else {
        address = UxRomBanking::directToBankedAddressMovable(curBlockPos);
      }
      dst.writeu16le(address);
      currentPointerTablePos += 2;
      dst.seek(curBlockPos);
    }
    
    // write data
    dst.write(currentScriptBuffer.data().data(), currentScriptBuffer.size());
    blockRemaining -= outputSize;
  }
  // if it won't fit, try to put it in the free block
  else if (outputSize <= freeBlockRemaining) {
    // temporarily move output position to free block
    int curBlockPos = dst.tell();
    dst.seek(freeBlockPos);
    dst.write(currentScriptBuffer.data().data(), currentScriptBuffer.size());
    // restore current position
    dst.seek(curBlockPos);
    
    if (currentPointerTablePos != -1) {
      int curBlockPos = dst.tell();
      
      dst.seek(currentPointerTablePos);
      int address;
      if (freeBlockIsInFixedBank) {
        address = UxRomBanking::directToBankedAddressFixed(freeBlockPos);
      }
      else {
        address = UxRomBanking::directToBankedAddressMovable(freeBlockPos);
      }
      dst.writeu16le(address);
      currentPointerTablePos += 2;
      dst.seek(curBlockPos);
    }
    
    freeBlockPos += outputSize;
    freeBlockRemaining -= outputSize;
  }
  else {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::flushActiveScript()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Not enough free space for script");
  }
  
  // clear script buffer
  currentScriptBuffer = TBufStream(scriptBufferCapacity);
}
  
bool CatScriptReader::checkSymbol(BlackT::TStream& ifs, std::string& symbol) {
  if (symbol.size() > ifs.remaining()) return false;
  
  int startpos = ifs.tell();
  for (int i = 0; i < symbol.size(); i++) {
    if (symbol[i] != ifs.get()) {
      ifs.seek(startpos);
      return false;
    }
  }
  
  return true;
}
  
void CatScriptReader::processDirective(BlackT::TStream& ifs) {
  skipSpace(ifs);
  
  std::string name = matchName(ifs);
  matchChar(ifs, '(');
  
  for (int i = 0; i < name.size(); i++) {
    name[i] = toupper(name[i]);
  }
  
  if (name.compare("LOADTABLE") == 0) {
    processLoadTable(ifs);
  }
  else if (name.compare("STARTBLOCK") == 0) {
    processStartBlock(ifs);
  }
  else if (name.compare("SETFREEBLOCK") == 0) {
    processSetFreeBlock(ifs);
  }
  else {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::processDirective()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unknown directive: "
                              + name);
  }
  
  matchChar(ifs, ')');
}

void CatScriptReader::processLoadTable(BlackT::TStream& ifs) {
  std::string tableName = matchString(ifs);
  TThingyTable table(tableName);
  loadThingy(table);
}

void CatScriptReader::processStartBlock(BlackT::TStream& ifs) {
//  std::string tableName = matchString(ifs);
//  TThingyTable table(tableName);
//  loadThingy(table);
  
  // flush pending script (e.g. for writes to unterminated tilemaps)
  if (currentScriptBuffer.size() > 0) flushActiveScript();

  // argument 1: address of block we want to put data in
  dst.seek(matchInt(ifs));
  matchChar(ifs, ',');
  // argument 2: size of destination block
  blockRemaining = matchInt(ifs);
  matchChar(ifs, ',');
  // argument 3: nonzero if location is in the fixed bank (i.e. pointers
  // must be C000-based instead of 8000-based)
  blockIsInFixedBank = (matchInt(ifs) != 0);
  
  // argument 4 (optional): address of table to write data pointers to
  // (if omitted, no pointers are written)
  if (checkChar(ifs, ',')) {
    matchChar(ifs, ',');
    currentPointerTablePos = matchInt(ifs);
  }
}

void CatScriptReader::processSetFreeBlock(BlackT::TStream& ifs) {
//  std::string tableName = matchString(ifs);
//  TThingyTable table(tableName);
//  loadThingy(table);

  // argument 1: address of block we want to put data in
  freeBlockPos = matchInt(ifs);
  matchChar(ifs, ',');
  // argument 2: size of destination block
  freeBlockRemaining = matchInt(ifs);
  matchChar(ifs, ',');
  // argument 3: nonzero if location is in the fixed bank (i.e. pointers
  // must be C000-based instead of 8000-based)
  freeBlockIsInFixedBank = (matchInt(ifs) != 0);
}

void CatScriptReader::skipSpace(BlackT::TStream& ifs) const {
  ifs.skipSpace();
}

bool CatScriptReader::checkString(BlackT::TStream& ifs) const {
  skipSpace(ifs);
  
  if (!ifs.eof() && (ifs.peek() == '"')) return true;
  return false;
}

bool CatScriptReader::checkInt(BlackT::TStream& ifs) const {
  skipSpace(ifs);
  
  if (!ifs.eof()
      && (isdigit(ifs.peek()) || ifs.peek() == '$')) return true;
  return false;
}

bool CatScriptReader::checkChar(BlackT::TStream& ifs, char c) const {
  skipSpace(ifs);
  
  if (!ifs.eof() && (ifs.peek() == c)) return true;
  return false;
}

std::string CatScriptReader::matchString(BlackT::TStream& ifs) const {
  skipSpace(ifs);
  if (ifs.eof()) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchString()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unexpected end of line");
  }
  
  if (!checkString(ifs)) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchString()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unexpected non-string at: "
                              + getRemainingContent(ifs));
  }
  
  ifs.get();
  
  std::string result;
  while (!ifs.eof() && (ifs.peek() != '"')) result += ifs.get();
  
  matchChar(ifs, '"');
  
  return result;
}

int CatScriptReader::matchInt(BlackT::TStream& ifs) const {
  skipSpace(ifs);
  if (ifs.eof()) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchInt()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unexpected end of line");
  }
  
  if (!checkInt(ifs)) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchInt()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unexpected non-int at: "
                              + getRemainingContent(ifs));
  }
  
  std::string numstr;
  // get first character (this covers the case of an initial '$' for
  // hexadecimal)
  numstr += ifs.get();
  // handle possible initial "0x"
  if ((numstr[0] == '0') && (tolower(ifs.peek()) == 'x')) numstr += ifs.get();
  
  char next = ifs.peek();
  while (!ifs.eof()
         && (isdigit(next)
          || (tolower(next) == 'a')
          || (tolower(next) == 'b')
          || (tolower(next) == 'c')
          || (tolower(next) == 'd')
          || (tolower(next) == 'e')
          || (tolower(next) == 'f'))) {
    numstr += ifs.get();
    next = ifs.peek();
  }
  
  return TStringConversion::stringToInt(numstr);
}

void CatScriptReader::matchChar(BlackT::TStream& ifs, char c) const {
  skipSpace(ifs);
  if (ifs.eof()) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchChar()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unexpected end of line");
  }
  
  if (ifs.peek() != c) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchChar()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Expected '"
                              + c
                              + "', got '"
                              + ifs.get()
                              + "'");
  }
  
  ifs.get();
}
  
std::string CatScriptReader
  ::getRemainingContent(BlackT::TStream& ifs) const {
  std::string content;
  while (!ifs.eof()) content += ifs.get();
  return content;
}

std::string CatScriptReader::matchName(BlackT::TStream& ifs) const {
  skipSpace(ifs);
  if (ifs.eof()) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchName()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Unexpected end of line");
  }
  
  if (!isalpha(ifs.peek())) {
    throw TGenericException(T_SRCANDLINE,
                            "CatScriptReader::matchName()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ":\n  Couldn't read name at: "
                              + getRemainingContent(ifs));
  }
  
  std::string result;
  result += ifs.get();
  while (!ifs.eof()
         && (isalnum(ifs.peek()))) {
    result += ifs.get();
  }
  
  return result;
}


}
