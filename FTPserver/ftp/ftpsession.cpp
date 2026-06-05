#include "ftpsession.h"
#include "../net/connection.h"
#include <sstream>
#include <iostream>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

#include "../net/channel.h"
#include "../net/acceptor.h"
#include "../net/eventloop.h"
#include "../net/epoll.h"
#include "../net/util.h"

#include <filesystem>

namespace fs = std::filesystem;
std::string normalize_path(const std::string& path) {
    fs::path p(path);
    fs::path abs = fs::weakly_canonical(p); // 或 fs::canonical，但需目录存在
    return abs.string();
}
Ftpsession::Ftpsession(Connection *conn):   alive(true),conn(conn),
      loggin(false),
        
      username(""),
      curdir("./"),
      loop(nullptr),
      data_ready(false),
       pasvchannel(nullptr), 
      pasvlistenfd(-1),
      datafd(-1),
      pasvport(0),        
      typemode('I')  
{
    initcommands();
}
Ftpsession::~Ftpsession()
{
    std::cout
    << "[PASV] member fd="
    << pasvlistenfd
    << std::endl;
}
void Ftpsession::initcommands()
{
    commands["USER"] = std::bind(&Ftpsession::handleUSER, this, std::placeholders::_1);
    commands["PASS"] = std::bind(&Ftpsession::handlePASS, this, std::placeholders::_1);
    commands["SYST"] = std::bind(&Ftpsession::handleSYST, this, std::placeholders::_1);
    commands["PWD"] = std::bind(&Ftpsession::handlePWD, this, std::placeholders::_1);
    commands["CWD"] = std::bind(&Ftpsession::handleCWD, this, std::placeholders::_1);
    commands["TYPE"] = std::bind(&Ftpsession::handleTYPE, this, std::placeholders::_1);
    commands["QUIT"] = std::bind(&Ftpsession::handleQUIT, this, std::placeholders::_1);
    commands["PASV"] = std::bind(&Ftpsession::handlePASV, this, std::placeholders::_1);
    commands["LIST"] = std::bind(&Ftpsession::handleLIST, this, std::placeholders::_1);
    commands["RETR"] = std::bind(&Ftpsession::handleRETR, this, std::placeholders::_1);
    commands["STOR"] = std::bind(&Ftpsession::handleSTOR, this, std::placeholders::_1);
}
// ftp协议，命令处理函数
void Ftpsession::oncommand(const std::string &cmdline)
{

    if (!alive)
    {
        std::cout << "[FTP] session dead, ignore cmd\n";
        return;
    }
    std::cout
        << "[FTP CMD] "
        << cmdline
        << std::endl;
    std::cout << "cmd:" << cmdline << std::endl;
    std::stringstream ss(cmdline);
    std::string cmd;
    ss >> cmd;
    for (auto &c : cmd)
    {
        c = toupper(c);
    }
    std::string arg;
    getline(ss, arg);
    while (!arg.empty() && arg[0] == ' ')
    {
        arg.erase(0, 1);
    }
    while (!arg.empty() && (arg.back() == '\r' || arg.back() == '\n'))
    {
        arg.pop_back();
    }
    std::cout << "Received command: " << cmd << " arg: " << arg << std::endl;
    auto it = commands.find(cmd);
    if (it != commands.end())
    {
        it->second(arg);
    }
    else
    {
        sendresponse("502 Command not implemented\r\n");
    }
}
void Ftpsession::sendresponse(const std::string &msg)
{
    conn->send(msg);
}
// 认证
void Ftpsession::handleUSER(const std::string &arg)
{
    username = arg;
    sendresponse("331 User name okay, need password\r\n");
}

