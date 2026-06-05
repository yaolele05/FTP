#include "epoll.h"
#include "channel.h"

#include <unistd.h>
#include <cstring>
#include <iostream>

Epoll::Epoll()
{
    epfd=epoll_create1(0);
    if(epfd<0)
    {
        perror("epoll_create1 error");//perror
        exit(1);
    }
}
Epoll::~Epoll()
{
  close(epfd);
}
 void Epoll::addchannel(Channel*ch)
 {
    epoll_event ev;
   
    ev.events=ch->events;
    ev.data.ptr=ch;
    if(epoll_ctl(epfd,EPOLL_CTL_ADD,ch->fd,&ev)<0)
    perror("epoll_ctl add");

    std::cout << "[EPOLL] register PASV fd" << std::endl;
 }
 void Epoll::modchannel(Channel*ch)
 {
  epoll_event ev;
  memset(&ev,0,sizeof(ev));
  ev.events=ch->events;
  ev.data.ptr=ch;
  if(epoll_ctl(epfd,EPOLL_CTL_MOD,ch->fd,&ev)<0)
  perror("epoll_ctl mod");
   
 }
 void Epoll::delchannel(Channel*ch)
 {
   if(epoll_ctl(epfd,EPOLL_CTL_DEL,ch->fd,NULL)<0)
   perror("epoll_ctl del");
 }

std::vector<Channel*> Epoll::poll(int timeout)
{
   std::vector<epoll_event> events(1024);
    int nfds=epoll_wait(
        epfd,
        events.data(),
        static_cast<int>(events.size()),
        timeout
    );
if(nfds<0)  perror("epoll_wait error");
     std::vector<Channel*> activeChannels;

    for(int i=0;i<nfds;i++)
    {
        Channel* ch=
            static_cast<Channel*>(events[i].data.ptr);

        ch->revents=events[i].events;

        activeChannels.push_back(ch);
    }
     
    return activeChannels;
}