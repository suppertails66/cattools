#include "cat/CatScriptReader.h"
#include "nes/NesRom.h"
#include "util/TStringConversion.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TThingyTable.h"
#include <iostream>

using namespace std;
using namespace BlackT;
using namespace Nes;

int main(int argc, char* argv[]) {
  if (argc < 5) {
    cout << "City Adventure Touch: Mystery of Triangle script inserter"
      << endl;
    cout << "Usage: " << argv[0] << " <inrom> <script> <thingy> <outrom>"
      << endl;
    
    return 0;
  }
  
  NesRom rom = NesRom(string(argv[1]));
  TBufStream ofs(rom.size());
  ofs.write((char*)rom.directRead(0), rom.size());
  ofs.seek(0);
  
  TIfstream ifs(argv[2], ios_base::binary);
  TThingyTable thingy = TThingyTable(string(argv[3]));
  
  CatScriptReader(ifs, ofs, thingy)();
  
  // write modified data back to NesRom object
  ofs.seek(0);
  ofs.read((char*)rom.directWrite(0), ofs.size());
  
  // write modified ROM to file
  rom.exportToFile(string(argv[4]),
                   8,
                   0,
                   NesRom::nametablesHorizontal,
                   NesRom::mapperUxRom);
  
  return 0;
}
