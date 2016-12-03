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

#include "header.h"

using namespace std;

string PORTNUM;
string FILENAME;
struct timeval START_TIME, CURR_TIME;
int MODE = 0;

int CWND = 1024;


void send_packet(const int mode, const int sockfd, const struct sockaddr_in &cli_addr, const socklen_t &cli_len) {

}

int main(int argc, char* argv[])
{
  if (argc != 3) {
  	cerr << "Invalid # of arguments!" << endl;
  	exit(1);
  }

  PORTNUM = argv[1];
  FILENAME = argv[2];

  struct sockaddr_in serv_addr, cli_addr;
	socklen_t cli_len = sizeof(cli_addr);

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(PORTNUM.c_str()));
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  int sockfd; 
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
  	cerr << "Error opening socket!" << endl;
  	exit(-1);
  }

  int yes = 1; 
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
  	cerr << "Error setting sockopts (SO_REUSEADDR)!" << endl;
  	exit(-1);
  }

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
  	cerr << "Error binding socket!" << endl;
  	exit(-1);
  }

  struct timeval RTO;
  RTO.tv_sec = 0;
  RTO.tv_usec = 500000; // 500ms

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &RTO, sizeof(timeval)) < 0) {
		cerr << "Error setting sockopts (SO_RCVTIMEO)!" << endl;
		exit(-1);
	}


  string getcontent;
  ifstream openfile (FILENAME);

  // gettimeofday(&START_TIME, NULL);
  while(1) {
  	vector<char> buf(HEADER_SIZE);
  	int recvlen;
  	uint16_t ACK_NUM = 0, BASE_NUM = 0;

  	// gettimeofday(&current, NULL); // TODO does sockopt handle this?

  	// RECEIVE SYN FROM CLIENT
  	if ((recvlen = recvfrom(sockfd, &buf[0], HEADER_SIZE, 0, (struct sockaddr*) &cli_addr, &cli_len)) < 0) {
  		Header syn_h; 
  		cout << "Receiving packet " << syn_h.getAckNum() << endl;
  		ACK_NUM = (syn_h.getSeqNum() + 1) % MSN;

  		syn_h.decode(buf); 

  		if (syn_h.findSyn()) {
  			Header syn_ack = Header()
  			send_packet(1, sockfd, cli_addr, cli_len);

  		}
  	}






  }

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
