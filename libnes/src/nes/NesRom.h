#ifndef NESROM_H
#define NESROM_H


#include "util/TByte.h"
#include "util/TString.h"
#include "util/TArray.h"

namespace Nes {


class NesRom {
public:
  enum MapperType {
    mapperUxRom = 0x02,
    mapperMmc3  = 0x04
  };
  
  enum NametableArrangementFlag {
    nametablesVertical   = 0x00,
    nametablesHorizontal = 0x01
  };

  NesRom(const BlackT::TString& filename);
  
  void exportToFile(const BlackT::TString& filename,
                    int numPrgBanks,
                    int numChrBanks,
                    NametableArrangementFlag nametablesFlag,
                    MapperType mapperNum,
                    bool hasBatteryRam = false);
  
  const BlackT::TByte* directRead(int address) const;
  BlackT::TByte* directWrite(int address);
  
  MapperType mapperType() const;
  void setMapperType(MapperType mapperType__);
  
  int size() const;
  
  void changeSize(int newSize);
protected:
  const static int minFileSize_ = 16;
  
  const static int inesHeaderOffset_ = 0;
  const static int inesHeaderSize_ = 16;
  const static int inesIdentifierSize_ = 4;
  const static BlackT::TByte inesIdentifier[];
  
  MapperType mapperType_;

  void readFromFile(const BlackT::TString& filename);

  BlackT::TArray<BlackT::TByte> rom_;
};


};


#endif
