#include <cstring> 
#include <iostream> 
#include <stdio.h> 
#include <string> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netdb.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <unordered_map> 
#include <sys/time.h> 
#include <cmath> 
#include <signal.h> 
#include <fstream> 
#include <errno.h>

using namespace std;

string PORTNUM;
string FILENAME;
struct timeval START_TIME, CURR_TIME;

// Global Constants
const uint16_t RTO = 500; // 500ms
const uint16_t HEADER_SIZE = 8; // 8 bytes
const uint16_t MSS = 1024;
const uint16_t MAX_PACKET_LEN = 1032; // max 1024 bytes of payload
const uint16_t MSN = 30720; // 30 KB
const uint16_t SSTHRESH = 15360; // initial slow start threshold (bytes)
const uint16_t MAX_RECVWIN = 15360; // TODO 

int CWND = 1024;


int main(int argc, char* argv[])
{
  if (argc != 3) {
  	cerr << "Invalid # of arguments" << endl;
  	exit(1);
  }

  PORTNUM = argv[1];
  FILENAME = argv[2];

  struct addrinfo hints;
  struct addrinfo *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, PORTNUM.c_str(), &hints, &res) != 0) {
  	cerr << "getAddrInfo error!" << endl;
  	exit(-1);
  }

  int sockfd = socket(res->ai_family, res->ai_socktype, 0);
  int yes = 1; 
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
  	cerr << "Error setting sockopts!" << endl;
  	exit(-1);
  }

  if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
  	cerr << "Error binding socket!" << endl;
  	exit(-1);
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&RTO, sizeof(timeval)) < 0) {
		cerr << "Error setting sockopts!" << endl;
		exit(-1);
	}

  string getcontent;
  ifstream openfile (FILENAME);
  if(openfile.is_open())
  {
      while(! openfile.eof())
      {
          getline(openfile, getcontent);
          cout << getcontent << endl;
      }
  }

  return 0;
}
