#include "ftpsession.h"
#include "../net/connection.h"
#include <sstream>
#include <iostream>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include <dirent.h>
#include <sys/stat.h>
Ftpsession::Ftpsession(Connection* conn): conn(conn),loggin(false),curdir("/"),pasvlistenfd(-1),datafd(-1),pasvport(0)
{
    initcommands();
}
Ftpsession::~Ftpsession()
{

}
void Ftpsession::initcommands()
{
    commands["USER"]=std::bind(&Ftpsession::handleUSER,this,std::placeholders::_1);
    commands["PASS"]=std::bind(&Ftpsession::handlePASS,this,std::placeholders::_1);
    commands["SYST"]=std::bind(&Ftpsession::handleSYST,this,std::placeholders::_1);
    commands["PWD"]=std::bind(&Ftpsession::handlePWD,this,std::placeholders::_1);
    commands["TYPE"]=std::bind(&Ftpsession::handleTYPE,this,std::placeholders::_1);
    commands["QUIT"]=std::bind(&Ftpsession::handleQUIT,this,std::placeholders::_1);
    commands["PASV"]=std::bind(&Ftpsession::handlePASV,this,std::placeholders::_1);
    commands["LIST"]=std::bind(&Ftpsession::handleLIST,this,std::placeholders::_1);
}
//ftp协议入口函数，根据命令分发表调用相应的处理函数
void Ftpsession::oncommand(const std::string& cmdline)
{
    std::cout<<"cmd:"<<cmdline<<std::endl;
    std::stringstream ss(cmdline);
    std::string cmd;
    ss>>cmd;
     for(auto& c:cmd)
{
    c=toupper(c);
}
    std::string arg;
    getline(ss,arg);
    if(!arg.empty()&&arg[0]==' ')
    {
        arg.erase(0,1);
    }
    while(!arg.empty() && (arg.back()=='\r' ||arg.back()=='\n'))
 {
    arg.pop_back();
 }
    std::cout<<"Received command: "<<cmd<<" arg: "<<arg<<std::endl;
    auto it=commands.find(cmd);
    if(it!=commands.end())
    {
        it->second(arg);
    }
    else
    {
        sendresponse("502 Command not implemented\r\n");
    }
}
void Ftpsession::sendresponse(const std::string& msg)
{
    conn->send(msg);
}
void Ftpsession::handleUSER(const std::string& arg)
{
    if(arg=="test")
    {
        sendresponse("331 User name okay, need password\r\n");
    }
    else
    {
        sendresponse("530 Not logged in\r\n");
    }
}
void Ftpsession::handlePASS(const std::string& arg)
{
    if(arg=="123456")
    {
        loggin=true;
        sendresponse("230 User logged in, proceed\r\n");
    }
    else
    {
        sendresponse("530 Not logged in\r\n");
    }
}
void Ftpsession::handleSYST(const std::string& arg)
{
    sendresponse("215 UNIX Type: L8\r\n");//返回系统类型
}

