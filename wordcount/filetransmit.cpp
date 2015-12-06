#include <iostream>
#include <fstream>
#include <string> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>

#include "filetransmit.h"
#include "constant.h"


void wordCount(string input, unordered_map<string, int> &countMap)
{
    istringstream chunk(input);
    string line;
    while(getline(chunk, line))
    {
        for(int i = 0; i < line.size(); i ++)
        {
            if(!isdigit(line[i]) && line[i]!=':')
            {
                line = line.substr(i);
                break;
            }
        }
        string buf; // Have a buffer string
        stringstream ss(line); // Insert the string into a stream

        vector<string> tokens; // Create vector to hold our words
        while (ss >> buf)
            tokens.push_back(buf);

        for(int i = 0; i < tokens.size(); i ++)
        {
            if(!countMap.count(tokens[i]))
            {
                countMap[tokens[i]]=1;
            }
            else
                countMap[tokens[i]]++;
        }
    }

}

void writeHashToFile(string filename, unordered_map<string, int> countMap)
{
    ofstream myfile (filename);
    if (myfile.is_open())
    {
        for(auto it = countMap.begin(); it!=countMap.end(); it++)
        {
            myfile << it->first << " " << it->second << endl;
        }
        myfile.close();
    }
    else 
        cout << "Unable to open file";
}

/*
    Dealing with Put file and its request
*/
string receivePutRequest(int sockfd, char* buf, uint32_t len, std::string& sender)
{
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;
    cout<<"receiving"<<endl;
    int byte_count = 0;
    bzero(buf, len);
    FILE * filew;
    //filew=fopen("acopy.txt","wb");
    int numw = 0;
    bool findFileName = false;
    string createFileName = "";
    unordered_map<string, int> countMap;

    while ((byte_count = recvfrom(sockfd, buf, len, 0, &addr, &fromlen))!=0)
    {
        if(!findFileName)
        {
            string temp(buf);
            for(int i = 0; i < temp.size(); i ++)
            {
                if(temp[i]==':') 
                {
                    findFileName = true;
                    if(createFileName=="file_location_log.txt")
                    {
                        filew=fopen(createFileName.c_str(),"wb");
                        printf("first write %s\n", buf+i+1);
                        fwrite(buf+i+1,1,byte_count-i-1,filew);
                        break;
                    }
                    else
                    {
                        wordCount(string(buf+i+1), countMap);
                        break;
                    }

                }
                else
                {
                    createFileName+=temp[i];
                }
            }
        }
        else
        {
            if(createFileName=="file_location_log.txt")
            {
                fwrite(buf,1,byte_count,filew);
            }
            else
                wordCount(string(buf), countMap);
        }
        bzero(buf, len);
    }

    if(createFileName=="file_location_log.txt")
        fclose(filew);

    if (byte_count == -1)
    {
        printf("ERROR RECEIVING!!!\n");
        exit(-1);
    }

    struct sockaddr_in *sin = (struct sockaddr_in *) &addr;

    sender = inet_ntoa(sin->sin_addr);
    if(createFileName!="file_location_log.txt")
        writeHashToFile(createFileName+"Result", countMap);
    
    return createFileName;
}

void putFile(int out_fd, std::string localfilename, std::string sdfsfilename, std::string& add, int port)
{
    struct sockaddr_in servaddr,cliaddr;
    struct hostent *server;
    struct stat stat_buf;      /* argument to fstat */
    int rc;                    /* holds return code of system calls */

    server = gethostbyname(add.c_str());

    if(server == NULL)
    {
        std::cout << "Host does not exist" << std::endl;
        exit(1);
    }

    memset((char *) &servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    memcpy((char *) &servaddr.sin_addr.s_addr,(char *) server -> h_addr, server -> h_length);
    servaddr.sin_port = htons(port);


    sdfsfilename+=":";
    int filename_len = write(out_fd,sdfsfilename.c_str(), strlen(sdfsfilename.c_str()));
    if(filename_len<0) printf("Error: sending filename\n");

    /* open the file to be sent */
    int fd = open(localfilename.c_str(), O_RDONLY);
    if (fd == -1) {
      fprintf(stderr, "unable to open '%s': %s\n", localfilename.c_str(), strerror(errno));
      exit(1);
    }
    /* get the size of the file to be sent */
    fstat(fd, &stat_buf);
    /* copy file using sendfile */
    off_t offset = 0;
    rc = sendfile (out_fd, fd, &offset, stat_buf.st_size);
    close(out_fd);
    close(fd);
    
    if (rc == -1) {

      fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
      exit(1);
    }

    if (rc != stat_buf.st_size) {
      fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n",
              rc,
              (int)stat_buf.st_size);
      exit(1);
    }
}

