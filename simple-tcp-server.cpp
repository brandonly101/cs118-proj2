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

    // Open the file and prepare for transmitting it.
    ifstream OPENFILE;
    OPENFILE.open(FILENAME, ifstream::in);
    if (!OPENFILE.is_open()) {
        cerr << "Error, file is not open!" << endl;
        exit(-1);
    }
    OPENFILE.seekg(0, ios::end);
    int OPENFILE_SIZE = OPENFILE.tellg();
    int OPENFILE_INDEX = 0;
    OPENFILE.seekg(0);
    cout << "FILE SIZE : " << OPENFILE_SIZE << endl;

    vector<char> buf(HEADER_SIZE); // TODO check if this gets flushed

    int bytes_recv;
    srand(time(NULL)); // get random sequence number to start with
    int SEQ_NUM = 0;
    int ACK_NUM = 0;
    int SSTHRESH = INITIAL_SSTHRESH;
    int CWND = INITIAL_CWND;
    int LAST_ACKED_OPENFILE_INDEX = 0;

    while(1) {
        // handle data from client
        if ((bytes_recv = recvfrom(sockfd, &buf[0], HEADER_SIZE, 0, (struct sockaddr*) &cli_addr, &cli_len)) != -1) {
            Header decoded_packet;
            decoded_packet.decode(buf);
            ACK_NUM = (decoded_packet.getSeqNum() + 1) % MSN;
           

            // HANDLE SYN
            if (decoded_packet.isSyn()) {
                cout << "Receiving packet " << decoded_packet.getAckNum() << endl;

                // SEND SYN ACK TO CLIENT
                Header syn_ack = Header(SEQ_NUM, ACK_NUM, 0, true, true, 0);
                cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << " SYN" << endl;
                if (sendto(sockfd, (void *) &syn_ack, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, cli_len) < 0) {
                    cerr << "Error sending SYN ACK from server to client" << endl;
                    exit(-1);
                }
                STAGE = CONNECTION_OPEN;
                LAST_BYTE_SENT = SEQ_NUM;
                SEQ_NUM = (SEQ_NUM + 1) % MSN;
                OPENFILE_INDEX = 0;
                OPENFILE.seekg(0);

            // HANDLE ACK
            } else if (decoded_packet.isAck() && !decoded_packet.isFin()) {
                cout << "Receiving packet " << decoded_packet.getAckNum() << endl;
                STAGE = CONNECTION;

                int acked_bytes = ((decoded_packet.getAckNum() - 1) < 0) ? MSN : decoded_packet.getAckNum() - 1;
                if (acked_bytes > LAST_BYTE_ACKED) {
                    LAST_ACKED_OPENFILE_INDEX += (acked_bytes - LAST_BYTE_ACKED);
                    LAST_BYTE_ACKED = acked_bytes;
                } else if (acked_bytes < LAST_BYTE_ACKED && (LAST_BYTE_ACKED - acked_bytes) > (MSN + 1)/2) { // wrap around
                    int inc = MSN - LAST_BYTE_ACKED + acked_bytes;
                    LAST_ACKED_OPENFILE_INDEX += inc;
                    LAST_BYTE_ACKED = acked_bytes;
                }

                // calculate bytes inflight
                int inflight = ((MSN + LAST_BYTE_SENT) - LAST_BYTE_ACKED) % MSN;

                cout << "Last acked file index: " << LAST_ACKED_OPENFILE_INDEX << endl;
                cout << "Inflight bytes: " << inflight << endl;

                // TODO check if inflight == 0?
                if (decoded_packet.isFin()) {
                  // HANDLE FIN ACK
                  STAGE = CONNECTION_CLOSE;
                  cout << "FIN ACK RECEIVED" << endl;

                  cout << LAST_ACKED_OPENFILE_INDEX << " >= " << OPENFILE_SIZE << endl;
                  if (LAST_ACKED_OPENFILE_INDEX >= OPENFILE_SIZE) {
                    
                  }

                } else {
                    // resume where we left off for data


                    // send file
                    int WINDOW_SIZE = min((int) decoded_packet.getWindow(), min(CWND, (MSN + 1) / 2)) - inflight;

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

                    
                    cout << "WINDOW SIZE : " << WINDOW_SIZE << endl;
                    while (bytes_sent < WINDOW_SIZE && !OPENFILE.eof()) {
                      cout << "BYTES READ : " << OPENFILE.gcount() << endl;
                      cout << "BYTES SENT : " << bytes_sent << endl;
                        Header data; // ACK flag set
                        vector<char> encoded_packet;
                        int fileSizeRemaining = OPENFILE_SIZE - OPENFILE_INDEX;

                        if (fileSizeRemaining > MSS) {
                            // Create the header for the packet.
                            data = Header(SEQ_NUM, ACK_NUM, WINDOW_SIZE, 0, 0, 0); // ACK flag set
                            encoded_packet = data.encode();
                            encoded_packet.resize(MAX_PACKET_LEN);
                            cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << endl;
                            SEQ_NUM = (SEQ_NUM + MSS) % MSN;

                            // Read in the file.
                            OPENFILE.read(&encoded_packet[HEADER_SIZE], MSS);
                            OPENFILE_INDEX += MSS;


                        } else {
                            cout << "Sending last packet! " << endl;
                            // Create the header for the packet.
                            data = Header(SEQ_NUM, ACK_NUM, WINDOW_SIZE, 0, 0, 1); // ACK flag set
                            encoded_packet = data.encode();
                            encoded_packet.resize(HEADER_SIZE + fileSizeRemaining);
                            cout << "Sending packet " << SEQ_NUM << " " << CWND << " " << SSTHRESH << " FIN" << endl;
                            SEQ_NUM = (SEQ_NUM + fileSizeRemaining) % MSN;

                            // Read in the file.
                            OPENFILE.read(&encoded_packet[HEADER_SIZE], fileSizeRemaining);

                            STAGE = CONNECTION_CLOSE; 
                        }

                        // Send the packet!
                        if (sendto(sockfd, &encoded_packet[0], encoded_packet.size(), 0, (struct sockaddr*) &cli_addr, cli_len) < 0) {
                            cerr << "Error sending packet" << endl;
                            exit(-1);
                        }
                        LAST_BYTE_SENT = SEQ_NUM;
                        bytes_sent += OPENFILE.gcount();
                    }
                    
                }
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
    OPENFILE.close();
}
