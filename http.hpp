/* CS118 project 1: HTTP message abstraction */
#ifndef http_hpp
#define http_hpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>

//#define _DEBUG 1

using namespace std;
typedef string HttpVersion;
typedef string ByteBlob;
typedef string HttpMethod;
typedef string HttpStatus;
enum request {GET_STATUS_LENGTH, GET_HEADER_END};

struct URL {
public:
    string path;
    string host;
    string port;
};

URL parseURL(string abs_url);
vector<string> split(const string& str, const string& delimiter = "\r\n\r\n");
vector<string> w_split(const string& str, const string& delimiter = "\r\n\r\n");
vector<string> explode(const string& str);

class HttpMessage {
public:
    /* First line is status line, which contains version, status code,
     status description */
    virtual void decodeFirstLine(ByteBlob line) = 0;
    
    virtual void consume(ByteBlob blob) = 0;
    
    virtual ByteBlob encode() = 0;
    
    HttpVersion getVersion() {return m_version;};
    void setVersion(HttpVersion version) {m_version = version;};
    void setHeader(string key, string value);
    string getHeader(string key);   // search header value based on a key
    string packHeaders();            // generate header lines seperated by "\r\n"
    void decodeHeaderLine();
    
    bool valid = true;
    int h_groupLines(const ByteBlob& line, request m_request);
    
private:
    HttpVersion m_version;
    map<string, string> m_headers;
    vector<string> seperate_lines;  // temperary variable to convert to map, may be deleted later
    
    
};


/* A HTTP request looks like:
 GET /index.html HTTP/1.1\r\n
 User-Agent: Wget/1.15 (linux-gnu)\r\n
 Accept: \r\n
 Host: localhost:4000\r\n
 Connection: Keep-Alive\r\n
 \r\n
 */

class HttpRequest : public HttpMessage {
public:
    virtual void decodeFirstLine(ByteBlob line);
    
    virtual void consume(ByteBlob blob);
    
    virtual ByteBlob encode();
    
    HttpMethod getmethod() {return m_method;};
    void setMethod(HttpMethod method) {m_method = method;};
    URL getUrl() {return m_url;};
    void setUrl(URL url) {m_url = url;};
    void setPath(string path) {m_path = path;};
    string getPath() {return m_path;};
    
private:
    HttpMethod m_method;
    string m_path;
    URL m_url;
};

/* A HTTP response looks like:
 HTTP/1.1 200 OK\r\n
 Content-Type: text/html\r\n
 Server: Apache/2.4.18 (FreeBSD)\r\n
 Content-Length: 91\r\n
 Connection: close\r\n
 \r\n
 <html>\n
 <head>\n
 hello, world\n
 </head>\n
 </html>\n
 */

class HttpResponse : public HttpMessage {
public:
    virtual void decodeFirstLine(ByteBlob line);
    
    virtual void consume(ByteBlob blob);
    
    virtual ByteBlob encode();
    
    HttpStatus getstatus() {return m_status;};
    void setStatus(HttpStatus status) {m_status = status;};
    string getDescription() {return m_statusDescription;};
    void setDescription(string description) {m_statusDescription = description;};
    ByteBlob getPayload() {return m_payload;};
    void setPayload(ByteBlob payload) {m_payload = payload;};
    
private:
    HttpStatus m_status;
    ByteBlob m_body;
    string m_statusDescription;
    ByteBlob m_payload;
};



/* consume() takes a byteblob and set up the fields in class */
void HttpRequest::consume(ByteBlob line) {
    HttpRequest::decodeFirstLine(line);
    HttpMessage::decodeHeaderLine();
}


/* encode() does the opposite of consume, it generates a byteblob to send */
ByteBlob HttpRequest::encode() {
    string statusLine = getmethod() + " " + getPath() + " " + getVersion() + "\r\n";
    return statusLine + packHeaders() + "\r\n"; // the last carriage return signals end of header
}


void HttpResponse::consume(ByteBlob line) {
    HttpResponse::decodeFirstLine(line);
    HttpMessage::decodeHeaderLine();
    int startOfPayload = h_groupLines(line, GET_HEADER_END);
    m_payload = line.substr(startOfPayload);
#ifdef _DEBUG
    cout << m_payload;
#endif
}


ByteBlob HttpResponse::encode() {
    string statusLine = getVersion() + " " + getstatus() + " " + getDescription() + "\r\n";
    return statusLine + packHeaders() + "\r\n" + getPayload();
}



void HttpMessage::setHeader(string key, string value) {
    m_headers.insert(pair<string, string>(key, value));
}

string HttpMessage::packHeaders() {
    map<string, string>::const_iterator p;
    string result = "";
    for (p = m_headers.begin(); p != m_headers.end(); p++) {
        result = result + p->first + ": " + p->second + "\r\n";
    }
    return result;
}


string HttpMessage::getHeader(string key) {
    map<string, string>::const_iterator p;
    for (p = m_headers.begin(); p != m_headers.end(); p++) {
        if (p->first == key)
            return p->second;
    }
    return ""; // if not found, return empty string
}



/* Group the lines by \r\n and store them into the vector seperate_lines,
 either returns status length or returns end of header part according to
 requests. */
int HttpMessage::h_groupLines(const ByteBlob& line, request m_request) {
    unsigned long LEN = line.length();
    int substr_start = 0, i;
    for (i = 0; i < LEN-1; i++) {
        if (line[i] == '\r' && line[i+1] == '\n') {
            seperate_lines.push_back(line.substr(substr_start, i - substr_start));
            substr_start = i + 2;
            if (line[i+2] == '\r' && line[i+3] == '\n') // reach the end of header
                break;
        }
    }
    if (m_request == GET_STATUS_LENGTH)
        return int(seperate_lines[0].length());
    else return i + 4;
    
}



