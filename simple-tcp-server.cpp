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
  ifstream OPENFILE (FILENAME);

  vector<char> buf(HEADER_SIZE); // TODO check if this gets flushed

  int bytes_recv;
  srand(time(NULL)); // get random sequence number to start with
  uint16_t SEQ_NUM = rand() % MSN;
  uint16_t ACK_NUM = 0, BASE_NUM = 0;
  uint16_t SSTHRESH = INITIAL_SSTHRESH; 
  uint16_t CWND = INITIAL_CWND;

  uint16_t OPENFILE_INDEX = 0;

  // gettimeofday(&START_TIME, NULL);
  while(1) {
  	// gettimeofday(&current, NULL); // TODO does sockopt handle this?


    // handle data from client
  	if ((bytes_recv = recvfrom(sockfd, &buf[0], HEADER_SIZE, 0, (struct sockaddr*) &cli_addr, &cli_len)) != -1) {
  		Header decoded_packet; 
  		cout << "Receiving packet " << decoded_packet.getAckNum() << endl;
  		ACK_NUM = (decoded_packet.getSeqNum() + 1) % MSN;

  		decoded_packet.decode(buf); 

      // HANDLE SYN 
  		if (decoded_packet.isSyn()) {

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

      // HANDLE ACK
  		} else if (decoded_packet.isAck()) {
        STAGE = CONNECTION; 
        cout << "Receiving packet " << decoded_packet.getAckNum() << endl;

        // send file

        if (!OPENFILE.is_open()) {
          cerr << "Error, file is not open!" << endl;
          exit(-1);
        }

        // TODO calculate new CWND = min((double) MSS, MSN / 2.0);
        uint16_t CWND_USED = 0;
        uint16_t pos = 0;
        int bytes_read;


        while (CWND - CWND_USED >= MSS && !OPENFILE.eof()) {
          Header data = Header(SEQ_NUM, ACK_NUM, 0, 1, 0, 0); // ACK flag set

          vector<char> encoded_packet = data.encode(); 
          encoded_packet.resize(MSS);
          
          OPENFILE.read(&encoded_packet[OPENFILE_INDEX], 1024); 

          bytes_read = OPENFILE.gcount();

          OPENFILE_INDEX += bytes_read; // TODO need last data received?
          CWND_USED += bytes_read; 

          if (sendto(sockfd, &encoded_packet[0], encoded_packet.size(), 0, (struct sockaddr*) &cli_addr, cli_len) < 0) {
            cerr << "Error sending packet" << endl;
            exit(-1);
          } 
          cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << endl; 

          LAST_BYTE_SENT = SEQ_NUM; 
          SEQ_NUM = (SEQ_NUM + bytes_read) % MSN; 
        }

      }


  	} else {
      // Timeout
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // handle the case if we haven't started our 3 way handshake
        if (STAGE == NOT_CONNECTED) {
          string data; 
          data.resize(100); 
          OPENFILE.seekg(10); 
          OPENFILE.read(&data[0], 100);
          cout << data << endl;
          // while (!OPENFILE.eof()) {
          //   Header data = Header(SEQ_NUM, ACK_NUM, 0, 1, 0, 0); // ACK flag set

          //   vector<char> encoded_packet = data.encode(); 
          //   vector<char> encoded_packet2 = data.encode(); 

          //   encoded_packet.resize(108);
            
          //   OPENFILE.read(&encoded_packet[HEADER_SIZE], 100); 
          //   OPENFILE_INDEX += OPENFILE.gcount(); // TODO need last data received?

          //   OPENFILE.seekg(OPENFILE_INDEX - 5);
          //   OPENFILE.read(&encoded_packet2[8], 100);

          //   for (std::vector<char>::const_iterator k = encoded_packet2.begin(); k != encoded_packet2.end(); ++k)
          //       std::cout << *k;


          //   for (std::vector<char>::const_iterator i = encoded_packet.begin(); i != encoded_packet.end(); ++i)
          //       std::cout << *i;
          //   cout << endl;
          // }




          continue; 
        } else {
          // Retransmit
          SSTHRESH = max((CWND / 2), 1024); // minimum CWND is 1024
          CWND = INITIAL_CWND; 

          // retransmit SYN-ACK
          if (STAGE == CONNECTION_OPEN) { 
            Header syn_ack = Header(SEQ_NUM, ACK_NUM, 0, 1, 1, 0);
            if (sendto(sockfd, (void *) &syn_ack, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, cli_len) < 0) {
              cerr << "Error sending SYN ACK from server to client" << endl;
              exit(-1);
            }

            cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << " Retransmission SYN" << endl;
            STAGE = CONNECTION_OPEN;
            LAST_BYTE_SENT = SEQ_NUM; 
            SEQ_NUM = (SEQ_NUM + 1) % MSN;
          } else if (STAGE == CONNECTION) { 

          } else if (STAGE == CONNECTION_CLOSE) {

          } else {

          }


        }
        

      }
    }






  }

  

  return 0;
}
