# Turbo Vision 1.0 for Linux (GCC, C++11)
#
#   make            library (lib/libtv.a) + demos + docdemos
#   make lib        libraries only (lib/libtv.a, lib/libtvaddons.a)
#   make check-addons  tests of the add-on classes
#   make clean
#
#   make DEBUG=1    -O0 -g
#   make ASAN=1     address/undefined sanitizers

CXX      ?= g++
AR       ?= ar
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++11 -Wall -Wno-unused -Wno-parentheses \
            -Wno-char-subscripts -Wno-write-strings -Wno-narrowing \
            -Wno-deprecated-declarations -Wno-misleading-indentation \
            -Wno-class-memaccess -Wno-reorder -Wno-sign-compare \
            -Wno-overloaded-virtual -Wno-switch -Wno-int-to-pointer-cast -fno-strict-aliasing
CPPFLAGS += -Iinclude -Iinclude/compat
# dependency files: an object is rebuilt when a header it includes changes
CPPFLAGS += -MMD -MP

ifeq ($(DEBUG),1)
CXXFLAGS += -O0 -g
endif
ifeq ($(ASAN),1)
CXXFLAGS += -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS  += -fsanitize=address,undefined
endif

LIB      := lib/libtv.a
LIBSRC   := $(wildcard source/*.cpp) $(wildcard source/platform/*.cpp)
LIBOBJ   := $(patsubst %.cpp,obj/%.o,$(LIBSRC))

# whole-archive: every streamable class registers itself from a static
# constructor, exactly as the DOS library relied on __link()
LINKLIB  := -Wl,--whole-archive $(LIB) -Wl,--no-whole-archive

TVDEMO   := demos/tvdemo1.cpp demos/tvdemo2.cpp demos/tvdemo3.cpp \
            demos/ascii.cpp demos/calendar.cpp demos/calc.cpp \
            demos/fileview.cpp demos/gadgets.cpp demos/mousedlg.cpp \
            demos/puzzle.cpp
TVEDIT   := demos/tvedit1.cpp demos/tvedit2.cpp demos/tvedit3.cpp
FORMSLIB := demos/forms.cpp demos/datacoll.cpp demos/fields.cpp \
            demos/listdlg.cpp
FORMSOBJ := $(patsubst %.cpp,obj/%.o,$(FORMSLIB))
DOCDEMOS := $(wildcard docdemos/*.cpp)

# add-on classes (addons/): a library of their own, lib/libtvaddons.a
ADDLIB   := lib/libtvaddons.a
ADDSRC   := $(wildcard addons/source/*.cpp)
ADDOBJ   := $(patsubst %.cpp,obj/%.o,$(ADDSRC))
LINKADD  := -Wl,--whole-archive $(ADDLIB) $(LIB) -Wl,--no-whole-archive

PROGRAMS := bin/tvdemo bin/tvedit bin/hello bin/keytest \
            bin/tvforms bin/genphone bin/genparts bin/tvhc bin/yamledit \
            $(patsubst docdemos/%.cpp,bin/%,$(DOCDEMOS))

all: lib $(PROGRAMS) bin/demohelp.hlp bin/phonenum.tvf bin/parts.tvf

lib: $(LIB) $(ADDLIB)

$(LIB): $(LIBOBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

obj/addons/%.o: CPPFLAGS += -Iaddons/include

$(ADDLIB): $(ADDOBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

# YAMLEDIT: TVEDIT with TYamlEditWindow
obj/addons/demos/yamledit.o: CPPFLAGS += -Idemos

bin/yamledit: obj/addons/demos/yamledit.o obj/demos/tvedit2.o obj/demos/tvedit3.o $(ADDLIB) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LINKADD)

bin/yamlsyntax-test: obj/addons/tests/yamlsyntax_test.o $(ADDLIB) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $< $(LINKADD)

check-addons: bin/yamlsyntax-test
	bin/yamlsyntax-test

obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

bin/tvdemo: $(patsubst %.cpp,obj/%.o,$(TVDEMO)) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LINKLIB)

bin/tvedit: $(patsubst %.cpp,obj/%.o,$(TVEDIT)) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LINKLIB)

bin/hello: obj/demos/hello.o $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $< $(LINKLIB)

# docdemos 06-16 display their own source file
obj/docdemos/%.o: CPPFLAGS += -DTV_DOCDEMOS_DIR='"$(CURDIR)/docdemos"'

bin/%: obj/docdemos/%.o $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $< $(LINKLIB)

# TVFORMS: the form editor and the generators of its two data files
bin/tvforms: obj/demos/tvforms.o $(FORMSOBJ) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LINKLIB)

obj/demos/genphone.o: demos/genform.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DPHONENUM -c $< -o $@

obj/demos/genparts.o: demos/genform.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DPARTS -c $< -o $@

bin/genphone: obj/demos/genphone.o $(FORMSOBJ) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LINKLIB)

bin/genparts: obj/demos/genparts.o $(FORMSOBJ) $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LINKLIB)

bin/phonenum.tvf: bin/genphone
	cd bin && ./genphone

bin/parts.tvf: bin/genparts
	cd bin && ./genparts

# TVHC: the help compiler
bin/tvhc: obj/help/tvhc.o $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $< $(LINKLIB)

# compiles help/demohelp.txt and compares the result with the DOS build
check-help: bin/tvhc
	@mkdir -p obj/help
	cp help/demohelp.txt obj/help/demohelp.txt
	rm -f obj/help/demohelp.hlp obj/help/demohelp.h
	cd obj/help && ../../bin/tvhc demohelp.txt
	cmp obj/help/demohelp.hlp demos/demohelp.hlp && echo "demohelp.hlp: identical to the DOS build"

# keytest only needs the terminal driver: plain archive linking
bin/keytest: obj/tools/keytest.o $(LIB)
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $< $(LIB)

bin/demohelp.hlp: demos/demohelp.hlp
	@mkdir -p bin
	cp $< $@

clean:
	rm -rf obj lib bin

# keep the docdemo objects (make would delete them as intermediates)
.SECONDARY:

-include $(shell find obj -name '*.d' 2>/dev/null)

.PHONY: all lib clean check-help check-addons
