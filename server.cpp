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

string portNumber = "";
string fileName = "";

int main(int argc, char* argv[])
{

  if (argc != 3) {
  	cout << "Invalid # of arguments";
  	exit(1);
  }

  portNumber = argv[1];
  fileName = argv[2];
  return 0;
}
/*
string HOSTNAME = "127.0.0.1";
string PORTNUM = "4000";
string DIRECTORY = ".";
string HTTP_VERSION = "1.0";

const size_t MAX_HTTP_MESSAGE_SIZE = 65536;

void sendResponse(int clientSockfd, HTTPRequest request);
void sendEncoded(int fd, string wire);

int main(int argc, char* argv[])
{
    cout << "Webserver is now running" << endl;

    // Check if there are a correct number of arguments with the command.
	if (argc != 1 && argc != 4) {
		cout << "Error Msg" << endl;
		exit(1);
	}

    // Set the specified parameters rather than just the default ones.
	if (argc == 4) {
		HOSTNAME = (strcmp(argv[1], "localhost") == 0 ? "127.0.0.1" : argv[1]);
		PORTNUM = argv[2];
		DIRECTORY = argv[3];
	}

    // create a socket using TCP IP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // allow others to reuse the address
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt() error");
        return 1;
    }

    // bind address to socket
    struct sockaddr_in addr;
    int port = atoi(PORTNUM.c_str());
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);     // short, network byte order
    addr.sin_addr.s_addr = inet_addr(HOSTNAME.c_str());
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind() error");
        return 2;
    }

    // set socket to listen status
    if (listen(sockfd, 1) == -1)
    {
        perror("listen() error");
        return 3;
    }

    // read/write data from/into the connection
    char buffer[MAX_HTTP_MESSAGE_SIZE] = {0};

    while (1)
    {
        // accept a new connection
        struct sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSockfd == -1)
        {
            perror("accept() error");
            return 4;
        }

        char ipstr[INET_ADDRSTRLEN] = {'\0'};
        inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));

        // Prepare 'buffer'; set '\0' to all bytes of 'buffer'.
        memset(buffer, '\0', sizeof(buffer));

        // Check for any incoming requests from web-client, and throw error if failure.
        // When a request comes in, fill the encoded contents into buffer.
        if (recv(clientSockfd, buffer, MAX_HTTP_MESSAGE_SIZE, 0) == -1)
        {
            perror("recv() error");
            return 5;
        }

        // Create httprequest object here, and have it decode whatever is in buffer;
        // use HTTPRequest.decode.
        HTTPRequest request;
        HTTPResponse response;

        if (request.decode(string(buffer)) == false)
        {
            response.setVersion(HTTP_VERSION);
            response.setStatusCode("400");
            response.setStatusMessage("Bad Request");
            sendEncoded(clientSockfd, response.encode());
            close(clientSockfd);
            continue;
        }
        // cout << "Request received!" << endl;
        // Print out the request line.
        cout << request.getMethod() << " " << request.getURL() << " " << request.getVersion() << endl;

        // Send out a response to web-client. The stuff that will be sent it is
        // in buffer, so I think we're supposed to either change 'buffer' to
        // another variable or just change its contents before calling this
        // function...
        sendResponse(clientSockfd, request);

        close(clientSockfd);
    }

    return 0;
}

string getFileContentType(string file)
{
	if (file.length() < 3)
		return "";

    int i;
    for (i = file.length() - 1; i >= 0; i--) {
        if (file[i] == '.')
            break;
    }

    if (i == 0)
        return "";

    string fileType = file.substr(i);

    if (fileType == ".html")
        return "text/html";
    if (fileType == ".jpg")
        return "image/jpg";
    if (fileType == ".jpeg")
        return "image/jpeg";
    if (fileType == ".gif")
        return "image/gif";
    if (fileType == ".png")
        return "image/png";
    if (fileType == ".txt")
        return "text/plain";

    return "error";
}

void sendEncoded(int fd, string wire)
{
    if (send(fd, wire.c_str(), wire.length(), 0) == -1)
    {
        perror("send() error");
        exit(6);
    }
}

void sendResponse(int clientSockfd, HTTPRequest request)
{
    // Create the HTTP Response.
    HTTPResponse response;
    response.setVersion(HTTP_VERSION);

    // Check if URL is just "/"; if so, simply default it to "/index.html".
    if (request.getURL() == "/")
    {
        request.setURL("/index.html");
    }

    string filename = DIRECTORY + request.getURL();
    int fd = open(filename.c_str(), O_RDONLY);

    // If the file was not found, prep the response for a 404 Not Found and
    // send it.
    if (fd == -1)
    {
        response.setStatusCode("404");
        response.setStatusMessage("Not Found");
        sendEncoded(clientSockfd, response.encode());
        close(fd);
        return;
    }

    // Store the contents of the file into a string.

    // Get file information.
    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("fstat() error");
        exit(-1);
    }

    // Get the file itself into a buffer and then into the response body.
    char buffer[MAX_HTTP_MESSAGE_SIZE];
    memset(buffer, '\0', sizeof(buffer));
    if (read(fd, buffer, st.st_size) == -1)
    {
        perror("read() error");
        exit(-1);
    }
    string bufferString = buffer;
    response.setStatusCode("200");
    response.setStatusMessage("OK");
    response.setHeader("Content-length", to_string(st.st_size));
    response.setHeader("Content-type", getFileContentType(request.getURL()));
    response.setBody(bufferString);
    sendEncoded(clientSockfd, response.encode());

    close(fd);
}
*/