/* Group the header lines of HTTP MESSAGE according to ":" */
void HttpMessage::decodeHeaderLine() {
    unsigned long SIZE = seperate_lines.size();
    
    for (int i = 1; i < SIZE; i++) {
        string entry = seperate_lines[i];
        size_t found = entry.find(':');
        if (found == string::npos) {
            perror("wrong header format!");
            exit(1);
        }
        string key = entry.substr(0, found);
        string value = entry.substr(found + 2); // the rest of string goes to value
#ifdef _DEBUG
        cout << key << ": " << value << endl;
#endif
        m_headers.insert(pair<string, string>(key, value));
    }
    
    
}



/* Decode the first line of HTTP REQUEST */
void HttpRequest::decodeFirstLine(ByteBlob line) {
    
    int lengthOfStatus = h_groupLines(line, GET_STATUS_LENGTH);
    
    string firstline = line.substr(0, lengthOfStatus);
    
    vector<string> status = explode(firstline);
    if (status.size() != 3) {       // method, URL, version
        valid = false;
        return;
    }
    
    setMethod(status[0]);
    setPath(status[1]);
    setVersion(status[2]);
    
#ifdef _DEBUG
    for (int i = 0; i < 3; i++)
        cout << status[i] << endl;
#endif
}



/* Decode the first line of HTTP RESPONSE */
void HttpResponse::decodeFirstLine(ByteBlob line) {
    
    int lengthOfStatus = h_groupLines(line, GET_STATUS_LENGTH);
    
    // parse the status line into an array of strings
    int substr_start = 0, count = 0;
    
    // version, status code, status description
    string status[3];
    
    for (int i = substr_start; i < lengthOfStatus; i++) {
        if (line[i] == ' ') {
            string temp = line.substr(substr_start, i-substr_start);
            status[count++] = temp;
            substr_start = i + 1;
            if (count == 2) break;  // since description may spaces, keep everything after the second space
        }
    }
    status[2] = line.substr(substr_start, lengthOfStatus-substr_start);
    setVersion(status[0]);
    setStatus(status[1]);
    setDescription(status[2]);
    
#ifdef _DEBUG
    for (int i = 0; i < 3; i++)
        cout << status[i] << endl;
#endif
}

/* helper function for split(), recursive */
vector <string> w_split(const string& str, const string& delimiter) {
    if (str.empty())
        return vector<string>(0);
    else {
        unsigned long pos = str.find(delimiter);
        string cur_req = str.substr(0, pos + delimiter.length());
        string rest = str.substr(pos + delimiter.length());
        vector<string> rest_req = w_split(rest);
        rest_req.push_back(cur_req);
        return rest_req;
    }
}

/* This function parses the pipelined requests into a vector of requests*/
vector<string> split(const string& str, const string& delimiter) {
    vector<string> temp = w_split(str, delimiter);
    reverse(temp.begin(), temp.end());
    return temp;
}



/* parseURL takes an absolute URL as an input and returns port number, path and hostname. */
URL parseURL(string url) {
    URL r_URL;
    string r_path, r_host, r_port;
    string http("http://");
    
    if (url.compare(0, http.size(), http) == 0) {
        unsigned long pos = url.find_first_of("/:", http.size());
        
        if (pos == string::npos) {
            // No port or path
            pos = url.size();
        }
        
        // extract host name
        r_host = url.substr(http.size(), pos - http.size());
        
        unsigned long ppos = url.find_first_of("/", pos);
        if (ppos == string::npos)
            ppos = url.size();
        if (pos < url.size() && url[pos] == ':') {
            // A port is provided
            
            if (ppos == string::npos) {
                // No path provided, assume port is the rest of string
                ppos = url.size();
            }
            r_port = url.substr(pos+1, ppos-pos-1);
        }
        r_path = url.substr(ppos);  // the rest is path
        
    } else {
        perror( "Not an HTTP url" );
        exit(2);
    }
    
    r_URL.path = r_path;
    r_URL.host = r_host;
    r_URL.port = r_port;
    return r_URL;
}

/* helper function for reponse parsing*/
vector<string> w_split_response(string response) {
    if (response == "")
        return vector<string>(0);
    else {
        
        const string key = "Content-Length";
        const string end = "\r\n\r\n";
        HttpResponse temp;
        temp.consume(response);
        
        int header_length = int(response.find(end));
        string temp_body = temp.getHeader(key);
        
        int body_len = stoi(temp_body);
        
        int total_len = header_length + body_len + int(end.length());
        
        string cur_response = response.substr(0, total_len);
        if (total_len == 0)
            return vector<string>(0);
        string rest_response = response.substr(total_len);
        
        vector<string> result = w_split_response(rest_response);
        result.push_back(cur_response);
        
        return result;
    }
}

/* input: string, output: vector<string>, each member is a response */
vector<string> split_response(string response) {
    vector<string> temp = w_split_response(response);
    reverse(temp.begin(), temp.end());
    return temp;
}


vector<string> explode(string const &input) {
    istringstream buffer(input);
    vector<std::string> ret;
    
    copy(istream_iterator<string>(buffer),
         (istream_iterator<string>()),
         back_inserter(ret));
    return ret;
}

#endif







