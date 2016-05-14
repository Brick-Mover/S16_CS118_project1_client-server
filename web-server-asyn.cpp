#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "http.hpp"

#define BUFSIZE 512

int
main(int argc, char **argv)
{
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    int port;
    string dir;
    if (argc == 1)
    {
        string ip = "127.0.0.1";
        strcpy(ipstr, ip.c_str());
        port = 4000;
        dir = ".";
    }
    else
    {
        struct addrinfo hints;
        struct addrinfo* res;
        // prepare hints
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        // get address
        int status = 0;
        if ((status = getaddrinfo(argv[1], "http", &hints, &res)) != 0) {
            perror("getaddrinfo: Error");
            //cout<<"Status of getaddrinfo is "<<status;
            return 1;
        }
        for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
            // convert address to IPv4 address
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            // convert the IP to a string
            inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
        }
        freeaddrinfo(res); // free the linked list
        
        port = atoi(argv[2]);
        dir = argv[3];
    }

    fd_set readFds;
    fd_set writeFds;
    fd_set errFds;
    fd_set watchFds;
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_ZERO(&errFds);
    FD_ZERO(&watchFds);
    
    // create a socket using TCP IP
    int listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
    int maxSockfd = listenSockfd;
    
    // put the socket in the socket set
    FD_SET(listenSockfd, &watchFds);
    
    // allow others to reuse the address
    int yes = 1;
    if (setsockopt(listenSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return 3;
    }
    
    // bind address to socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipstr);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    if (::bind(listenSockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 3;
    }
    
    // set socket to listen status
    if (listen(listenSockfd, 32) == -1) {
        perror("listen");
        return 4;
    }
    
    while (true) {
        // set up watcher
        int nReadyFds = 0;
        readFds = watchFds;
        writeFds = watchFds;
        errFds = watchFds;
        if ((nReadyFds = select(maxSockfd + 1, &readFds, &writeFds, &errFds, NULL)) == -1) {
            perror("select");
            return 5;
        }
        
        if (nReadyFds == 0)
            continue;
        else {
            for(int fd = 0; fd <= maxSockfd; fd++) {
                // get one socket for reading
                if (FD_ISSET(fd, &readFds)) {
                    if (fd == listenSockfd) { // this is the listen socket
                        struct sockaddr_in clientAddr;
                        socklen_t clientAddrSize = sizeof(clientAddr);
                        int clientSockfd = accept(fd, (struct sockaddr*)&clientAddr, &clientAddrSize);
                        if (clientSockfd == -1) {
                            perror("accept");
                            return 6;
                        }
                        
                        // update maxSockfd
                        if (maxSockfd < clientSockfd)
                            maxSockfd = clientSockfd;
                        
                        // add the socket into the socket set
                        FD_SET(clientSockfd, &watchFds);
                    }
                    else { // this is the normal socket
                            string instring;
							char inbuf[BUFSIZE];
							long recvLen;
							        while (instring.find("\r\n\r\n") == string::npos ||
										   instring.rfind("\r\n\r\n") != instring.length()-4)
									{
										if ((recvLen = recv(fd, inbuf, BUFSIZE, MSG_DONTWAIT)) == -1)
										{
											if (errno == EAGAIN || errno == EWOULDBLOCK)
													continue;
											else
											{
												perror("recv");
												exit(6);
											}
										}
										
										instring.append(inbuf, recvLen);
									}
									
									//cout << "Socket " << fd << " receives: " << instring << endl;
									
									vector<string> req_vec = split(instring);
									vector<string>::iterator it = req_vec.begin();
									
									for(; it != req_vec.end(); it++)
									{
										HttpRequest req;
										HttpResponse res;
										
										req.consume(*it);
										res.setVersion(req.getVersion());
										
										if (req.valid == false || req.getmethod() != "GET")
										{
											res.setStatus("400");
											res.setDescription("Bad request");
											res.setHeader("Content-Length", "0");
										}
										else
										{
											string path;
											if (req.getHeader("Host") == "")
												path = dir + parseURL(path).path;
											else
												path = dir + req.getPath();
											
											ifstream infile;
											infile.open(path);
											if (infile.is_open())
											{
												string payload((istreambuf_iterator<char>(infile)),
															   istreambuf_iterator<char>());
												infile.close();
												res.setStatus("200");
												res.setDescription("OK");
												res.setPayload(payload);
												long len = payload.size();
												stringstream ss;
												ss << len;
												string contentlen = ss.str();
												res.setHeader("Content-Length", contentlen);
											}
											else
											{
												res.setStatus("404");
												res.setDescription("Not found");
												res.setHeader("Content-Length", "0");
											}
										}
										string outstring = res.encode();

										
										if (send(fd, outstring.c_str(), outstring.size(), 0) == -1) {
											perror("send");
											exit(7);
										}
										//cout<<"finish send"<<endl;
										
										if ((req.getVersion() == "HTTP/1.0") ||
											(req.getVersion() == "HTTP/1.1" && req.getHeader("Connection") == "close"))
										{
											close(fd);
											FD_CLR(fd, &watchFds);
										}
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}