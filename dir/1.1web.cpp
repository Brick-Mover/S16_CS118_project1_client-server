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
#include <vector>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

#define DEBUG 1

int
main(int argc, char *argv[])
{
  //check if only single argument
  if(argc == 1){
    cerr << "Missing URL input"<<endl;
    return 1;
  }

  int n_url=argc-1;
  URL *url = new URL[n_url];

  for(int i=0; i<n_url; i++){
    url[i] = parseURL(argv[i+1]);
    if(url[i].port == ""){
      url[i].port="80";
    }
	size_t f = url[i].path.find_last_of("/");
	if(url[i].path == "/" || url[i].path.substr(f)=="/"){
		url[i].path = "/index.html";
	}
    #ifdef DEBUG
    cout<<i<<"th URL path is:"<<url[i].path<<" URL host is:"<<url[i].host<<" URL port is:"<<url[i].port<<endl;
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
  cout<<"host is "<<url[0].host<<" and port is "<<url[0].port<<endl;
  
  int status=0;
  if((status = getaddrinfo(url[0].host.c_str(), url[0].port.c_str(), &hints, &res))!=0){
    cerr << "getaddrinfo: "<< gai_strerror(status) << endl;
    return 1;
  }

  // convert the IP to a sring
  struct addrinfo* p = res;
  struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
  char ipstr1[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr1, sizeof(ipstr1));
  
  //test only
  #ifdef DEBUG
  cout<<"IP address of "<<url[0].host<<" is "<<ipstr1<<endl;
  #endif
  //================================================

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(url[0].port.c_str()));     // short, network byte order
  #ifdef DEBUG
  cout<<"sin_port works"<<endl;
  #endif
  serverAddr.sin_addr.s_addr = inet_addr(ipstr1);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  #ifdef DEBUG
  cout<<"memset serveraddr works"<<endl;
  #endif

  // connect to the server
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    return 1;
  }
  
  #ifdef DEBUG
  cout<<"Connection succeed"<<endl;
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
  cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << endl;


  // send/receive data to/from connection
  // set up http request message
  ByteBlob *testRequest = new ByteBlob[n_url];
  for(int i=0; i<n_url; i++){
  HttpRequest httpRequest;
  httpRequest.setUrl(url[i]);
  httpRequest.setMethod("GET");
  httpRequest.setPath(url[i].path);
  httpRequest.setVersion("HTTP/1.1");
  httpRequest.setHeader("Host", url[0].host);
  httpRequest.setHeader("Connection", "Keep-Alive");
  testRequest[i] = httpRequest.encode();

  //  char buf[20] = {0};
  if (send(sockfd, testRequest[i].c_str(), strlen(testRequest[i].c_str()), 0) == -1) {
    perror("send");
    return 1;
  }
  cout<<"send the requset message "<<endl;
  
  }
  
  // set timer
  struct timeval timeout;
  timeout.tv_sec=3;
  timeout.tv_usec=0;
  
  fd_set active_fd_set;
  /*
  FD_ZERO (&active_fd_set);
  FD_SET (sockfd, &active_fd_set);
  if(select(FD_SETSIZE, &active_fd_set, NULL, NULL, &timeout) == 0){
	  cout<<"Resend the request message."<<endl;
	  for(int i=0; i<n_url; i++){
	  if (send(sockfd, testRequest[i].c_str(), strlen(testRequest[i].c_str()), 0) == -1) {
		perror("send");
		return 1;
	    }
	  }
  }
  */

  int BUFSIZE=16;
  string recvString;
  char recvBuf[BUFSIZE];
  int recvLen=0;
  
  bool findSeparation;
  int remaining;
  string *recvStringPart = new string[n_url];
  
