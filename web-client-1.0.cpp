#include "http.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

#include <iostream>
#include <fstream>
#include <sstream>

#define DEBUG 1

int
main(int argc, char *argv[])
{
  //check if only single argument
  if(argc == 1){
    std::cerr << "Missing URL input"<<std::endl;
    return 1;
  }

  int n_url=argc-1;
  URL *url = new URL[n_url];

  for(int i=0; i<n_url; i++){
    url[i] = parseURL(argv[i+1]);
    if(url[i].port == ""){
      url[i].port="80";
    }
	if(url[i].path == "/"){
		url[i].path = "/index.html";
	}
    #ifdef DEBUG
    std::cerr<<i<<"th URL path is:"<<url[i].path<<" URL host is:"<<url[i].host<<" URL port is:"<<url[i].port<<std::endl;
    #endif
  }

  for(int i=0; i<n_url; i++){
  // create sockets using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // name resolution to get the IP address
  //================================================
  struct addrinfo hints;
  struct addrinfo* res;
  
  // prepare hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  // get address
  std::cerr<<"host is "<<url[i].host<<" and port is "<<url[i].port<<endl;
  
  int status=0;
  if((status = getaddrinfo(url[i].host.c_str(), url[i].port.c_str(), &hints, &res))!=0){
    std::cerr << "getaddrinfo: "<< gai_strerror(status) << std::endl;
    return 1;
  }

  // convert the IP to a sring
  struct addrinfo* p = res;
  struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
  char ipstr1[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr1, sizeof(ipstr1));
  
  //test only
  #ifdef DEBUG
  std::cerr<<"IP address of "<<url[i].host<<" is "<<ipstr1<<std::endl;
  #endif
  //================================================

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(url[i].port.c_str()));     // short, network byte order
  #ifdef DEBUG
  std::cerr<<"sin_port works"<<std::endl;
  #endif
  serverAddr.sin_addr.s_addr = inet_addr(ipstr1);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  #ifdef DEBUG
  std::cerr<<"memset serveraddr works"<<std::endl;
  #endif

  // connect to the server
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    return 1;
  }
  
  #ifdef DEBUG
  std::cerr<<"Connection succeed"<<std::endl;
  #endif
  // connection established and contruct the HTTP request message
  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
    perror("getsockname");
    return 1;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cerr << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;


  // send/receive data to/from connection
  // set up http request message
  HttpRequest httpRequest;
  httpRequest.setUrl(url[i]);
  httpRequest.setMethod("GET");
  httpRequest.setPath(url[i].path);
  httpRequest.setVersion("HTTP/1.0");
  httpRequest.setHeader("Host", url[i].host);
  ByteBlob testRequest = httpRequest.encode();

#ifdef DEBUG
  std::cerr<<"Test request is:"<<endl<<testRequest;
#endif

  //  char buf[20] = {0};
  if (send(sockfd, testRequest.c_str(), strlen(testRequest.c_str()), 0) == -1) {
    perror("send");
    return 1;
  }

  int BUFSIZE=1024;
  string recvString;
  char recvBuf[BUFSIZE];
  int recvLen;
  
  // set timer
  struct timeval timeout;
  timeout.tv_sec=3;
  timeout.tv_usec=0;
  
  fd_set active_fd_set;
  FD_ZERO (&active_fd_set);
  FD_SET (sockfd, &active_fd_set);
  if(select(FD_SETSIZE, &active_fd_set, NULL, NULL, &timeout) == 0){
	  std::cerr<<"Resend the request message."<<endl;
	  if (send(sockfd, testRequest.c_str(), strlen(testRequest.c_str()), 0) == -1) {
		  perror("send");
		  return 1;
		  }
  }

  
  while (true) {
    recvLen = 0;
    memset(recvBuf, '\0', BUFSIZE);
	
    if ((recvLen = recv(sockfd, recvBuf, BUFSIZE, MSG_WAITALL)) == -1) {
      perror("recv");
      return 1;
    }
                            
    recvString.append(recvBuf, recvLen);
                            
    if (recvLen < BUFSIZE){
      cerr<<"Receive length: "<<recvLen<<endl;
      //cerr<<"Receive string is: "<<recvString;
      break;
    }
  }

  HttpResponse Res;
  Res.consume(recvString);
  string contentLen = Res.getHeader("Content-Length");
  unsigned long contentLEN = atoi(contentLen.c_str());
  unsigned long bodyLEN = Res.getPayload().length();
  std::cerr<<"Content len is "<<contentLEN<<" Body len is "<<bodyLEN<<endl;
  if(contentLEN!=bodyLEN){
	  std::cerr<<"Incomplete Response with URL:"<<url[i].host<<url[i].path<<endl;
	  return 1;
  }
  
  string resStatus=Res.getstatus();
#ifdef DEBUG
  std::cerr<<"status is "<<resStatus<<endl;
#endif
  if(resStatus != "200"){
    std::cerr<<"Not a good request to URL:"<<url[i].host<<url[i].path<<endl;
    return 1;
  }
  else{
    //string content = res.getPayload();
    ofstream outfile;
    std::size_t found = url[i].path.find_last_of("/");
    string filename = url[i].path.substr(found+1);
    std::cerr<<"file name is: "<<filename;
    outfile.open(filename);
    if(outfile.is_open()){
      string content = Res.getPayload();
	  //cerr<<"content length is "<<content.length()<<endl;
      //outfile.write(content.c_str(), strlen(content.c_str()));
	  std::copy(content.begin(), content.end(), std::ostreambuf_iterator<char>(outfile));
    }
  }  

  /*
  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));

    std::cerr << "send: ";
    std::cin >> input;
    


    if (recv(sockfd, buf, 20, 0) == -1) {
      perror("recv");
      return 5;
    }
    ss << buf << std::endl;
    std::cerr << "echo: ";
    std::cerr << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }*/

  close(sockfd);

  }//end of huge for loop

  return 0;
}
