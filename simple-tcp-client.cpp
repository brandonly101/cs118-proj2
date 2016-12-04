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

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "header.h"

using namespace std;

// Constants.
const timeval SOCK_RTO { 0, 500000 };

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

    // Initiate the TCP connection via the three-way handshake. Create and send a SYN packet.
    Header sendSyn(0, 0, 0, false, true, false);
    vector<char> sendSynEncoded = sendSyn.encode();
    if (send(sockfd, &sendSynEncoded[0], sendSynEncoded.size(), 0) == -1) {
        perror("send() error");
        return 5;
    }
    cout << printSendMessage(sendSyn, false) << endl;

    // Try and receive the server's SYN-ACK.
    vector<char> recvSynEncoded(HEADER_SIZE);
    Header received;
    while (!received.isSyn() || !received.isAck()) {
        recv(sockfd, &recvSynEncoded[0], recvSynEncoded.size(), 0);
        received.decode(recvSynEncoded);
    }
    cout << received.printRecieveMessage(received) << endl;

    // The server's SYN-ACK has been received. Send out an ACK to the server.
    Header sendAck(0, 0, 0, true, false, false);
    vector<char> sendAckEncoded = sendAck.encode();
    cout << printSendMessage(sendAckEncoded, false);
    if (send(sockfd, &sendAckEncoded[0], sendAckEncoded.size(), 0) == -1) {
         perror("send() error");
         return 4;
    }

    // Receive the server's packets. Break out until a FIN packet is received.
    vector<char> recvPacketEncoded(MAX_PACKET_LEN);
    Header recvPacket;
    while (!recvPacket.isFin()) {
        recv(sockfd, &recvPacketEncoded[0], recvPacketEncoded.size(), 0);
        recvPacket.decode(recvSynEncoded);
        // vector<char> recvPacketPayload(&recvPacketEncoded[HEADER_SIZE], &recvPacketEncoded[MAX_PACKET_LEN]);
    }

    // struct sockaddr_in clientAddr;
    // socklen_t clientAddrLen = sizeof(clientAddr);
    // if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1)
    // {
    //     perror("getsockname() error");
    //     return 3;
    // }

    // char ipstr[INET_ADDRSTRLEN] = {'\0'};
    // inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    // cout << "Set up a connection from: " << ipstr << ":" <<
    // ntohs(clientAddr.sin_port) << endl;

    // Create the HTTP request. For now, the only method is GET 1.0.
    // HTTPRequest req;
    // req.setMethod("GET");
    // req.setURL(urlObj.path);
    // //cout << getFileContentType(urlObj.path);
    // req.setVersion("1.0");
    // string encodedReq = req.encode();

    // // Send the HTTP request.
    // if (send(sockfd, encodedReq.c_str(), encodedReq.size(), 0) == -1)
    // {
    //     perror("send() error");
    //     return 4;
    // }

    // send/receive data to/from connection
    // char buffer[MAX_HTTP_MESSAGE_SIZE];
    // memset(buffer, '\0', sizeof(buffer));

    // Wait for and receive the HTTP response.
    // if (recv(sockfd, buffer, MAX_HTTP_MESSAGE_SIZE, 0) == -1)
    // {
    //     perror("recv() error");
    //     return 5;
    // }
    // cout << buffer << endl;
    // string header = string(buffer);

    // Download the file...
    // HTTPResponse response;
    // response.decode(string(buffer));
    // string filename = "." + req.getURL();
    // int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 777);
    // if (write(fd, response.getBody().c_str(), response.getBody().length()) == -1)
    // {
    //     perror("read() error");
    //     return -1;
    // }
    // close(fd);

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

string printSendMessage(Header head, bool retransmit) {
    string flags = "";
    if (head.isSyn())
        flags = "SYN";
    else if (head.isFin())
        flags = "FIN";
    else if (retransmit)
        flags = "Retransmission";

    if (flags != "")
        flags = " " + flags;

    return "Sending packet " + head.getAckNum() + flags;
}

string printRecieveMessage(Header head) {
    return "Receiving Packet " + head.getSeqNum();
}