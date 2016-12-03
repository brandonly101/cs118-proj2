#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

string ipAddr = "";
string portNumber = "";

int main(int argv, char* argv[])
{
	if (argv != 3) {
		cout << "Invalid number of arguments";
		exit(1);
	}


	// prob need to do an if else statement for ip addr or port number;
	ipAddr = argv[1];
	portNumber = argv[2];
  return 0;
}

/*


struct URLObject
{
    string host;
    string portnum;
    string path;
};

URLObject parseURL(string url);
void sendRequest(int sockfd, string url);
void receiveResponse(int sockfd);
void getAddr(string host, string portnum, string& ip);
string getFileContentType(string file);

// Constants
const size_t MAX_HTTP_MESSAGE_SIZE = 65536;

int main(int argc, char* argv[])
{
    // Each other argument is a URL for different HTTP requests.
    string urlTemp;
    for (int i = 1; i < argc; i++)
    {
        // Read in the URL arguments into a URL object.
        URLObject urlObj = parseURL(argv[i]);

        // create a socket using TCP IP
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket error");
            return 1;
        }

        string ip = "";
        getAddr(urlObj.host, urlObj.portnum, ip);

        if (ip == "") { continue; }

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
            return 2;
        }

        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1)
        {
            perror("getsockname() error");
            return 3;
        }

        char ipstr[INET_ADDRSTRLEN] = {'\0'};
        inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
        // cout << "Set up a connection from: " << ipstr << ":" <<
        // ntohs(clientAddr.sin_port) << endl;

        cout << "Sending a request ..." << endl;

        // Create the HTTP request. For now, the only method is GET 1.0.
        HTTPRequest req;
        req.setMethod("GET");
        req.setURL(urlObj.path);
		//cout << getFileContentType(urlObj.path);
        req.setVersion("1.0");
        string encodedReq = req.encode();

        // Send the HTTP request.
        if (send(sockfd, encodedReq.c_str(), encodedReq.size(), 0) == -1)
        {
            perror("send() error");
            return 4;
        }

        // send/receive data to/from connection
        char buffer[MAX_HTTP_MESSAGE_SIZE];

        memset(buffer, '\0', sizeof(buffer));

        // Wait for and receive the HTTP response.
        if (recv(sockfd, buffer, MAX_HTTP_MESSAGE_SIZE, 0) == -1)
        {
            perror("recv() error");
            return 5;
        }
        cout << buffer << endl;
        string header = string(buffer);

        // Download the file...
        HTTPResponse response;
        response.decode(string(buffer));
        string filename = "." + req.getURL();
        int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 777);
        if (write(fd, response.getBody().c_str(), response.getBody().length()) == -1)
        {
            perror("read() error");
            return -1;
        }
        close(fd);

        //receiveResponse(sockfd, buffer);
        close(sockfd);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Helper function implementations.
////////////////////////////////////////////////////////////////////////////////
URLObject parseURL(string url)
{
    cout << "URL TO BE PARSED IS " << url << endl;
    // Clean the URL by removing the "http://".
    if (url.substr(0, 7) == "http://") { url = url.substr(7); }

    // Create the URL struct.
    URLObject urlObject;

    // Parse the URL and apply it to the URL object as appropriate.
    size_t colon = url.find(":");
    size_t slash = url.find("/");

    string host = url;
    string port = "80";
    string path = "/";

    if (colon != string::npos) {
        host = url.substr(0, colon);
        host = host =="localhost" ? "127.0.0.1" : host;

        if (slash != string::npos) {
            port = url.substr(colon + 1, slash - (colon + 1));
        }
    } else if (slash != string::npos) {
        host = url.substr(0, slash);
    }

    if (slash != string::npos) {
        path = url.substr(slash);
    }

    urlObject.path = path;
    urlObject.host = host;
    urlObject.portnum = port;

    return urlObject;
}

void getAddr(string host, string portnum, string &ip)
{
    struct addrinfo* res;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host.c_str(), portnum.c_str(), &hints, &res);
    if (status != 0) {
        cerr << "getaddrinfo error" << endl;
        exit(2);
    }

    char ret[INET_ADDRSTRLEN] = {'\0'};
    struct sockaddr_in* ip_struct = (struct sockaddr_in*) res[0].ai_addr;

    inet_ntop(res[0].ai_family, &(ip_struct->sin_addr), ret, sizeof(ret));

    ip = ret;
    freeaddrinfo(res);
}


string responseMsg(string statusCode)
{
    if (statusCode == "200")
        return "HTTP/1.0 200 OK\r\n";
    if (statusCode == "400")
        return "HTTP/1.0 400 Bad Request\r\n";
    if (statusCode == "404")
        return "HTTP/1.0 404 Not Found\r\n";
    return "Invald or not supported status code\r\n."
}
*/