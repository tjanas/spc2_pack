CXX = g++
CXXFLAGS = -O3
objects = spc2_pack.o spc2_write.o spc_load.o sha1.o

all: spc2_pack

spc2_pack: ${objects}
	$(CXX) $(CXXFLAGS) $(objects) -o spc2_pack

spc2_pack.o: spc2_pack.c
	$(CXX) $(CXXFLAGS) -c spc2_pack.c

spc2_write.o: spc2_write.c spc2_write.h
	$(CXX) $(CXXFLAGS) -c spc2_write.c

spc_load.o: spc_load.c spc_load.h
	$(CXX) $(CXXFLAGS) -c spc_load.c

sha1.o: sha1.c sha1.h
	$(CXX) $(CXXFLAGS) -c  sha1.c

clean:
	rm *.o spc2_pack
