#pragma once

#include <iostream>

#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
class Ftpclient
{
    public:
    Ftpclient();
    ~Ftpclient();
    bool connectserver(const std::string& ip,int port);
    bool login(const std::string& user,const std::string& pass);
    bool upload(const std::string& localfile,const std::string& remotefile);
    bool download(const std::string& remotefile,const std::string& localfile);
    void list();
    void quit();
    void pwd();
    void cwd(const std::string& dir);
    void type(char t);
    void retr(const std::string& remotefile,const std::string& localfile);
    void stor(const std::string& localfile,const std::string& remotefile);

    private:
    int controlfd;
    int datafd;
   
    bool sendcommand(const std::string& cmd);
    std::string readresponse();
    bool pasv(std::string& ip,int& port);
    bool connectdatasocket(const std::string&ip,int port);;
    void closedatasocket();

};