//--------------------------------------------------------------------------------------------------------
/*
    Dealing with Get file and its request
*/
string receiveGetRequest(int sockfd, char* buf, uint32_t len, std::string& sender)
{
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;
    cout<<"receiving get"<<endl;
    int byte_count = 0;
    bzero(buf, len);
    string sdfsfilename = "";
    bool findFileName = false;
    while (!findFileName && (byte_count = recvfrom(sockfd, buf, len, 0, &addr, &fromlen))!=0)
    {
        printf("get file %s\n", buf);
        string temp(buf);
        for(int i = 0; i < temp.size(); i ++)
        {
            if(temp[i]==':')
            {
                findFileName = true;
            }
            else
            {
                sdfsfilename+=temp[i];
            }
        }
        bzero(buf, len);
    }
    if (byte_count == -1)
    {
        printf("ERROR RECEIVING!!!\n");
        exit(-1);
    }

    struct sockaddr_in *sin = (struct sockaddr_in *) &addr;

    sender = inet_ntoa(sin->sin_addr);

    return sdfsfilename;
}

void getFile(int sock_fd, std::string sdfsfilename, std::string localfilename, char* buf, uint32_t len)
{
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;
    int byte_count = 0;
    sdfsfilename+=":";
    bzero(buf, len);
    int filename_len = write(sock_fd,sdfsfilename.c_str(), strlen(sdfsfilename.c_str()));

    if(filename_len<0) 
        printf("Error: sending filename\n");

    
    FILE *filew;
    bool isRead = true;
    unordered_map<string, int> countMap;
    while ((byte_count = recvfrom(sock_fd, buf, len, 0, &addr, &fromlen))!=0)
    {
        if(isRead)
        {
            filew = fopen(localfilename.c_str(), "wb");
            isRead = false;
        }

        if(sdfsfilename=="file_location_log.txt")
        {
            fwrite(buf,1,byte_count,filew);
        }
        else
            wordCount(string(buf), countMap);

        //fwrite(buf,1,byte_count,filew);
        bzero(buf, len);
    }
    cout<<"FINIGHING GETTING REQUEST"<<endl;

    if(sdfsfilename!="file_location_log.txt")
        writeHashToFile(sdfsfilename.substr(0, sdfsfilename.size()-1) +"Result", countMap);


    close(sock_fd);
    if(!isRead)
        fclose(filew);
}

void replyGetRequest(int sockfd, string sdfsfilename)
{

    struct stat file_stat;
    if(stat (sdfsfilename.c_str(), &file_stat) != 0)
    {
        printf("FILE %s does not exist\n", sdfsfilename.c_str());
        close(sockfd);
        return;
    }

    int fd = open(sdfsfilename.c_str(), O_RDONLY);
    if(fd==-1)
    {
        printf("FILE %s does not exist\n", sdfsfilename.c_str());

    }

    struct stat stat_buf;      /* argument to fstat */
    int rc;                    /* holds return code of system calls */
    /* get the size of the file to be sent */
    fstat(fd, &stat_buf);
    /* copy file using sendfile */
    off_t offset = 0;
    rc = sendfile (sockfd, fd, &offset, stat_buf.st_size);
    close(fd);
    close(sockfd);

}

void deleteFile(int sock_fd, std::string sdfsfilename)
{
    int filename_len = write(sock_fd,sdfsfilename.c_str(), strlen(sdfsfilename.c_str()));

    if(filename_len<0) 
        printf("Error: sending filename\n");

    close(sock_fd);
}

void receiveDeleteRequest(int sockfd)
{
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;
    cout<<"receiving get"<<endl;
    int byte_count = 0;
    char buf[1024];
    bzero(buf, 1024);
    bool findFileName = false;
    while ((byte_count = recv(sockfd, buf, sizeof(buf), 0))!=0)
    {
        
    }
    string sdfsfilename(buf); 
    cout<<sdfsfilename<<endl;
    if (byte_count == -1)
    {
        printf("ERROR RECEIVING!!!\n");
        exit(-1);
    }

    remove(sdfsfilename.c_str());
}