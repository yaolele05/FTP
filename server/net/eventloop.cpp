#include "eventloop.h"
#include "channel.h"

#include <iostream>

eventloop::eventloop():epoll(),channels(){};//初始化列表
void eventloop::addchannel(Channel* ch)
{
   channels[ch->fd]=ch;
   epoll.addchannel(ch);
}

void eventloop::updatechannel(Channel* ch)
{
    epoll.modchannel(ch);
}
 
void eventloop::removechannel(Channel*ch)
{
   channels.erase(ch->fd);
   epoll.delchannel(ch);
}
void eventloop::loop()
{
  while(1)
  {
    std::vector<Channel*> activeChannels=epoll.poll(-1);
    for(auto ch:activeChannels)
    {
      ch->handleevent();
    }
  }
}