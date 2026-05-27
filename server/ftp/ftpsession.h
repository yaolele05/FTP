#pragma once

#include <string>
#include <unordered_map>
#include <functional>

class Connection;
class Ftpsession
{
    public:
    using Commandcallback = std::function<void(const std::string&)>;
    public:
        Ftpsession(Connection* conn);
        ~Ftpsession();

        void oncommand(const std::string& cmdline);
        void sendresponse(const std::string& msg);

    private:
    void initcommands();

    void handleUSER(const std::string& arg);

    void handlePASS(const std::string& arg);

    void handleSYST(const std::string& arg);

    void handlePWD(const std::string& arg);

    void handleTYPE(const std::string& arg);

    void handleQUIT(const std::string& arg);

    void handlePASV(const std::string& arg);

    void handleLIST(const std::string& arg);
    bool acceptdataconnection();
    private:
        Connection* conn;
        bool loggin;
        std::string username;
        std::string curdir;
        int pasvlistenfd;
        int datafd;
        int pasvport;
        std::unordered_map<std::string, Commandcallback> commands;//haxi 命令分发表

};

