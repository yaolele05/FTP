
#pragma once
#include "channel.h"
#include "eventloop.h"
#include "../ftp/ftpsession.h"
#include <string>

 class Ftpsession;
class Connection
{
    private:
    int connfd;
    eventloop*loop;
    Channel*channel;
    Ftpsession* session;
    public:
    Connection(eventloop*loop,int fd);
    ~Connection();
      void handleread();
      void send(const std::string& msg);
      std::string inputbuffer;
      
      int fd() const
      {
        return connfd;
      }
 
    

};