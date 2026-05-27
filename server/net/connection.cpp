#include "connection.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include "../ftp/ftpsession.h"
Connection::Connection(eventloop*loop,int fd):connfd(fd),loop(loop)
{
    channel=new Channel(fd);
    channel->events=EPOLLIN;
    channel->setreadcallback(std::bind(&Connection::handleread,this));//绑定读事件回调函数
     
       
     loop->addchannel(channel);//将通道添加到事件循环
     session=new Ftpsession(this);//
     send("220 simple ftp server\r\n");
};

Connection::~Connection()
{
    //????loop->removechannel(channel);//从事件循环中移除通道
      
  
}
void Connection::send(const std::string& msg)
 {
    ssize_t n = ::send(connfd, msg.c_str(), msg.size(), 0);
    if (n < 0 && errno != EAGAIN) {
        std::cerr << "send error" << std::endl;}
    }
/*
void Connection::send(  const std::string& msg)
{
    ::send(
        connfd,
        msg.c_str(),
        msg.size(),
        0
    );
}*/
void Connection::handleread()
{
    char buf[4096];

    while(true)
    {
        int n = recv(
            connfd,
            buf,
            sizeof(buf),
            0
        );

        if(n > 0)
        {
            inputbuffer.append(buf,n);
        }
        else if(n == 0)
        {
            std::cout<<"client close"<<std::endl;

            loop->removechannel(channel);

            close(connfd);

            return;
        }
        else
        {
            if(errno == EAGAIN ||
               errno == EWOULDBLOCK)
            {
                break;
            }

            perror("recv");

            loop->removechannel(channel);

            close(connfd);

            return;
        }
    }

    // command parser
    while(true)
    {
        size_t pos =
            inputbuffer.find("\r\n");

        if(pos == std::string::npos)
        {
            break;
        }

        std::string cmdline =
            inputbuffer.substr(0,pos);

        inputbuffer.erase(0,pos+2);

        std::cout
            <<"cmdline:"
            <<cmdline
            <<std::endl;

        session->oncommand(cmdline);
    }
}
/*
void Connection::handleread()
{
    std::cout
        <<"Connection::handleread"
        <<std::endl;

    char buf[4096];

    while(true)
    {
        memset(buf,0,sizeof(buf));

        int n = recv(
            connfd,
            buf,
            sizeof(buf)-1,
            0
        );

        // 成功读取数据
        if(n > 0)
        {
            std::string cmdline(buf,n);

            std::cout
                <<"recv data: "
                <<cmdline
                <<std::endl;

            session->oncommand(cmdline);
        }

        // 客户端关闭连接
        else if(n == 0)
        {
            std::cout
                <<"client close"
                <<std::endl;

            loop->removechannel(channel);

            close(connfd);

            return;
        }

        // 出错
        else
        {
            // ET模式核心：
            // 数据已经读完
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout
                    <<"read complete"
                    <<std::endl;

                break;
            }

            // 真正错误
            perror("recv");

            loop->removechannel(channel);

            close(connfd);

            return;
        }
    }
}*/
/*
void Connection::handleread()
{
    std::cout
    <<"Connection::handleread"
    <<std::endl;
    char buf[4096]={0};

    int n=recv(
        connfd,
        buf,
        sizeof(buf)-1,
        0
    );

    if(n<=0)
    {
        std::cout<<"client close"<<std::endl;

        loop->removechannel(channel);

        close(connfd);

        return;
    }

    std::string cmdline(buf,n);

    session->oncommand(cmdline);
}*/
/*
void Connection::handleread()
{
    char buf[1024]={0};
    int n=recv(connfd,buf,sizeof(buf),0);
    if(n<=0)
    {
        std::cout<<"Connection closed by peer"<<std::endl;
        //delete this;????//删除当前连接对象，触发析构函数，关闭连接
        close (connfd); //关闭连接文件描述符
        return;

    }
    else
    {
        std::cout<<"Received data: "<<buf<<std::endl;
        //这里可以添加处理数据的逻辑，例如解析请求、生成响应等
    }
}*/