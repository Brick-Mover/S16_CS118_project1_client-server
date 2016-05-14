#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <thread>

#include <iostream>
#include <fstream>
#include <sstream>

#include "http.hpp"

#define BUFSIZE 512

char ipstr[INET_ADDRSTRLEN] = {'\0'};
int port;
string dir;

void threadfun(int clientSockfd)
{
    string instring;
    char inbuf[BUFSIZE];
    long recvLen;
    
    clock_t begin, end;
    begin = end = clock();
    
    while (true)
    {
        instring = "";
        memset(inbuf, '\0', sizeof(inbuf));
        recvLen = 0;
        
        while (instring.find("\r\n\r\n") == string::npos ||
               instring.rfind("\r\n\r\n") != instring.length()-4)
        {
            if ((recvLen = recv(clientSockfd, inbuf, BUFSIZE, MSG_DONTWAIT)) == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    end = clock();
                    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
                    if (elapsed_secs > 3)
                    {
						//cout<<"close connection "<<endl;
                        close(clientSockfd);
                        return;
                    }
                    else
                        continue;
                }
                else
                {
                    perror("recv");
                    exit(6);
                }
            }
            
            instring.append(inbuf, recvLen);
            begin = clock();
        }
        
        //cout << "Socket " << clientSockfd << " receives: " << instring << endl;
        
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
			//sleep(6);
			/*
			for(int i=0; i<10; i++){
				sendstr = "";
				send(clientSockfd, sendstr.c_str(), sendstr.size(), 0);
				
			}
			*/
            
            if (send(clientSockfd, outstring.c_str(), outstring.size(), 0) == -1) {
                perror("send");
                exit(7);
            }
			//cout<<"finish send"<<endl;
            
            if ((req.getVersion() == "HTTP/1.0") ||
                (req.getVersion() == "HTTP/1.1" && req.getHeader("Connection") == "close"))
            {
                close(clientSockfd);
                return;
            }
        }
    }
}

int
main(int argc, char **argv)
{
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
            perror("getaddrinfo");
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
    
    // create a socket using TCP IP
    int listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // allow others to reuse the address
    int yes = 1;
    if (setsockopt(listenSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return 2;
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
    
    thread *threadptr;
    while (true)
    {
        // accept a new connection
        struct sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSockfd = accept(listenSockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSockfd == -1) {
            perror("accept");
            return 5;
        }
        
        threadptr = new thread (threadfun, clientSockfd);
        threadptr->detach();
    }
    
    return 0;
}
