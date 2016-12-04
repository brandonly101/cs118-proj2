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

#define NOT_CONNECTED 0
#define CONNECTION_OPEN 1
#define CONNECTION 2
#define CONNECTION_CLOSE 3

string PORTNUM;
string FILENAME;
struct timeval START_TIME, CURR_TIME;

int STAGE = NOT_CONNECTED;

uint16_t INITIAL_CWND = MSS;
uint16_t INITIAL_SSTHRESH = 15360; // initial slow start threshold (bytes)
int LAST_BYTE_SENT = -1;
int LAST_BYTE_ACKED = -1;

// void send_packet(const int mode, const int sockfd, const struct sockaddr_in &cli_addr, const socklen_t &cli_len) {

// }

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
  RTO.tv_sec = 2;
  RTO.tv_usec = 500000; // 500ms

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &RTO, sizeof(timeval)) < 0) {
		cerr << "Error setting sockopts (SO_RCVTIMEO)!" << endl;
		exit(-1);
	}


  string getcontent;
  ifstream openfile (FILENAME);

  vector<char> buf(HEADER_SIZE);
  int bytes_recv;
  srand(time(NULL)); // get random sequence number to start with
  // uint16_t SEQ_NUM = rand() % MSN;
  uint16_t SEQ_NUM = 0;
  uint16_t ACK_NUM = 0, BASE_NUM = 0;
  uint16_t SSTHRESH = INITIAL_SSTHRESH; 
  uint16_t CWND = INITIAL_CWND;

  // gettimeofday(&START_TIME, NULL);
  while(1) {


  	// gettimeofday(&current, NULL); // TODO does sockopt handle this?


  	// RECEIVE SYN FROM CLIENT
    // loops until we receive something from client to start 3 way handshake
  	if ((bytes_recv = recvfrom(sockfd, &buf[0], HEADER_SIZE, 0, (struct sockaddr*) &cli_addr, &cli_len)) != -1) {
  		Header p; 
  		cout << "Receiving packet " << p.getAckNum() << endl;
  		ACK_NUM = (p.getSeqNum() + 1) % MSN;

  		p.decode(buf); 

      // HANDLE SYN 
  		if (p.isSyn()) {

        // SEND SYN ACK TO CLIENT
        Header syn_ack = Header(SEQ_NUM, ACK_NUM, 0, 1, 1, 0);
        if (sendto(sockfd, (void *) &syn_ack, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, cli_len) < 0) {
          cerr << "Error sending SYN ACK from server to client" << endl;
          exit(-1);
        }

        cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << " SYN" << endl;
        STAGE = CONNECTION_OPEN;
        LAST_BYTE_SENT = SEQ_NUM; 
        SEQ_NUM = (SEQ_NUM + 1) % MSN; 



  			
        // Header syn_ack = Header()

  		} else if (p.isAck()) {
        STAGE = CONNECTION; 

      }


  	} else {
      // Timeout
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (STAGE == NOT_CONNECTED) {
          continue; 
        } else {
          // Retransmit


        }
        

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