void Ftpsession::handlePASS(const std::string &arg)
{
    std::string clean = arg;

    while (!clean.empty() &&
           (clean.back() == '\r' || clean.back() == '\n' || clean.back() == ' '))
    {
        clean.pop_back();
    }

    std::cout << "[DEBUG PASS] [" << clean << "]" << std::endl;

    if (clean == "123456")
    {
        loggin = true;
        sendresponse("230 User logged in, proceed\r\n");
    }
    else
    {
        sendresponse("530 Not logged in\r\n");
    }
}
void Ftpsession::handleSYST(const std::string &arg)
{
    sendresponse("215 UNIX Type: L8\r\n"); // 返回系统类型
}
void Ftpsession::handlePWD(const std::string &arg)
{
    std::string response =
        "257 \"" + curdir + "\" is current directory\r\n";

    sendresponse(response);
}

void Ftpsession::handleTYPE(const std::string &arg)
{
    if (arg == "A" || arg == "a")
    {
        typemode = 'A';      
        sendresponse("200 Type set to A\r\n");
    }
    else if (arg == "I" || arg == "i" || arg.empty())
    {
        typemode = 'I';       
        sendresponse("200 Type set to I\r\n");
    }
    else
    {
        sendresponse("504 Unknown TYPE mode\r\n");
    }
}


void Ftpsession::handleQUIT(const std::string &arg)
{
    sendresponse("221 goodbye\r\n");
    alive = false;

}

void Ftpsession::handlePASV(const std::string &arg)
{
    std::cout << "PASV start" << std::endl;
  
    pasvlistenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    
    if (pasvlistenfd < 0)
{
    perror("socket");
    return;
}

std::cout << "[PASV] listen fd = " << pasvlistenfd << std::endl;

    int opt = 1;
    setsockopt(pasvlistenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;
    if( bind(pasvlistenfd, (sockaddr *)&addr, sizeof(addr))<0)
    {
        perror("bind");
        close(pasvlistenfd);
        pasvlistenfd = -1;
        return;
    }


    sockaddr_in localaddr{};
    socklen_t len = sizeof(localaddr);
    getsockname(pasvlistenfd, (sockaddr *)&localaddr, &len);
    pasvport = ntohs(localaddr.sin_port);

    std::cout << "[PASV] port=" << pasvport << std::endl;

     int flags = fcntl(pasvlistenfd, F_GETFL, 0);

    fcntl(pasvlistenfd, F_SETFL, flags | O_NONBLOCK);
     if(listen(pasvlistenfd, 1)<0)
    {
        perror("listen");
        close(pasvlistenfd);
        pasvlistenfd = -1;
        return;
    }
    std::cout
<< "[PASV] listen ok fd="
<< pasvlistenfd
<< std::endl;

std::cout
    << "[PASV] waiting connect, fd="
    << pasvlistenfd
    << " port="
    << pasvport
    << std::endl;

    //
    pasvchannel = new Channel(pasvlistenfd);
   // pasvchannel->setreadcallback(std::bind(&Ftpsession::acceptdataconnection, this));
        
    pasvchannel->setreadcallback([this]() {

    sockaddr_in cli;
    socklen_t len = sizeof(cli);

    int connfd = accept(pasvlistenfd, (sockaddr*)&cli, &len);

    if (connfd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        perror("accept");
        return;
    }

    setnonblock(connfd);
    datafd = connfd;

    std::cout << "[PASV] accepted data fd = " << connfd << std::endl;

    
    close(pasvlistenfd);
    pasvlistenfd = -1;

});
    pasvchannel->events = EPOLLIN ;

    loop->addchannel(pasvchannel);

    int p1 = pasvport / 256;

    int p2 = pasvport % 256;

    std::string resp =
        "227 Entering Passive Mode " "(127,0,0,1," +
        std::to_string(p1) + "," + std::to_string(p2) + ")\r\n";
    //
    sendresponse(resp);
    std::cout
    << "[PASV] member fd="
    << pasvlistenfd
    << std::endl;
}
bool Ftpsession::acceptdataconnection()
{
    sockaddr_in clientaddr{};

    socklen_t len = sizeof(clientaddr);

    datafd = accept(  pasvlistenfd,(sockaddr *)&clientaddr,&len);

    if (datafd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            std::cout  << "no data connection yet"<< std::endl;
            return false;
        }

        perror("accept");

        sendresponse(
            "425 can't open data connection\r\n");

        return false;
    }

    int flags = fcntl(datafd, F_GETFL, 0);

    fcntl( datafd,F_SETFL,flags & ~O_NONBLOCK );

    std::cout<< "data connected" << std::endl;
    return true;
}

