CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=104278461
CLASSES=

all: web-server-asyn web-server-syn  web-client-1.0 web-client-1.1

web-server-asyn: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

web-server-syn: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp
	
web-client-1.0: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

web-clien-1.1: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server-asyn web-server-syn web-client-1.0 web-client-1.1 *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
