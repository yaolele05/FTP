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
     session=new Ftpsession(this);
     session-> setloop(this->loop);
     send("220 simple ftp server\r\n");
};

Connection::~Connection()
{
    
      loop->removechannel(channel);
    close(connfd);
    delete channel;
    delete session;  
  
}
void Connection::send(const std::string& msg)
 {
    ssize_t n = ::send(connfd, msg.c_str(), msg.size(), 0);
    if (n < 0 && errno != EAGAIN) {
        std::cerr << "send error" << std::endl;}
 }

void Connection::handleread()
{
    char buf[4096];

    while(true)
    {
        int n = recv(  connfd, buf,sizeof(buf),0);

        if(n > 0)
        {
            inputbuffer.append(buf,n);
        }
        else if(n == 0)
        {
            std::cout<<"client close"<<std::endl;
            session->alive = false;

            close(connfd);
            connfd = -1;

             

            return;
        }
        else
        {
            if(errno == EAGAIN ||  errno == EWOULDBLOCK)
            {
                break;
            }

            perror("recv");

            close(connfd);
            connfd = -1;
            return;
        }
    }

    
    while(true)
    {
        size_t pos = inputbuffer.find("\r\n");

        if(pos == std::string::npos)
        {
            break;
        }

        std::string cmdline =
            inputbuffer.substr(0,pos);

        inputbuffer.erase(0,pos+2);

        std::cout <<"cmdline:"<<cmdline<<std::endl;

        session->oncommand(cmdline);
    }
}
