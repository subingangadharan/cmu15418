CXX=icc -m64 -std=c++11
CXXFLAGS=-I.. -O2 -Wall

APP_NAME=convert

OBJFILES = dirs/graph.o dirs/convert.o

default: $(APP_NAME)

.PHONY: dirs clean

dirs:
		/bin/mkdir -p dirs/

clean:
		/bin/rm -rf dirs *~ $(APP_NAME)

$(APP_NAME): dirs $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(OBJFILES) -o $@ 

dirs/graph.o: ../graph.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

dirs/convert.o: convert.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@
