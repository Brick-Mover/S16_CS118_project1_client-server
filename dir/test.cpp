#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "http.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#define DEBUG 1

//#include "HttpMessage.hpp"

#define BUFSIZE 1024

int
main(int argc, char **argv)
{
	string a = "www.google.com/aaa/";
	size_t found = a.find_last_of("/");
	cout<<"last / is: "<<a.substr(found)<<endl;
	cout<<"after last / is: "<<a.substr(found+1)<<endl;
	if(a.substr(found) == "/"){
		cout<<"file name is empty"<<endl;
	}
	
	string recvString = 
	"HTTP/1.1 200 OK\r\n"
	"x-servedByHost: prd-10-60-170-45.nodes.56m.dmtio.net\r\n"
	"Cache-Control: max-age=60\r\r"
	"Content-Length: 160934\r\n"
	"Accept-Ranges: bytes\r\n"
	"\r\n"
	"<!DOCTYPE html><html class";
	
	//bool remaining = true;
	/*
	size_t foundSeparation = recvString.find("\r\n\r\n");
	if(foundSeparation != string::npos){
		string bodyString = recvString.substr(foundSeparation+4);
		string cont_header = "Content-Length";
		size_t found_cont_len = recvString.find(cont_header);
		string cont_len_line = recvString.substr(found_cont_len+cont_header.length()+2);
		size_t found_cont_len_end = cont_len_line.find("\r\n");
		string cont_len = cont_len_line.erase(found_cont_len_end);
		cout<<"cont_len is "<<cont_len<<endl;
		int clen = atoi(cont_len.c_str());
		cout<<"clen is "<<clen<<endl;
		
		int remaining = clen - bodyString.length();
		cout<<"remaining byte are: "<<remaining<<endl;
	}
	*/
	
	string filename[3];
	filename[0]="aaa";
	filename[1]="bbb";
	filename[2]="ccc";
   // create a socket using TCP IP
   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   struct sockaddr_in serverAddr;
   serverAddr.sin_family = AF_INET;
   serverAddr.sin_port = htons(4000);     // short, network byte order
   serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
   memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

   // connect to the server
   if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
   {
       perror("connect");
       return 2;
   }
   cout<<"Connection succeed!"<<endl;
   
   struct sockaddr_in clientAddr;
   socklen_t clientAddrLen = sizeof(clientAddr);
   if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1)
   {
       perror("getsockname");
       return 3;
   }
   
   char ipstr[INET_ADDRSTRLEN] = {'\0'};
   inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
   std::cout << "Set up a connection from: " << ipstr << ":" <<
   ntohs(clientAddr.sin_port) << std::endl;
      
   string input[3];
   input[0] = 
   "GET /dir/a.jpg HTTP/1.1\r\n"
   "User-Agent: Wget/1.15 (linux-gnu)\r\n"
   "Accept:*/*\r\n"
   "Host: localhost:4000\r\n"
   "Connection: Keep-Alive\r\n"
   "Keep-Alive: timeout=50\r\n\r\n";
   
   input[1] = 
   "GET /dir/image.jpg HTTP/1.1\r\n"
   "User-Agent: Wget/1.15 (linux-gnu)\r\n"
   "Accept:*/*\r\n"
   "Host: localhost:4000\r\n"
   "Connection: Keep-Alive\r\n"
   "Keep-Alive: timeout=50\r\n\r\n";
   char buf[BUFSIZE] = {0};
   std::stringstream ss;
   
   
   

for (int i = 0; i < 2; i++)
{
  if (send(sockfd, input[i].c_str(), strlen(input[i].c_str()), 0) == -1) {
	perror("send");    
	return 4;
	}
	
  cout<<"send "<<i<<" th request message!"<<endl;
        
  string recvString = "";
  char recvBuf[BUFSIZE];
  int recvLen;
  
  while (true) {
    recvLen = 0;
    memset(recvBuf, '\0', BUFSIZE);
	
    if ((recvLen = recv(sockfd, recvBuf, BUFSIZE, 0)) == -1) {
      perror("recv");
      return 1;
    }
                            
    recvString.append(recvBuf, recvLen);
            
	cout<<"Receive length: "<<recvLen<<endl;
			
    if (recvLen < BUFSIZE){
      cout<<"Receive length is smaller than bufsize"<<endl;
      cout<<"Receive length is: "<<recvLen<<endl;
      break;
    }
  }

  HttpResponse Res;
  //cout<<recvString<<endl;
  cout<<"before consume"<<endl;
  Res.consume(recvString);  
  cout<<"after consume"<<endl;
  string resStatus=Res.getstatus();
  string contentLen = Res.getHeader("Content-Length");
  cout<<"content length is "<<contentLen<<endl;
#ifdef DEBUG
  std::cout<<"status is "<<resStatus<<endl;
#endif
  if(resStatus != "200"){
    std::cout<<"Not a good request to URL"<<endl;
    return 1;
  }
  else{
    ofstream outfile;
    //std::size_t found = url[i].path.find_last_of("/");
    //string filename = url[i].path.substr(found+1);
	string name = filename[i];
    std::cout<<"file name is: "<<name<<endl;
    outfile.open(name);
    if(outfile.is_open()){
      string content = Res.getPayload();
	  std::copy(content.begin(), content.end(), std::ostreambuf_iterator<char>(outfile));
    }
  }
           sleep(1);
   }

   
   close(sockfd);
   
   return 0;
}
//=========================================================
/*
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

  //for(int i=0; i<n_url; i++){
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
  std::cout<<"host is "<<url[0].host<<" and port is "<<url[0].port<<endl;
  
  int status=0;
  if((status = getaddrinfo(url[0].host.c_str(), url[0].port.c_str(), &hints, &res))!=0){
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
  std::cout<<"IP address of "<<url[0].host<<" is "<<ipstr1<<std::endl;
  #endif
  //================================================

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(url[0].port.c_str()));     // short, network byte order
  #ifdef DEBUG
  std::cout<<"sin_port works"<<std::endl;
  #endif
  serverAddr.sin_addr.s_addr = inet_addr(ipstr1);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  #ifdef DEBUG
  std::cout<<"memset serveraddr works"<<std::endl;
  #endif

  // connect to the server
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    return 1;
  }
  
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
  std::cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;


  // send/receive data to/from connection
  // set up http request message
 for(int i=0; i<n_url; i++){
  HttpRequest httpRequest;
  httpRequest.setUrl(url[i]);
  httpRequest.setMethod("GET");
  httpRequest.setPath(url[i].path);
  httpRequest.setVersion("HTTP/1.1");
  httpRequest.setHeader("Host", url[i].host);
  httpRequest.setHeader("Connection", "Keep-Alive");
  httpRequest.setHeader("Keep-Alive", "100")
  ByteBlob testRequest = httpRequest.encode();

#ifdef DEBUG
  std::cout<<"Test request is:"<<endl<<testRequest;
#endif

  //  char buf[20] = {0};
  if (send(sockfd, testRequest.c_str(), strlen(testRequest.c_str()), 0) == -1) {
    perror("send");
    return 1;
  }

  string recvString;
  char recvBuf[BUFSIZE];
  int recvLen;
  
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
    ofstream outfile;
    std::size_t found = url[i].path.find_last_of("/");
    string filename = url[i].path.substr(found+1);
    std::cout<<"file name is: "<<filename;
    outfile.open(filename);
    if(outfile.is_open()){
      string content = Res.getPayload();
	  std::copy(content.begin(), content.end(), std::ostreambuf_iterator<char>(outfile));
    }
  }

  close(sockfd);
  return 0;
}
*/