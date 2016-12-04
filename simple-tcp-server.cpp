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
#include <algorithm>

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

int INITIAL_CWND = MSS;
int INITIAL_SSTHRESH = 15360; // initial slow start threshold (bytes)
int LAST_BYTE_SENT = -1;
int LAST_BYTE_ACKED = -1;
int file_size = 4096;

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


  ifstream OPENFILE (FILENAME);
  OPENFILE.seekg(0, ios::end);
  int OPENFILE_SIZE = OPENFILE.tellg();
  OPENFILE.seekg(0);

  vector<char> buf(HEADER_SIZE); // TODO check if this gets flushed

  int bytes_recv;
  srand(time(NULL)); // get random sequence number to start with
  int SEQ_NUM = rand() % MSN;
  int ACK_NUM = 0, BASE_NUM = 0;
  int SSTHRESH = INITIAL_SSTHRESH; 
  int CWND = INITIAL_CWND;
  int OPENFILE_INDEX = 0;
  int LAST_ACKED_OPENFILE_INDEX = 0;

  // gettimeofday(&START_TIME, NULL);
  while(1) {
  	// gettimeofday(&current, NULL); // TODO does sockopt handle this?


    // handle data from client
  	if ((bytes_recv = recvfrom(sockfd, &buf[0], HEADER_SIZE, 0, (struct sockaddr*) &cli_addr, &cli_len)) != -1) {
  		Header decoded_packet; 
  		ACK_NUM = (decoded_packet.getSeqNum() + 1) % MSN;
      // SEQ_NUM = decoded_packet.getAckNum() % MSN; // TODO why?

  		decoded_packet.decode(buf); 

      // HANDLE SYN 
  		if (decoded_packet.isSyn()) {
        cout << "Receiving packet " << decoded_packet.getAckNum() << endl;

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
  		} else if (decoded_packet.isAck() && !decoded_packet.isFin()) {
        cout << "Receiving packet " << decoded_packet.getAckNum() << endl;

        SEQ_NUM = decoded_packet.getAckNum() % MSN;

        STAGE = CONNECTION; 
        cout << "Receiving packet " << decoded_packet.getAckNum() << endl;


        int acked_bytes = ((decoded_packet.getAckNum() - 1) < 0) ? MSN : decoded_packet.getAckNum() - 1; 
        if (acked_bytes > LAST_BYTE_ACKED) {
          LAST_ACKED_OPENFILE_INDEX += (acked_bytes - LAST_BYTE_ACKED); 
          LAST_BYTE_ACKED = acked_bytes; 
        } else if (acked_bytes < LAST_BYTE_ACKED && (LAST_BYTE_ACKED - acked_bytes) > (MSN + 1)/2) { // wrap around
          int inc = MSN - LAST_BYTE_ACKED + acked_bytes; 
          LAST_ACKED_OPENFILE_INDEX += inc; 
          LAST_BYTE_ACKED = acked_bytes; 

        } 

        // TODO check if inflight == 0?
        if (false) {
        // if (LAST_ACKED_OPENFILE_INDEX >= OPENFILE_SIZE) {
          // HANDLE FIN
          STAGE = CONNECTION_CLOSE;
          


        } else {
          // resume where we left off for data
          // calculate bytes inflight, have MSN first because of unsigned op
          int inflight = ((MSN + LAST_BYTE_SENT) - LAST_BYTE_ACKED) % MSN;

          // send file

          int WINDOW_SIZE = min((int) decoded_packet.getWindow(), min(CWND, (MSN + 1) / 2)) - inflight; 

          // if (!OPENFILE.is_open()) {
          //   cerr << "Error, file is not open!" << endl;
          //   exit(-1);
          // }

          // TODO handle duplicate ACKs
          if (CWND < SSTHRESH) { 
            // Slow Start
            CWND += MSS; 
          } else {
            // Congestion Avoidance
            CWND += MSS * (MSS / CWND);
          }

          // TODO have CWND as a double?
          int bytes_sent = 0;
          int bytes_read;

          
          //
          // START OF HARCODE
          //
          Header data; // ACK flag set

          // OPENFILE.read(&encoded_packet[HEADER_SIZE], 1024);
          // bytes_read = OPENFILE.gcount();
          bytes_read = MSS;
          OPENFILE_INDEX += bytes_read; // TODO need last data received?

          if (decoded_packet.getAckNum() < 10240) { // Hardcoded case for testing fin.
            data = Header(SEQ_NUM, ACK_NUM, WINDOW_SIZE, 0, 0, 0); // ACK flag set
            cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << endl;
          } else {
            data = Header(SEQ_NUM, ACK_NUM, WINDOW_SIZE, 0, 0, 1); // ACK flag set
            cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << " FIN" << endl;
            STAGE = CONNECTION_CLOSE;
          }

          vector<char> encoded_packet = data.encode();
          encoded_packet.resize(MAX_PACKET_LEN);

          if (sendto(sockfd, &encoded_packet[0], encoded_packet.size(), 0, (struct sockaddr*) &cli_addr, cli_len) < 0) {
              cerr << "Error sending packet" << endl;
              exit(-1);
          }
          LAST_BYTE_SENT = SEQ_NUM;
          // SEQ_NUM = (SEQ_NUM + bytes_read) % MSN;

          //
          // END OF HARCODE
          //

          /* START OF PREV CODE
          while (bytes_sent < WINDOW_SIZE && !OPENFILE.eof()) {
          // while (CWND - CWND_USED >= MSS && !OPENFILE.eof()) {
            Header data = Header(SEQ_NUM, ACK_NUM, 0, 1, 0, 0); // ACK flag set

            vector<char> encoded_packet = data.encode(); 
            encoded_packet.resize(MSS);
            
            OPENFILE.read(&encoded_packet[OPENFILE_INDEX], 1024); 

            bytes_read = OPENFILE.gcount();

            OPENFILE_INDEX += bytes_read;
            bytes_sent += bytes_read; 

            if (sendto(sockfd, &encoded_packet[0], encoded_packet.size(), 0, (struct sockaddr*) &cli_addr, cli_len) < 0) {
              cerr << "Error sending packet" << endl;
              exit(-1);
            } 
            cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << endl; 

            LAST_BYTE_SENT = SEQ_NUM; 
            SEQ_NUM = (SEQ_NUM + bytes_read) % MSN; 
          }
          */
        }

        


      } else if (decoded_packet.isFin()) {
        cout << "Done. Connection closed" << endl;

      }
  	} /* else {
      // Timeout
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // handle the case if we haven't started our 3 way handshake
        if (STAGE == NOT_CONNECTED) {
        // string data;
        // data.resize(100);
        // OPENFILE.seekg(10);
        // OPENFILE.read(&data[0], 100);
        // cout << data << endl;
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
        }
        continue;
      } else {
        // Retransmit
        SSTHRESH = max(CWND/2, MSS); // minimum CWND is 1024
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

        // retransmit data
        } else if (STAGE == CONNECTION) {
          OPENFILE_INDEX = LAST_ACKED_OPENFILE_INDEX;
          OPENFILE.seekg(OPENFILE_INDEX);

          SEQ_NUM = (LAST_BYTE_ACKED + 1) % MSN;
        // retransmit FIN
        } else if (STAGE == CONNECTION_CLOSE) {
          Header fin_ack(SEQ_NUM, ACK_NUM, WINDOW_SIZE, 1, 0, 1);
          vector<char> encoded_fin_packet = fin_ack.encode();
          if (sendto(sockfd, (void *) &encoded_fin_packet, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, cli_len) < 0) {
            cerr << "Error sending FIN ACK from server to client" << endl;
            exit(-1);
          }
          cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << " Retransmission FIN" << endl;
        
        } else {


      }


    }*/
  }
}