void Ftpsession::handlePWD(const std::string& arg)
{
    std::string response="257 \""+curdir+"\" is current directory\r\n";
    sendresponse(response);
}
void Ftpsession::handleTYPE(const std::string& arg)
{
    sendresponse("200 Type set ok \r\n");
}
void Ftpsession::handleQUIT(const std::string& arg)
{
    sendresponse("221 goodbye\r\n");
}
void Ftpsession::handlePASV(const std::string& arg)
{
    std::cout<<"PASV start"<<std::endl;
    pasvlistenfd=socket(AF_INET,SOCK_STREAM,0);

int flags =
    fcntl(pasvlistenfd,F_GETFL, 0);

fcntl( pasvlistenfd,  F_SETFL,flags | O_NONBLOCK);
    int opt=1;
    setsockopt(pasvlistenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(0);
    bind(pasvlistenfd,(sockaddr*)&addr,sizeof(addr));
    listen(pasvlistenfd,1);
    sockaddr_in localaddr;
    socklen_t len = sizeof(localaddr);
    getsockname(pasvlistenfd,(sockaddr*)&localaddr,&len);
    pasvport=ntohs(localaddr.sin_port);
    //
    int p1=pasvport/256;

    int p2=pasvport%256;

    std::string resp=
        "227 Entering Passive Mode "
        "(127,0,0,1,"
        +std::to_string(p1)
        +","
        +std::to_string(p2)
        +")\r\n";
   //
    sendresponse(resp);
}
bool Ftpsession::acceptdataconnection()
{
    sockaddr_in clientaddr;

    socklen_t len = sizeof(clientaddr);

    datafd = accept(
        pasvlistenfd,
        (sockaddr*)&clientaddr,
        &len
    );

    if(datafd < 0)
    {
        //accept 改为非阻塞状态的时候
        if(errno == EAGAIN ||
           errno == EWOULDBLOCK)
        {
            std::cout
                <<"no data connection yet"
                <<std::endl;

            return false;
        }

        perror("accept");

        sendresponse(
            "425 can't open data connection\r\n"
        );

        return false;
    }
//datafd设置为非阻塞
    int flags = fcntl( datafd, F_GETFL,0);

    fcntl(        datafd,F_SETFL,flags | O_NONBLOCK)  ;

    std::cout
        <<"data connected"
        <<std::endl;

    return true;
}
void Ftpsession::handleLIST(const std::string& arg)
{
    if(pasvlistenfd<0)
    {
        sendresponse("425 Use PASV first\r\n");
        return;
    }
    sendresponse("150 opening data connection\r\n");
   /* //尝试accept，等待数据连接建立
     if(!acceptdataconnection())
    {
        sendresponse("425 No data connection\r\n");
        return;
    }
    sendresponse("425 Opening data connection\r\n");

    DIR* dir=opendir(".");
    if(!dir)
    {
        sendresponse("550 failed to open directory\r\n");
        close(datafd);
        return;
    }
    struct dirent* entry;
    std::string result;
    while((entry=readdir(dir))!=nullptr)
    {
        result+=entry->d_name;
        result+= "\r\n";

    }
    send(datafd,result.c_str(),result.size(),0);
    closedir(dir);
    close(datafd);
    datafd=-1;
    close(pasvlistenfd);
    pasvlistenfd=-1;
 */
    sendresponse("226 transfer complete\r\n");
     close(pasvlistenfd);
    pasvlistenfd = -1;
}
/*
bool Ftpsession::acceptdataconnection()
{
      std::cout<<"waiting data connection..."<<std::endl;
    sockaddr_in clientaddr;
    socklen_t len=sizeof(clientaddr);
   datafd=accept(pasvlistenfd,(sockaddr*)&clientaddr,&len);
   if(datafd<0)
   {
     sendresponse(
            "425 can't open data connection\r\n"
        );

        return false;
   }
     std::cout<<"accept returned"<<std::endl;
   return true;
}
void Ftpsession::handleLIST(const std::string& arg)
{


    if(pasvlistenfd < 0)
    {
        sendresponse("425 Use PASV first\r\n");
        return;
    }
        std::cout<<"data connected"<<std::endl;
    sendresponse("150 opening data connection\r\n");
    if(!acceptdataconnection())
    {
        return;
    }
    
        DIR* dir=opendir(".");

    if(!dir)
    {
        sendresponse(
            "550 failed to open directory\r\n"
        );

        close(datafd);

        return;
    }
    //发送目录内容
        struct dirent* entry;

    std::string result;
        while((entry=readdir(dir))!=nullptr )
    {
        result+=entry->d_name;

        result+="\r\n";
    }
        send(
        datafd,
        result.c_str(),
        result.size(),
        0
    );
        closedir(dir);
        
        close(datafd);

    datafd=-1;

    close(pasvlistenfd);

    pasvlistenfd=-1;
        sendresponse("226 transfer complete\r\n" );

}*/