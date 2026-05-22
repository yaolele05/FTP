#include "eventloop.h"
#include "channel.h"

#include <iostream>

eventloop::eventloop():epoll(),channels(){};//初始化列表

void eventloop::addchannel(Channel*ch)
{
  channels[ch->getfd()]=ch;
  epoll.addchannel(ch);
}