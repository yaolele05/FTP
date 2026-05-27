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
#include <vector>
#include <fstream>
Ftpsession::Ftpsession(Connection* conn): conn(conn),loggin(false),curdir("/"),pasvlistenfd(-1),datafd(-1),pasvport(0),typemode('I')
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
    commands["RETR"]=std::bind(&Ftpsession::handleRETR,this,std::placeholders::_1);
    commands["STOR"]=std::bind(&Ftpsession::handleSTOR,this,std::placeholders::_1);
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
//认证
void Ftpsession::handleUSER(const std::string& arg)
{
    username=arg;
        sendresponse("331 User name okay, need password\r\n");
    
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
    if (arg == "A" || arg == "a")
    {
        sendresponse("200 Type set to A \r\n");
    }
    else if (arg == "I" || arg == "i" || arg.empty())
    {
        sendresponse("200 Type set to I \r\n");
    }
    else
    {
        sendresponse("504 Unknown TYPE mode\r\n");
    }
}

void Ftpsession::handleQUIT(const std::string& arg)
{
    sendresponse("221 goodbye\r\n");
}


void Ftpsession::handlePASV(const std::string& arg)
{
    std::cout<<"PASV start"<<std::endl;
    closedataconnection();
    pasvlistenfd=socket(AF_INET,SOCK_STREAM,0);

int flags = fcntl(pasvlistenfd,F_GETFL, 0);

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
    sockaddr_in localaddr{};
    socklen_t len = sizeof(localaddr);
    getsockname(pasvlistenfd,(sockaddr*)&localaddr,&len);
    pasvport=ntohs(localaddr.sin_port);
    //
    int p1=pasvport/256;

    int p2=pasvport%256;

    std::string resp=
        "227 Entering Passive Mode " "(127,0,0,1," +std::to_string(p1)+"," +std::to_string(p2)
        +")\r\n";
   //
    sendresponse(resp);
}
bool Ftpsession::acceptdataconnection()
{
    sockaddr_in clientaddr{};

    socklen_t len = sizeof(clientaddr);

    datafd = accept(pasvlistenfd, (sockaddr*)&clientaddr, &len);

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
    int flags = fcntl( datafd, F_GETFL,0);//获取当前文件状态

    fcntl(datafd,F_SETFL,flags | O_NONBLOCK)  ;//设置非阻塞

    std::cout
        <<"data connected"
        <<std::endl;

    return true;
}
void Ftpsession::closedataconnection()
{
    if(datafd>=0)
    {
        close(datafd);
        datafd=-1;
    }
    if(pasvlistenfd>=0)
    {
        close(pasvlistenfd);
        pasvlistenfd=-1;
    }

}
void Ftpsession::handleLIST(const std::string& arg)
{
    if(!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return;
    }
    if(pasvlistenfd<0)
    {
        sendresponse("425 Use PASV first\r\n");
        return;
    }
    sendresponse("150 opening data connection\r\n");
   //尝试accept，等待数据连接建立
     if(!acceptdataconnection())
    {
        sendresponse("425 No data connection\r\n");
        return;
    }
    

    DIR* dir=opendir(curdir.c_str());
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
   
    closedir(dir);
    send(datafd,result.c_str(),result.size(),0);
    closedataconnection();
  
    sendresponse("226 transfer complete\r\n");
    
}
void::Ftpsession::handleCWD(const std::string& arg)
{
    if(!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return; 
    }
    std::string newdir=curdir+"/"+arg;
    if(access(newdir.c_str(),F_OK)!=0)
    {
        sendresponse("550 Directory does not exist\r\n");
        return;
    }
    else{
        curdir=newdir;
        sendresponse("250 Directory changed to "+curdir+"\r\n");///???
    }
}
void Ftpsession::handleRETR(const std::string& arg)
{
    if(!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return; 
    }
    std::string path=curdir+"/"+arg;
    std::ifstream file(path,std::ios::binary);//打开文件
    if(!file)
    {
        sendresponse("550 Failed to open file\r\n");
        return ;
    }
    sendresponse("150 opening data connection\r\n");
    if(!acceptdataconnection())
    {
        sendresponse("425 No data connection\r\n");
        return;
    }
    char buf[4096];
    while(file.read(buf,sizeof(buf)))
    {
        send(datafd,buf,file.gcount(),0);   
    }
    file.close();
    closedataconnection();
    sendresponse("226 transfer complete\r\n");
}
void Ftpsession::handleSTOR(const std::string& arg)
{
    if(!loggin)
    {
        sendresponse("530 Not logged in\r\n");
        return; 
    }
    std::string path=curdir+"/"+arg;
    std::ofstream file(path,std::ios::binary);//创建文件
    if(!file)
    {
        sendresponse("550 Failed to create file\r\n");
        return ;
    }
    sendresponse("150 opening data connection\r\n");
    if(!acceptdataconnection())
    {
        sendresponse("425 No data connection\r\n");
        return;
    }
    char buf[4096];
    while(true)
    {        int n=recv(datafd,buf,sizeof(buf),0);
        if(n>0)     
           {
            file.write(buf,n);
       }
   }
        file.close();
        closedataconnection();
        sendresponse("226 transfer complete\r\n");  
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