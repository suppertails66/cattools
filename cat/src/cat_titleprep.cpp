#include "nes/NesPattern.h"
#include "nes/NesPalette.h"
#include "util/TStringConversion.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "exception/TGenericException.h"
#include <vector>
#include <list>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace BlackT;
using namespace Nes;

const static int grp1LoadPos = 0x00;
const static int grp1NumTiles = 0x40;
const static int grp1FreeTilesStart = 0x04;
const static int grp1NumFreeTiles = 0x3C;
const static int grp2LoadPos = 0x5B;
const static int grp2NumTiles = 0x93;
const static int grp2FreeTilesStart = 0x00;
const static int grp2NumFreeTiles = 0x06;

const static int ppuLineSize = 0x20;
const static int logoStartingOffsetInTilemap = 0x21;
const static int logoTileW = 22;
const static int logoTileH = 7;
const static int logoOriginalSize = ppuLineSize * logoTileH;

struct GraphicRegion {
  char* filename;
  int loadPos;
  int freeTilesStart;
  int freeTilesEnd;
  vector<NesPattern> patterns;
  list<int> remainingTileIndices;
};

void addGraphicRegion(vector<GraphicRegion>& regions,
                      char* filename,
                      int loadPos, int numTiles,
                      int freeTilesStart, int numFreeTiles) {
  GraphicRegion region;
  
  region.filename = filename;
  region.loadPos = loadPos;
  region.freeTilesStart = freeTilesStart;
  region.freeTilesEnd = freeTilesStart + numFreeTiles;
  
  TIfstream ifs(filename, ios_base::binary);
  for (int i = 0; i < numTiles; i++) {
    region.patterns.push_back(NesPattern());
    NesPattern& pattern = region.patterns.back();
    pattern.read(ifs);
  }
  
  for (int i = 0; i < numFreeTiles; i++) {
    region.remainingTileIndices.push_back(freeTilesStart + i);
  }
  
  regions.push_back(region);
}

int main(int argc, char* argv[]) {
  if (argc < 6) {
    cout << "City Adventure Touch: Mystery of Triangle title screen prep tool"
      << endl;
    cout << "Usage: " << argv[0] << " <logogrp> <tilemap> <grp-1> <grp-2>"
      << " <palette>"
      << endl;
    cout << "Note that tilemap, grp-1, and grp-2 will be overwritten." << endl;
    
    return 0;
  }
  
  // load source graphic
  TGraphic grp;
  TPngConversion::RGBAPngToGraphic(string(argv[1]), grp);
  
  // load tilemap
  TBufStream tilemap(1);
  tilemap.open(argv[2]);
  
  // load tiles and create lists of unassigned tile indices
  vector<GraphicRegion> regions;
  addGraphicRegion(regions, argv[3],
                   grp1LoadPos, grp1NumTiles,
                   grp1FreeTilesStart, grp1NumFreeTiles);
  addGraphicRegion(regions, argv[4],
                   grp2LoadPos, grp2NumTiles,
                   grp2FreeTilesStart, grp2NumFreeTiles);
  
  // load palette
  NesPalette palette;
  {
    TIfstream ifs(argv[5], ios_base::binary);
    palette.read(ifs);
  }
  
  for (int j = 0; j < logoTileH; j++) {
    for (int i = 0; i < logoTileW; i++) {
      int tilemapPos = logoStartingOffsetInTilemap
                        + (j * ppuLineSize)
                        + i;
      tilemap.seek(tilemapPos);
      int x = (i * NesPattern::width);
      int y = (j * NesPattern::height);
      
      NesPattern pattern;
      pattern.fromPalettizedGraphic(grp, palette, x, y);
      
/*      for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
          cout << setw(2) << " " << (int)pattern.data(i, j);
        }
        cout << endl;
      }
      char c; cin >> c; */
      
      // Find if this pattern matches any existing pattern
      bool foundExistingIndex = false;
      for (int k = 0; k < regions.size(); k++) {
        GraphicRegion& region = regions[k];
        
        for (int l = 0; l < region.patterns.size(); l++) {
          // do not assign from the free space
          if ((l >= region.freeTilesStart)
              && (l < region.freeTilesEnd)) continue;
          
          NesPattern& checkPattern = region.patterns[l];
          if (pattern == checkPattern) {
            tilemap.writeu8le(region.loadPos + l);
            foundExistingIndex = true;
            break;
          }
        }
        
        if (foundExistingIndex) break;
      }
      
      if (foundExistingIndex) continue;
      
      // New pattern: find a free tile
      bool foundNewIndex = false;
      for (int k = 0; k < regions.size(); k++) {
        GraphicRegion& region = regions[k];
        if (region.remainingTileIndices.size() <= 0) continue;
        
        int newIndex = region.remainingTileIndices.front();
        region.remainingTileIndices.pop_front();
        ++(region.freeTilesStart);
        
        region.patterns[newIndex] = pattern;
        tilemap.writeu8le(region.loadPos + newIndex);
        
        foundNewIndex = true;
        break;
      }
      
      // Couldn't create new pattern
      if (!foundNewIndex) {
        throw TGenericException(T_SRCANDLINE,
                                "main()",
                                "Ran out of space for patterns at ("
                                  + TStringConversion::intToString(i,
                                      TStringConversion::baseHex)
                                  + ", "
                                  + TStringConversion::intToString(j,
                                      TStringConversion::baseHex)
                                  + ")");
      }
    }
  }
  
  // save updated tilemap and graphics
  tilemap.save(argv[2]);
  for (int i = 0; i < regions.size(); i++) {
    GraphicRegion& region = regions[i];
    TOfstream ofs(region.filename,
                  ios_base::binary | ios_base::trunc);
    for (int j = 0; j < region.patterns.size(); j++) {
      region.patterns[j].write(ofs);
    }
  }
  
  return 0;
}
