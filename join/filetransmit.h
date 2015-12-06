#ifndef FILETRANSMIT_H
#define FILETRANSMIT_H

#include <sys/types.h>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string> 
#include <string.h> 
#include <ifaddrs.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using namespace std;

void wordCount(string input, unordered_map<string, int> &countMap);

void writeHashToFile(string filename, unordered_map<string, int> countMap);

string receivePutRequest(int sockfd, char* buf, uint32_t len, std::string& sender);

void putFile(int out_fd, std::string localfilename, std::string sdfsfilename, std::string& add, int port);

string receiveGetRequest(int sockfd, char* buf, uint32_t len, std::string& sender);

void getFile(int sock_fd, std::string sdfsfilename, std::string localfilename, char* buf, uint32_t len);

void replyGetRequest(int sockfd, string sdfsfilename); 

void deleteFile(int sock_fd, string sdfsfilename);

void receiveDeleteRequest(int sockfd); 

#endif