CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=EDIT_MAKE_FILE

# Add all .cpp files that need to be compiled for your server
SERVER_FILES=simple-tcp-server.cpp

# Add all .cpp files that need to be compiled for your client
CLIENT_FILES=simple-tcp-client.cpp

all: simple-tcp-server simple-tcp-client

*.o: *.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

server: $(SERVER_FILES:.cpp=.o)
	$(CXX) -o $@ $(CXXFLAGS) $(SERVER_FILES:.cpp=.o)

client: $(CLIENT_FILES:.cpp=.o)
	$(CXX) -o $@ $(CXXFLAGS) $(CLIENT_FILES:.cpp=.o)

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM simple-tcp-server simple-tcp-client received.data *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
