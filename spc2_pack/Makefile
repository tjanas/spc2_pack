CXX = g++
CXXFLAGS = -O3 -Wall -Wextra -Wpedantic
objects = spc2_pack.o spc2_write.o spc_load.o sha1.o

all: spc2_pack

spc2_pack: ${objects}
	$(CXX) $(CXXFLAGS) $(objects) -o spc2_pack

spc2_pack.o: spc2_pack.cpp
	$(CXX) $(CXXFLAGS) -c spc2_pack.cpp

spc2_write.o: spc2_write.cpp spc2_write.hpp
	$(CXX) $(CXXFLAGS) -c spc2_write.cpp

spc_load.o: spc_load.cpp spc_load.hpp
	$(CXX) $(CXXFLAGS) -c spc_load.cpp

sha1.o: sha1.c sha1.h
	$(CXX) $(CXXFLAGS) -c sha1.c

clean:
	rm -fv *.o spc2_pack
