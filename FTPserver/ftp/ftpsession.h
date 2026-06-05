#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>
#include "../net/channel.h"
#include "../net/eventloop.h"
class Connection;
class eventloop;   
class Channel;

class Ftpsession
{
    public:
    using Commandcallback = std::function<void(const std::string&)>;
    public:
          bool alive;
        Ftpsession(Connection* conn);
        ~Ftpsession();

        void oncommand(const std::string& cmdline);
        void sendresponse(const std::string& msg);
     
        public:
    void setloop(eventloop* lo) { loop = lo; }  

    private:
    void initcommands();

    void handleUSER(const std::string& arg);

    void handlePASS(const std::string& arg);

    void handleSYST(const std::string& arg);

    void handlePWD(const std::string& arg);

    void handleCWD(const std::string& arg);

    void handleTYPE(const std::string& arg);

    void handleQUIT(const std::string& arg);

    void handlePASV(const std::string& arg);

    void handleLIST(const std::string& arg);

    void handleRETR(const std::string& arg);

    void handleSTOR(const std::string& arg);
    std::string normalpath(const std::string& path);
    
    bool acceptdataconnection();
    
    void closedataconnection();
    void closepasv();
    
    private:
        Connection* conn;
        bool loggin;
       
        std::string username;
        std::string curdir;
       
        eventloop* loop;
         bool data_ready;
        Channel* pasvchannel=nullptr;
        int pasvlistenfd=-1;
        int datafd;
        int pasvport;
        char typemode;
        std::unordered_map<std::string, Commandcallback> commands;//haxi 命令分发表

};

