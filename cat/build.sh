# abort script on error
set -e

# temporarily add directories to path
PATH=".:./asm/bin/:$PATH"

# Note: input file must be unheadered!
INPUTROM=cat_noheader.nes
WORKROM=build/rom/cat_build.nes
OUTPUTROM=cat_build.nes

echo "-------------------------------------------------------------------------"
echo "Building tools..."
echo "-------------------------------------------------------------------------"
make blackt && make libnes && make

echo "-------------------------------------------------------------------------"
echo "Copying ROM..."
echo "-------------------------------------------------------------------------"
mkdir -p build/rom
cp "$INPUTROM" "$WORKROM"

echo "-------------------------------------------------------------------------"
echo "Converting graphics to binary format..."
echo "-------------------------------------------------------------------------"
mkdir -p build/grp/raw
nes_tileundmp grp/trans/0x33F9.png 0x40 build/grp/raw/0x33F9.bin
nes_tileundmp grp/trans/0x3697.png 0x93 build/grp/raw/0x3697.bin
nes_tileundmp grp/trans/0x8018.png 0x70 build/grp/raw/0x8018.bin

echo "-------------------------------------------------------------------------"
echo "Generating title logo..."
echo "-------------------------------------------------------------------------"
mkdir -p build/title
cp grp/title/title-tilemap.bin build/title/title-tilemap.bin
cat_titleprep grp/title/title-logo.png build/title/title-tilemap.bin build/grp/raw/0x33F9.bin build/grp/raw/0x3697.bin grp/title/title-logo-pal.bin
cp build/title/title-tilemap.bin build/grp/raw/0x2BD8.bin

echo "-------------------------------------------------------------------------"
echo "Compressing binary graphics..."
echo "-------------------------------------------------------------------------"
mkdir -p build/grp/cmp
for file in build/grp/raw/*; do
  echo "Compressing" $file
  cat_cmp $file "build/grp/cmp/$(basename $file)"
done

echo "-------------------------------------------------------------------------"
echo "Patching compressed graphics to ROM..."
echo "-------------------------------------------------------------------------"
#filepatch "$WORKROM" 0x33F9 build/grp/cmp/0x33F9.bin "$WORKROM" -l 670
filepatch "$WORKROM" 0x17C80 build/grp/cmp/0x33F9.bin "$WORKROM" -l 897
filepatch "$WORKROM" 0x3697 build/grp/cmp/0x3697.bin "$WORKROM" -l 1808
filepatch "$WORKROM" 0x8018 build/grp/cmp/0x8018.bin "$WORKROM" -l 1587
filepatch "$WORKROM" 0x2BD8 build/grp/cmp/0x2BD8.bin "$WORKROM" -l 518

echo "-------------------------------------------------------------------------"
echo "Building hacks..."
echo "-------------------------------------------------------------------------"
mkdir -p build/asm
ca65 asm/box_size_adjust.asm -o build/asm/box_size_adjust.out
ld65 -C asm/box_size_adjust.cfg build/asm/box_size_adjust.out
#ld65 -C asm/box_size_adjust.cfg -o build/asm/box_size_adjust build/asm/box_size_adjust.out
#./ld65 -C ltim_config.txt -o $1.nes $1.o

echo "-------------------------------------------------------------------------"
echo "Patching hacks to ROM..."
echo "-------------------------------------------------------------------------"
filepatch "$WORKROM" 0x1D6AD build/asm/box_size_adjust_1D6AD.bin "$WORKROM"
filepatch "$WORKROM" 0x1FE98 build/asm/box_size_adjust_1FE98.bin "$WORKROM"
filepatch "$WORKROM" 0x1D56E build/asm/box_size_adjust_1D56E.bin "$WORKROM"
#filepatch "$WORKROM" 0x1A432 build/asm/box_size_adjust_1A432.bin "$WORKROM"
filepatch "$WORKROM" 0x1C47B build/asm/box_size_adjust_1C47B.bin "$WORKROM"
filepatch "$WORKROM" 0x17050 build/asm/box_size_adjust_17050.bin "$WORKROM"
filepatch "$WORKROM" 0x17071 build/asm/box_size_adjust_17071.bin "$WORKROM"
filepatch "$WORKROM" 0x33E2 build/asm/box_size_adjust_33E2.bin "$WORKROM"
filepatch "$WORKROM" 0x29C5 build/asm/box_size_adjust_29C5.bin "$WORKROM"
filepatch "$WORKROM" 0x3F96 build/asm/box_size_adjust_3F96.bin "$WORKROM"

echo "-------------------------------------------------------------------------"
echo "Inserting script..."
echo "-------------------------------------------------------------------------"
#===============================================================================
# Note: cat_scriptpatch also adds the ROM header, so no patching should take
# place after this point
#===============================================================================
cat_scriptpatch "$WORKROM" cat_script_en.txt table/cat_table.tbl "$WORKROM"

echo "-------------------------------------------------------------------------"
echo "Finishing up..."
echo "-------------------------------------------------------------------------"
cp "$WORKROM" "$OUTPUTROM"

echo "-------------------------------------------------------------------------"
echo "Success!"
echo "-------------------------------------------------------------------------"
echo "Output file: $OUTPUTROM"

