IDIR := src
SRCDIR := src
ODIR := obj
LDIR :=

BLACKTDIR = ../blackt
LIBNESDIR = ../libnes

AR=ar
CXX=g++
# Compile only
CDEFINES = -DBLACKT_ENABLE_LIBPNG
#CLIBS = -lpng
CFLAGS = -std=gnu++11 -O2 -Wall -L${LIBNESDIR} -lnes -L${BLACKTDIR} -lblackt -lpng
CINCLUDES = -I${BLACKTDIR}/src -I${LIBNESDIR}/src
CXXFLAGS=$(CFLAGS) $(CDEFINES) $(CINCLUDES) -I$(IDIR)

#SRC := $(wildcard $(SRCDIR)/*/*.cpp)
#OBJ := $(patsubst $(SRCDIR)/%,$(ODIR)/%,$(patsubst %.cpp,%.o,$(SRC)))
#DEP := $(patsubst %.o,%.d,$(OBJ))
#LIB := libnes.a

LIBDEPS := $(LIBNESDIR)/libnes.a $(BLACKTDIR)/libblackt.a

TOOLSRCS := $(wildcard $(SRCDIR)/*.cpp)
#TOOLS = test opbextr opbinsr opbfileextr opbfileinsr dsimgconv
TOOLSINDIR := $(patsubst %.cpp,%,$(TOOLSRCS))
TOOLS := $(notdir $(TOOLSINDIR))

all: blackt libnes $(TOOLS)

blackt:
	cd ${BLACKTDIR} && $(MAKE) && cd $(CURDIR)

libnes:
	cd ${LIBNESDIR} && $(MAKE) && cd $(CURDIR)
	
#$(SRCDIR)/%: blackt libnes
#	$(CXX) $@.cpp $(OBJ) -o $(notdir $@) $(CXXFLAGS)

relsearch: $(SRCDIR)/relsearch.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

nes_tiledmp: $(SRCDIR)/nes_tiledmp.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

nes_tileundmp: $(SRCDIR)/nes_tileundmp.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

tblconv: $(SRCDIR)/tblconv.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

cat_decmp: $(SRCDIR)/cat_decmp.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

cat_cmp: $(SRCDIR)/cat_cmp.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

cat_scriptpatch: $(SRCDIR)/cat_scriptpatch.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

filepatch: $(SRCDIR)/filepatch.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

cat_titleprep: $(SRCDIR)/cat_titleprep.cpp $(LIBDEPS)
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $@ $(CXXFLAGS)

.PHONY: blackt libnes cleanme clean

cleanme:
	rm -f $(TOOLS)

clean: cleanme
#	rm -f $(LIB)
#	rm -rf $(ODIR)
	cd ${BLACKTDIR} && $(MAKE) clean && cd $(CURDIR)
	cd ${LIBNESDIR} && $(MAKE) clean && cd $(CURDIR)