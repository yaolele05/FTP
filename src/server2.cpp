#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
int main()
{
    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd<0)
    {
        std::cerr<<"socket error"<<std::endl;
        return -1;
    }
    sockaddr_in serveraddr{};
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(8080);
    serveraddr.sin_addr.s_addr=INADDR_ANY;

    if(bind(listenfd,(sockaddr*)&serveraddr,sizeof(serveraddr))<0)
    {
        std::cerr<<"bind error"<<std::endl;
        return -1;
    }
    if(listen(listenfd,10)<0)
    {
        std::cerr<<"listen error"<<std::endl;
        return -1;
    }
    std::cout<<"Server is listening on port 8080..."<<std::endl;
    while(1)
    {

        //accept
        int connfd=accept(listenfd,nullptr,nullptr);
        if(connfd<0)
        {
            std::cout<<"accept error"<<std::endl;
            continue;

        }
        std::cout<<"client connected\n"<<std::endl;
        char buffer[1024]={0};

        int n=recv(connfd,buffer,sizeof(buffer),0);
        if(n<0)
        {
            std::cerr<<"recv error"<<std::endl;
            close(connfd);
            continue;
        }
        else
        {
            std::cout<<"recv:"<<buffer<<std::endl;
            send(connfd,buffer,n,0);
       
        }
             close(connfd);
    }
    return 0;
}