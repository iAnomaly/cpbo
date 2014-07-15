DEFINES ?= _NIX
INCLUDE_PATH ?= /usr/local/boost_1_55_0
LIBRARY_PATH ?= $(INCLUDE_PATH)/stage/lib
WORKING_PATH ?= ./out

CPPFLAGS=-I"${INCLUDE_PATH}"
CXXFLAGS=
LDFLAGS=-L"${LIBRARY_PATH}"
#CXXFLAGS=-std=c++11 -stdlib=libc++
#LDFLAGS=-stdlib=libc++ -L"${LIBRARY_PATH}"

cpbo: main.o pbo.o sha1.o
	${CXX} ${LDFLAGS} \
	-o cpbo main.o pbo.o sha1.o \
	-lboost_filesystem \
	-lboost_system

%.o: %.cpp
	${CXX} ${CXXFLAGS} ${CPPFLAGS} -c -D $(DEFINES) -I"$(INCLUDE_PATH)/" $<

clean:
	rm -rf *.o