for(int i=0; i<n_url; i++){
	
  //find the content length
  recvString = "";
  findSeparation = false;
  remaining = 0;
  
  //set timer
  clock_t start;
  double duration;
  start = clock();
  
  while (!findSeparation) {    
    memset(recvBuf, '\0', BUFSIZE);
	
	/*
	FD_ZERO (&active_fd_set);
	FD_SET (sockfd, &active_fd_set);
	if(select(FD_SETSIZE, &active_fd_set, NULL, NULL, &timeout) == 0){
	  cout<<"Time out due to incomplete response message."<<endl;
	  close(sockfd);
	  return 1;
	}
	*/

    if ((recvLen = recv(sockfd, recvBuf, BUFSIZE, MSG_DONTWAIT)) == -1) {
		duration = (clock() - start) /((double) CLOCKS_PER_SEC);
		if(duration == 4){
			cout<<"recvLen in first while loop: "<<recvLen<<endl;
			cout<<"Time out due to incomplete response message (1st -1)."<<endl;
			close(sockfd);
			return 1;
		}
    }
	
	else if(recvLen == 0){
		duration = (clock() - start) /((double) CLOCKS_PER_SEC);
		if(duration == 4){
			cout<<"recvLen in first while loop: "<<recvLen<<endl;
			cout<<"Time out due to incomplete response message (1st 0)."<<endl;
			close(sockfd);
			return 1;
		}
	}
	//cout<<"recvLen in first while loop: "<<recvLen<<endl;
	else{
	cout<<"recvLen in first while loop: "<<recvLen<<endl;
	recvString.append(recvBuf, recvLen);
                            
    size_t foundSeparation = recvString.find("\r\n\r\n");
	if(foundSeparation != string::npos){
		//cout<<"recvString inside foundSeparation is "<<endl<<recvString<<endl;
		string bodyString = recvString.substr(foundSeparation+4);
		string cont_header = "Content-Length";
		size_t found_cont_len = recvString.find(cont_header);
		string cont_len_line = recvString.substr(found_cont_len+cont_header.length()+2);
		size_t found_cont_len_end = cont_len_line.find("\r\n");
		string cont_len = cont_len_line.erase(found_cont_len_end);
		cout<<"cont_len is "<<cont_len<<endl;
		int clen = atoi(cont_len.c_str());
		cout<<"clen is "<<clen<<endl;
		remaining = clen - bodyString.length();
		findSeparation = true;
		cout<<"remaining is "<<remaining<<endl;
	}
	start = clock();
	}	
	//string::npos    
	/*
    if (recvLen < BUFSIZE){
      cout<<"Receive length: "<<recvLen<<endl;
      //cout<<"Receive string is: "<<recvString;
      break;
    }
	*/	
  }
  
  start = clock();
  //receive message until the end
  while(remaining>0){
	memset(recvBuf, '\0', BUFSIZE);
	
	/*
	FD_ZERO (&active_fd_set);
	FD_SET (sockfd, &active_fd_set);
	if(select(FD_SETSIZE, &active_fd_set, NULL, NULL, &timeout) == 0){
	  cout<<"Time out due to incomplete response message."<<endl;
	  close(sockfd);
	  return 1;
	}
	*/

    if ((recvLen = recv(sockfd, recvBuf, BUFSIZE, MSG_DONTWAIT)) == -1) {
		duration = (clock() - start) /((double) CLOCKS_PER_SEC);
		if(duration == 4){
			cout<<"recvLen in second while loop: "<<recvLen<<endl;
			cout<<"Time out due to incomplete response message (2nd -1)."<<endl;
			close(sockfd);
			return 1;
		}
    }
	
	if(recvLen == 0){
		duration = (clock() - start) /((double) CLOCKS_PER_SEC);
		if(duration == 4){
			cout<<"recvLen in second while loop: "<<recvLen<<endl;
			cout<<"Time out due to incomplete response message (2nd 0)."<<endl;
			close(sockfd);
			return 1;
		}
	}
	else{
		//cout<<"recvLen in first while loop: "<<recvLen<<endl;
		recvString.append(recvBuf, recvLen);
		remaining -= recvLen;
		start = clock();
	}
	//cout<<"remaining is "<<remaining<<endl;
                           
	//cout<<"remaining is "<<remaining<<endl;
  }
  
  recvStringPart[i]=recvString;
}

recvString = "";
for(int i=0; i<n_url; i++){
	//int length = recvStringPart[i].length();
	recvString.append(recvStringPart[i].c_str(), recvStringPart[i].length());
}
//size_t found_cont_len_end = recvString.find("\r\n\r\n");
//recvString = recvString.erase(found_cont_len_end+100);
  
  //cout<<"Recv String is "<<recvString<<endl;
  //cout<<"line 178"<<endl;
  cout<<"recvString length is "<<recvString.length()<<endl;
  vector<string> SplitResponse = split_response(recvString);
  //cout<<"line 180"<<endl;
  //cout<<"Receive string is "<<recvString;
  cout<<"vector size is "<<SplitResponse.size()<<endl;
  //}
  //cout<<"line 184"<<endl;
  
for(int i=0; i<n_url; i++){
  HttpResponse Res;
  Res.consume(SplitResponse[i]);
  string contentLen = Res.getHeader("Content-Length");
  cout<<"content length of "<<i<<"th is "<<contentLen<<endl;
  unsigned long contentLEN = atoi(contentLen.c_str());
 unsigned long bodyLEN = Res.getPayload().length();
  if(contentLEN!=bodyLEN){
	  cout<<"Incomplete Response with URL:"<<url[i].host<<url[i].path<<endl;
	  return 1;
  }
  
  string resStatus=Res.getstatus();
#ifdef DEBUG
  cout<<"status is "<<resStatus<<endl;
#endif
  if(resStatus != "200"){
    cout<<"Not a good request to URL:"<<url[i].host<<url[i].path<<endl;
    return 1;
  }
  else{
    //string content = res.getPayload();
    ofstream outfile;
    size_t found = url[i].path.find_last_of("/");
    string filename = url[i].path.substr(found+1);
	//string filename = url[i].path;
    cout<<"file name is: "<<filename<<endl;
    outfile.open(filename);
    if(outfile.is_open()){
	  //cout<<"enter 340"<<endl;
      string content = Res.getPayload();
      //outfile.write(content.c_str(), strlen(content.c_str()));
	  //cout<<"content length "<<content.length();
	  //outfile << content;
	  copy(content.begin(), content.end(), ostreambuf_iterator<char>(outfile));
	  //const char* ch = content.c_str();
	  //char* c=ch;
	  //long len = content.length();
	  //while(len>0){
		//if(len < 1024){
			//outfile.write(c, len);
		//}
		//else{
			//outfile.write(c, 1024);
		//}
		//c += 1024;
		//len -= 1024;
	  //}
	  //cout<<"finish at line 346"<<endl;
    }
  } 
}  

  //free memory
  delete [] url;
  delete [] recvStringPart;

  close(sockfd);
//end of huge for loop

  return 0;
}