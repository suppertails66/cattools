#ifndef CATCOMPRESS_H
#define CATCOMPRESS_H


#include "util/TStream.h"
#include "util/TByte.h"
#include <vector>

namespace Nes {


class CatCompress {
public:
  static void decmpCatRle(BlackT::TStream& src, BlackT::TStream& dst);
  static void cmpCatRle(BlackT::TStream& src, BlackT::TStream& dst);
protected:
  enum CompressionTokenType {
    cmpTokenRun,
    cmpTokenAbsolute,
    cmpTokenSwitch
  };
  
  struct CompressionToken {
    CompressionTokenType type;
    int value;
    int count;
  };
  
  struct OptimalityEntry {
    int key;
    int value;
    
    bool operator<(const OptimalityEntry& other) const {
      return (value < other.value);
    }
  };
  
  typedef std::vector<OptimalityEntry> CmdOptimalityList;
  typedef std::vector<CompressionToken> CompressedData;
  
  const static bool initialRunFlag = false;
  
  const static int cmdOptimalityListSize = 256;
  const static int cmdOptimalityInf = -1;
  
  // token used internally by compressor to mark mode swap locations before
  // the maincmd and subcmd have been determined
  const static int cmpFlagSwapToken = -1;
  
  // minimum run length required to guarantee that it is optimal to switch
  // from absolute to run mode
  const static int minRunLengthWithSwitch = 4;
  // minimum run length required to guarantee that it is optimal to encode
  // the run rather than switching to absolute mode
  const static int minRunLengthNoSwitch = 3;
  // maximum second run length that can trigger a switch from run to
  // absolute mode
  const static int maxNextRunLengthForAbsSwitch = 2;
  // maximum length of a run
  const static int maxRunLength = 256;
  
  static void preCompressRle(BlackT::TStream& src,
                             CompressedData& dst);
  
  static int findRunLength(BlackT::TStream& src);
  
  static void analyzeCmdOptimality(CompressedData& src,
                                   int srcpos,
                                   CmdOptimalityList& optimals);
  
};


}


#endif
