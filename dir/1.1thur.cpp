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

//#define DEBUG 0

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
    std::cout<<i<<"th URL path is:"<<url[i].path<<" URL host is:"<<url[i].host<<" URL port is:"<<url[i].port<<std::endl;
    #endif
  }

  
  for(int i=0; i<n_url; i++){
	  //bool connect_before = false;
	  //bool last_connect = true;
	  //check if already connect before
	  for(int j = 0; j<i; j++){
		  if((url[j].host == url[i].host) && (url[j].port == url[i].port)){
			  //connect_before=true;
			  break;
		  }
	  }
	  
	  //check if it is the last connection
	  for(int k=i+1; k<n_url; k++){
		  if((url[k].host == url[i].host) && (url[k].port == url[i].port)){
			  //last_connect=false;
			  break;
		  }
	  }
	  
	//cout<<"connect befor: "<<connect_before<<" and last connect"<<last_connect<<endl;
	  
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
  //std::cout<<"host is "<<url[i].host<<" and port is "<<url[i].port<<endl;
  
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
  std::cout<<"IP address of "<<url[i].host<<" is "<<ipstr1<<std::endl;
  #endif
  //================================================

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(url[i].port.c_str()));     // short, network byte order
  #ifdef DEBUG
  std::cout<<"sin_port works"<<std::endl;
  #endif
  serverAddr.sin_addr.s_addr = inet_addr(ipstr1);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  #ifdef DEBUG
  std::cout<<"memset serveraddr works"<<std::endl;
  #endif

  // connect to the server
  //if(!connect_before){
	cout<<endl<<"connect "<<i<<endl;
	if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
		perror("connect");
		return 1;
	}
  //}
  
  #ifdef DEBUG
  std::cout<<"Connection succeed"<<std::endl;
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
  //std::cout << "Set up a connection from: " << ipstr << ":" <<
  //  ntohs(clientAddr.sin_port) << std::endl;


  // send/receive data to/from connection
  // set up http request message
  HttpRequest httpRequest;
  httpRequest.setUrl(url[i]);
  httpRequest.setMethod("GET");
  httpRequest.setPath(url[i].path);
  httpRequest.setVersion("HTTP/1.1");
  httpRequest.setHeader("Host", url[i].host);
  //if(last_connect){
	  //httpRequest.setHeader("Connection", "close");	  
  //}
  //else{
	//  httpRequest.setHeader("Connection", "Keep-Alive");
  //}
  httpRequest.setHeader("Connection", "close");
  ByteBlob testRequest = httpRequest.encode();

#ifdef DEBUG
  std::cout<<"Test request is:"<<endl<<testRequest;
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
	  std::cout<<"Resend the request message."<<endl;
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
      cout<<"Receive length: "<<recvLen<<endl;
      //cout<<"Receive string is: "<<recvString;
      break;
    }
  }

  HttpResponse Res;
  Res.consume(recvString);
  string contentLen = Res.getHeader("Content-Length");
  unsigned long contentLEN = atoi(contentLen.c_str());
  unsigned long bodyLEN = Res.getPayload().length();
  std::cout<<"Content len is "<<contentLEN<<" Body len is "<<bodyLEN<<endl;
  if(contentLEN!=bodyLEN){
	  std::cout<<"Incomplete Response with URL:"<<url[i].host<<url[i].path<<endl;
	  return 1;
  }
  
  string resStatus=Res.getstatus();
#ifdef DEBUG
  std::cout<<"status is "<<resStatus<<endl;
#endif
  if(resStatus != "200"){
    std::cout<<"Not a good request to URL:"<<url[i].host<<url[i].path<<endl;
    return 1;
  }
  else{
    //string content = res.getPayload();
    ofstream outfile;
    std::size_t found = url[i].path.find_last_of("/");
    string filename = url[i].path.substr(found+1);
    std::cout<<"file name is: "<<filename<<endl<<endl;
    outfile.open(filename);
    if(outfile.is_open()){
      string content = Res.getPayload();
	  //cout<<"content length is "<<content.length()<<endl;
      //outfile.write(content.c_str(), strlen(content.c_str()));
	  //cout<<"content length is "<<content.length();
	  std::copy(content.begin(), content.end(), std::ostreambuf_iterator<char>(outfile));
    }
  }  


  close(sockfd);

  }//end of huge for loop

  return 0;
}