void Ftpsession::closedataconnection()
{
    if (datafd >= 0)
    {
        close(datafd);
        datafd = -1;
    }

}
void Ftpsession::closepasv()
{
    if (pasvlistenfd >= 0)
    {
        close(pasvlistenfd);
        pasvlistenfd = -1;
    }

    if (pasvchannel)
    {
        loop->removechannel(pasvchannel);
        delete pasvchannel;
        pasvchannel = nullptr;
    }
}
void Ftpsession::handleLIST(const std::string &arg)
{
   
    if (!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return;
    }
     std::cout
    << "[LIST] datafd="
    << datafd
    << " pasvlistenfd="
    << pasvlistenfd
    << std::endl;
   
  
   
   if (datafd < 0)
  {
    sendresponse("425 No data connection\r\n");
    return;
  }
    sendresponse("150 opening data connection\r\n");

    DIR *dir = opendir(curdir.c_str());
    if (!dir)
    {
        sendresponse("550 failed to open directory\r\n");
        close(datafd);
        return;
    }
    struct dirent *entry;
    std::string result;
    while ((entry = readdir(dir)) != nullptr)
    {
        result += entry->d_name;
        result += "\r\n";
    }

    closedir(dir);
    send(datafd, result.c_str(), result.size(), 0);
    closedataconnection();

    sendresponse("226 transfer complete\r\n");
    close(datafd);
}
void Ftpsession::handleCWD(const std::string &arg)
{
    if (!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return;
    }

    fs::path newdir = fs::path(curdir) / arg;

    if (!fs::exists(newdir) || !fs::is_directory(newdir))
    {
        sendresponse("550 Directory does not exist\r\n");
        return;
    }

    curdir = fs::weakly_canonical(newdir).string();

    sendresponse("250 Directory changed to " + curdir + "\r\n");
}

void Ftpsession::handleRETR(const std::string &arg)
{
    if (!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return;
    }

    std::string path = curdir + "/" + arg;
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        sendresponse("550 Failed to open file\r\n");
        return;
    }
   

   
   if (datafd < 0)
   {
    sendresponse("425 No data connection\r\n");
    return;
   }
    sendresponse("150 opening data connection\r\n");


// 直接发送文件

    char buf[4096];
    while (true)
    {
        // 读文件数据
        file.read(buf, sizeof(buf));
        size_t n = file.gcount();

        // 读完
        if (n == 0)
            break;

        // 循环发送，保证所有数据都发出去
        size_t sent = 0;
        while (sent < n)
        {
            ssize_t ret = send(datafd, buf + sent, n - sent, 0);
            if (ret <= 0)
                goto endsend; // 发送失败直接结束
            sent += ret;
        }
    }

endsend:
    file.close();
    closedataconnection();
    sendresponse("226 transfer complete\r\n");
}

void Ftpsession::handleSTOR(const std::string &arg)
{
    if (!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return;
    }
    std::string path = curdir + "/" + arg;

    fs::path filepath(path);

   if(filepath.has_parent_path())
  {
    fs::create_directories(
        filepath.parent_path());
    }
    std::ofstream file(path, std::ios::binary); // 创建文件
    if (!file)
    {
        sendresponse("550 Failed to create file\r\n");
        return;
    }
   
    if (datafd < 0)
    {
    sendresponse("425 No data connection\r\n");
    return;
    }

  sendresponse("150 opening data connection\r\n");

// 直接recv
    char buf[4096];
    int n;
  
    while (true)
{
    n = recv(datafd, buf, sizeof(buf), 0);

    if (n > 0)
    {
        file.write(buf, n);
    }
    else if (n == 0)
    {
        break; // client close
    }
    else
    {
        if (errno == EAGAIN) continue;
        break;
    }
}
    file.close();
    closedataconnection();
    sendresponse("226 transfer complete\r\n");
}
