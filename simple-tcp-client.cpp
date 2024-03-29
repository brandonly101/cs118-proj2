#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <time.h>

#include "header.h"

using namespace std;

// Constants.
const timeval SOCK_RTO { 0, 500000 };
const string OUTPUT_FILENAME = "received.data";

struct URLObject
{
    string host;
    string portnum;
    string path;
};

URLObject parseURL(string url, string port);
void sendSYN(int sockfd, string url);
void receiveACK(int sockfd);
string getAddr(string host, string portnum);
string getFileContentType(string file);

int main(int argc, char* argv[])
{
    if (argc != 3) {
        cout << "Invalid number of arguments." << endl;
        return 1;
    }

    // Read in the URL arguments into a URL object.
    URLObject urlObj = parseURL(argv[1], argv[2]);

    // create a socket using UDP IP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket() error");
        return 2;
    }

    // set retransmission time
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &SOCK_RTO, sizeof(timeval)) == -1) {
        perror("setsockopt() error");
        return 3;
    }

    string ip = getAddr(urlObj.host, urlObj.portnum);
    struct sockaddr_in serverAddr;
    int port = atoi(urlObj.portnum.c_str());
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);     // short, network byte order
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    // connect to the server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("connect() error");
        return 4;
    }

    // Keep record of an internal sequence # and ack. #.
    int seqNum = 0;
    int ackNum = 0;

    // Try and receive the server's SYN-ACK.
    vector<char> recvSynAckEncoded(HEADER_SIZE);
    Header received;
    while (!received.isSyn() || !received.isAck()) {
        // Initiate the TCP connection via the three-way handshake. Create and send a SYN packet.
        Header sendSyn(seqNum, 0, 0, false, true, false);
        vector<char> sendSynEncoded = sendSyn.encode();
        cout << "Sending packet " << sendSyn.getAckNum() << " SYN" << endl;
        if (send(sockfd, &sendSynEncoded[0], sendSynEncoded.size(), 0) == -1) {
            perror("send() error");
            return 5;
        }
        seqNum++;

        recv(sockfd, &recvSynAckEncoded[0], recvSynAckEncoded.size(), 0);
        received.decode(recvSynAckEncoded);
        ackNum = (received.getSeqNum() + 1) % MSN;
        // cout << "AckNum: " << received.getAckNum() << " SeqNum: " << received.getSeqNum() << endl;
        // cout << "ACK Flag: " << received.isAck() << " SYN Flag: " << received.isSyn() << endl;
        if (received.isSyn() && received.isAck()) {
            cout << "Recieving packet " << received.getSeqNum() << endl;
            break;
        }
    }

    // The server's SYN-ACK has been received. Send out an ACK to the server.
    Header sendAck(seqNum, ackNum, MAX_RECVWIN, true, false, false);
    vector<char> sendAckEncoded = sendAck.encode();
    cout << "Sending packet " << sendAck.getAckNum() << endl;
    if (send(sockfd, &sendAckEncoded[0], sendAckEncoded.size(), 0) == -1) {
         perror("send() error");
         return 4;
    }
    seqNum++;

    // Open an output filestream to write too, along with a vector for a buffer.
    ofstream ofs;
    ofs.open(OUTPUT_FILENAME, ofstream::out | ofstream::trunc | ofstream::binary);
    int currFileSize = 0;
    int maxFileSize = 65536;
    vector<char> buffer;
    buffer.reserve(maxFileSize);

    // Receive the server's packets. Break out until a FIN packet is received.
    bool first = true;
    vector<char> recvPacketEncoded(MAX_PACKET_LEN);
    Header recvPacket;
    while (!recvPacket.isFin()) {
        // Receive the packet and parse the header. Then, parse the data segment.
        int bytesReceived = recv(sockfd, &recvPacketEncoded[0], recvPacketEncoded.size(), 0);
        if (bytesReceived <= 0 && first) {
            if (send(sockfd, &sendAckEncoded[0], sendAckEncoded.size(), 0) == -1) {
                perror("send() error");
                return 4;
            }
        } else {
            first = false;
            recvPacket.decode(recvPacketEncoded);
            cout << "Receiving packet " << recvPacket.getSeqNum() << endl;
            if (ackNum < recvPacket.getSeqNum()) {
                // Packet proper order packet has not been received. Send an ACK packet back with the same ACK.
                Header sendAckPacket(seqNum, ackNum, MAX_RECVWIN, 1, 0, recvPacket.isFin());
                vector<char> sendAckPacketEncoded = sendAckPacket.encode();
                cout << "Sending packet " << sendAckPacket.getAckNum() << endl;
                if (send(sockfd, &sendAckPacketEncoded[0], sendAckPacketEncoded.size(), 0) == -1) {
                     perror("send() error");
                     return 4;
                }
                seqNum = (seqNum + 1) % MSN;
            } else {
                ackNum = (recvPacket.getSeqNum() + bytesReceived - HEADER_SIZE) % MSN;

                // Read the data segment into the buffer.
                vector<char> recvPacketSegment(&recvPacketEncoded[HEADER_SIZE], &recvPacketEncoded[bytesReceived]);
                if (maxFileSize <= currFileSize) {
                    maxFileSize *= 2;
                    buffer.reserve(maxFileSize);
                }
                buffer.insert(buffer.end(), recvPacketSegment.begin(), recvPacketSegment.end());
                currFileSize += bytesReceived - HEADER_SIZE;

                // Packet has been received. Send an ACK packet back.
                Header sendAckPacket(seqNum, ackNum, MAX_RECVWIN, 1, 0, recvPacket.isFin());
                vector<char> sendAckPacketEncoded = sendAckPacket.encode();
                cout << "Sending packet " << sendAckPacket.getAckNum() << (recvPacket.isFin() ? " FIN" : "") << endl;
                if (send(sockfd, &sendAckPacketEncoded[0], sendAckPacketEncoded.size(), 0) == -1) {
                     perror("send() error");
                     return 4;
                }
                seqNum = (seqNum + 1) % MSN;
            }
        }
    }

    // Write the file to stream.
    ofs.write(&buffer[0], buffer.size());
    ofs.close();
    close(sockfd);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Helper function implementations.
////////////////////////////////////////////////////////////////////////////////
URLObject parseURL(string url, string port)
{
    // Clean the URL by removing the "http://".
    if (url.substr(0, 7) == "http://") {
        url = url.substr(7);
    }

    // Create the URL struct.
    URLObject urlObject;

    // Parse the URL and apply it to the URL object as appropriate.
    size_t colon = url.find(":");
    size_t slash = url.find("/");

    string host = url;
    string path = "/";

    if (colon != string::npos) {
        host = url.substr(0, colon);
        host = (host == "localhost") ? "127.0.0.1" : host;

        if (slash != string::npos) {
            port = url.substr(colon + 1, slash - (colon + 1));
        }
    } else if (slash != string::npos) {
        host = url.substr(0, slash);
    }

    urlObject.host = host;
    urlObject.portnum = port;

    return urlObject;
}

string getAddr(string host, string portnum)
{
    struct addrinfo* res;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(host.c_str(), portnum.c_str(), &hints, &res);
    if (status != 0) {
        cerr << "getaddrinfo error" << endl;
        exit(2);
    }

    char ret[INET_ADDRSTRLEN] = {'\0'};
    struct sockaddr_in* ip_struct = (struct sockaddr_in*) res[0].ai_addr;

    inet_ntop(res[0].ai_family, &(ip_struct->sin_addr), ret, sizeof(ret));

    string ip = ret;
    freeaddrinfo(res);

    return ip;
}