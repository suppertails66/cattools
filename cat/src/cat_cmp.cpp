#include "cat/CatCompress.h"
#include "nes/NesRom.h"
#include "util/TOpt.h"
#include "util/TStringConversion.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TFileManip.h"
#include <iostream>

using namespace std;
using namespace BlackT;
using namespace Nes;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "City Adventure Touch: Mystery of Triangle graphics compressor"
      << endl;
    cout << "Usage: " << argv[0] << " <infile> <outfile>" << endl;
    
    return 0;
  }
  
  char* infile = argv[1];
//  int offset = TStringConversion::stringToInt(string(argv[2]));
  char* outfile = argv[2];
  
  
//  TIfstream ifs(infile, ios_base::binary);
//  ifs.seek(offset);
  TBufStream ifs(1);
  ifs.open(infile);
  ifs.seek(0);
  TBufStream ofs(0x10000);
//  TOfstream ofs(outfile, ios_base::binary);
  
  CatCompress::cmpCatRle(ifs, ofs);
  
/*  TBufStream ofs2(0x10000);
  ofs.seek(0);
  CatCompress::cmpCatRle(ofs, ofs2);
  cout << "test: " << ofs2.size() << " bytes" << endl;
  
  TBufStream ofs3(0x10000);
  ofs2.seek(0);
  CatCompress::decmpCatRle(ofs2, ofs3);
  cout << "test2: " << ofs3.size() << " bytes" << endl; */
  cout << "Compressed size: " << ofs.tell() << " bytes" << endl;
//  cout << "Decompressed size: " << ofs.size() << " bytes" << endl;
  
  ofs.save(outfile);
  
  return 0;
}
