#include "acceptor.h"
#include <unistd.h>
#include <cstring>

#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
//下面是Acceptor类的实现，负责监听端口并接受新连接
Acceptor::Acceptor(eventloop*loop,int port):loop(loop)
{
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    setnonblock(listenfd);//设置非阻塞
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    if(bind(listenfd,(sockaddr*)&addr,sizeof(addr))<0)
    {
        perror("bind error");
        exit(1);    
    }
    listen(listenfd,128);
    acceptchannel=new Channel(listenfd);
    acceptchannel->setreadcallback(std::bind(&Acceptor::handleread,this));
    acceptchannel->events=EPOLLIN|EPOLLET;//监听可读事件，边缘触发
      loop->addchannel(acceptchannel);
      std::cout<<"Acceptor is listening on port "<<port<<std::endl;
}
void Acceptor::setnonblock(int fd)
{
   int f=fcntl(fd,F_GETFL,0);
   fcntl(fd,F_SETFL,f|O_NONBLOCK);

}
void Acceptor::handleread()
{
    while(1)
    {
        sockaddr_in clientaddr;
        socklen_t len=sizeof(clientaddr);
        int connfd=accept(listenfd,(sockaddr*)&clientaddr,&len);
        if(connfd<0) 
        {
            if(errno==EAGAIN||errno==EWOULDBLOCK)
            {                break;//没有更多连接了，退出循环
            }
            else
            {                perror("accept error");
                break;
            }
        }

        setnonblock(connfd);
        std::cout<<"Accepted new connection, fd: "<<connfd<<std::endl;
    
        if(newconncallback)
        {            newconncallback(connfd);//调用回调函数处理新连接
        }
    }
}
void Acceptor::setnewconncallback(std::function<void(int)> cb)
{
    newconncallback = cb;
}
Acceptor::~Acceptor()
{
    close(listenfd);
    delete acceptchannel;
}