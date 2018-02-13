
function dmpGrp() {
  mkdir -p grp/raw
  mkdir -p grp/rip
  echo "---Extracting at: $2---"
  ./cat_decmp "$1" "$2" "grp/raw/$2.bin"
  ./nes_tiledmp "grp/raw/$2.bin" 0 "grp/rip/$2.png"
}

# title screen
dmpGrp cat.nes 0x33F9
dmpGrp cat.nes 0x3697

# main font
dmpGrp cat.nes 0x8018
dmpGrp cat.nes 0x8759
dmpGrp cat.nes 0x8E56
# has a sign which may or may not contain actual characters
dmpGrp cat.nes 0x952C
dmpGrp cat.nes 0x9A7B
dmpGrp cat.nes 0xA154
dmpGrp cat.nes 0xA7C2
#dmpGrp cat.nes 0xB800
dmpGrp cat.nes 0x864B
#dmpGrp cat.nes 0xA14E
dmpGrp cat.nes 0x8735
dmpGrp cat.nes 0xAF54

dmpGrp cat.nes 0xC010
dmpGrp cat.nes 0xC723
dmpGrp cat.nes 0xCD9C
dmpGrp cat.nes 0xD486
dmpGrp cat.nes 0xDB93
dmpGrp cat.nes 0xE213
dmpGrp cat.nes 0xC723

dmpGrp cat.nes 0x10014
dmpGrp cat.nes 0x106E7
dmpGrp cat.nes 0x10DA0
dmpGrp cat.nes 0x1143F
dmpGrp cat.nes 0x11BD3
dmpGrp cat.nes 0x122D4
dmpGrp cat.nes 0x12CF5
dmpGrp cat.nes 0x135D4
#dmpGrp cat.nes 0x12917
dmpGrp cat.nes 0x13CB7
