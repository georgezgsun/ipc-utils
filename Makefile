# the shared memory and message queue example using ipc-utils library

# this must use GNU Make ("make" on Linux and Mac OS X, "gmake" on Solaris)

# preparation: 
#	mkdir ~/projects
#	mkdir ~/projects/common
#	mkdir ~/projects/common/include
#	mkdir ~/projects/common/lib
#	export LD_LIBRARY_PATH=$HOME/projects/common/lib:$LD_LIBRARY_PATH
#
#	Author: George Sun, 2019/10

COMMON = $(HOME)/projects/common
INCLUDE_PATH = $(COMMON)/include/
LIB_PATH = $(COMMON)/lib/

# compiler options -- C++11 with warnings
OPT_GCC = "-std=c++11" -Wall -Wextra

# compiler options and libraries for Linux, Mac OS X or Solaris
OPT = "-D_XOPEN_SOURCE=700"
LIB = -lrt

DLL_SRC = ipc-utils
DLL_VER = 1
DLL_SUB = 0
DLL = lib$(DLL_SRC).so

all: dll shm

# -fPIC options enable "position independent code", which is required for shared libraries.
# -g2 -gdwarf-2 options enable debugging information
# -c enables to generate the .o object files.
# -Wall options enable the generating of warnings
# multiple .o files can be linked into one shared library
# -shared 
# -Wl,options pass options to linker. -soname,libipc-utils.so.1 indicates the library name passed with -o option
# -o output of operation
# $(LIB_PATH) speifies the common library path. Do not forget to add it into $LD_LIBRARY_PATH
# The link to $(LIB_PATH)$(DLL) allows the naming convention for compile flag -libipc-utils to work
# The link to $(LIB_PATH)$(DLL).$(DLL_VER) allows the run time binding to work
dll: $(DLL_SRC).h $(DLL_SRC).cpp
	g++ $(OPT_GCC) -fPIC -g2 -gdwarf-2 -I$(INCLUDE_PATH) -c $(DLL_SRC).cpp
	g++ -shared -Wl,-soname,$(DLL).$(DLL_VER) -o $(DLL).$(DLL_VER).$(DLL_SUB) $(DLL_SRC).o $(LIB)
	rm $(DLL_SRC).o
	cp $(DLL_SRC).h $(INCLUDE_PATH)
	mv $(DLL).$(DLL_VER).$(DLL_SUB) $(LIB_PATH)
	ln -sf $(LIB_PATH)$(DLL).$(DLL_VER).$(DLL_SUB) $(LIB_PATH)$(DLL).$(DLL_VER)
	ln -sf $(LIB_PATH)$(DLL).$(DLL_VER) $(LIB_PATH)$(DLL)

shm: shm-test.cpp
	g++ $(OPT_GCC) $(OPT) -I$(INCLUDE_PATH) -L$(LIB_PATH) shm-test.cpp -l$(DLL_SRC) -o shm
	
run: shm
	./shm

clean:
	rm -f shm
