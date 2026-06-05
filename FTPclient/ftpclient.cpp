#include "ftpclient.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include  <fstream>
#include <filesystem>
#include <cerrno>

#include <sys/stat.h>

Ftpclient::Ftpclient():controlfd(-1)
{

}
Ftpclient::~Ftpclient()
{
    if(controlfd!=-1)//如果控制连接还存在，关闭它
    {
        close(controlfd);
    }
}
bool Ftpclient::login(const std::string& user,const std::string& pass)
{
    sendcommand("USER " + user);
    std::string resp = readresponse();
    std::cout << "USER resp: " << resp << std::endl;

    resp.erase(0, resp.find_first_not_of(" \r\n\t"));

    if(resp.substr(0,3) != "331")
    {
        std::cerr << "Unexpected response after USER command!" << std::endl;
        return false;
    }

    sendcommand("PASS " + pass);
    resp = readresponse();
    std::cout << "PASS resp: " << resp << std::endl;

    resp.erase(0, resp.find_first_not_of(" \r\n\t"));

    if(resp.substr(0,3) != "230")
    {
        std::cerr << "Unexpected response after PASS command!" << std::endl;
        return false;
    }

    return true;
}

bool Ftpclient::connectserver(const std::string& ip,int port)
{
    controlfd=socket(AF_INET,SOCK_STREAM,0);
    if(controlfd<0)
    {
        perror("socket");
        return false;
    }
    sockaddr_in clientaddr;
    memset(&clientaddr,0,sizeof(clientaddr));
    clientaddr.sin_family=AF_INET;
    clientaddr.sin_port=htons(port);
    if(inet_pton(AF_INET,ip.c_str(),&clientaddr.sin_addr)<=0)
    {
        perror("inet_pton");
        close(controlfd);
        controlfd=-1;
        return false;
    }
    if(connect(controlfd,(sockaddr*)&clientaddr,sizeof(clientaddr))<0)
    {
        perror("connect");
        close(controlfd);
        controlfd=-1;
        return false;
    }
    std::string resp=readresponse();
    std::cout<<"Server response: "<<resp<<std::endl;
    return true;
}
std::string Ftpclient::readresponse()
{
    std::string resp;
    char c;
    
    while (true)
    {
        int n = recv(controlfd, &c, 1, 0);
        if (n <= 0)
        {
            perror("recv");
            return "";
        }

        resp += c;

        
        if (resp.size() >= 2 &&
            resp[resp.size()-2] == '\r' &&
            resp[resp.size()-1] == '\n')
        {
            break;
        }
    }

    return resp;
}
/*
std::string Ftpclient::readresponse()
{
   char buf[4096];
   memset(buf,0,sizeof(buf));
   int n=recv(controlfd,buf,sizeof(buf)-1,0);
    if(n<=0)
    {
        perror("recv");
        return "";
    }
    buf[n]='\0';
    return std::string(buf);
}*/
bool Ftpclient::sendcommand(const std::string& cmd)
{
    std::string fullcmd=cmd+"\r\n";
    if(send(controlfd,fullcmd.c_str(),fullcmd.size(),0)<0)
    {
        perror("send");
        return false;
    }
    return true;
}

bool Ftpclient::pasv(std::string& ip, int& port)
{
    sendcommand("PASV");

    std::string resp = readresponse();

    std::cout << "[RAW PASV RESP]\n" << resp << std::endl;

    size_t code_pos = resp.find("227");
    if (code_pos == std::string::npos)
    {
        std::cerr << "PASV failed: no 227 reply" << std::endl;
        return false;
    }


    size_t l = resp.find('(');
    size_t r = resp.find(')');

    if (l == std::string::npos || r == std::string::npos || r <= l)
    {
        std::cerr << "PASV failed: no ( ) found" << std::endl;
        return false;
    }

    std::string data = resp.substr(l + 1, r - l - 1);

    std::cout << "[PASV RAW DATA] " << data << std::endl;

  
    int h1, h2, h3, h4, p1, p2;

    if (sscanf(data.c_str(),
               "%d,%d,%d,%d,%d,%d",
               &h1, &h2, &h3, &h4, &p1, &p2) != 6)
    {
        std::cerr << "PASV parse failed (sscanf)" << std::endl;
        return false;
    }


    ip = std::to_string(h1) + "." +
         std::to_string(h2) + "." +
         std::to_string(h3) + "." +
         std::to_string(h4);

    port = p1 * 256 + p2;

    std::cout << "[PASV PARSED] "
              << ip << ":" << port << std::endl;

    return true;
}
bool Ftpclient::connectdatasocket(const std::string&ip,int port)
{
    std::cout << "[DEBUG] connect data socket" << std::endl;
    datafd=socket(AF_INET,SOCK_STREAM,0);
    if(datafd<0)
    {
        perror("socket");
        return false;
    }

    sockaddr_in dataaddr;
    memset(&dataaddr,0,sizeof(dataaddr));

    dataaddr.sin_family=AF_INET;
    dataaddr.sin_port=htons(port);
    
    if(inet_pton(AF_INET,ip.c_str(),&dataaddr.sin_addr)<=0)
    {
        perror("inet_pton");
        close(datafd);
        datafd=-1;
        return false;
    }
    if(connect(datafd,(sockaddr*)&dataaddr,sizeof(dataaddr))<0)
    {
        perror("connect");
        close(datafd);
        datafd=-1;
        return false;
    }
    return true;
}
void Ftpclient::list()
{
    std::string ip;
    int port;
    if (!pasv(ip, port))
    {
        std::cerr << "PASV failed" << std::endl;
        return;
    }
    if (!connectdatasocket(ip, port))
    {
        std::cerr << "connect data failed" << std::endl;
        return;
    }
    if (!sendcommand("LIST"))
    {
        std::cerr << "send LIST failed" << std::endl;
        closedatasocket();
        return;
    }

    std::string resp = readresponse();
    std::cout << "[CONTROL] " << resp << std::endl;

    if (resp.rfind("150", 0) != 0)
    {
        std::cerr << "LIST failed (no 150)" << std::endl;
        closedatasocket();
        return;
    }

    char buf[4096];

    while (true)
    {
        int n = recv(datafd, buf, sizeof(buf), 0);

        if (n > 0)
        {
            std::cout.write(buf, n);
        }
        else if (n == 0)
        {
            
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }

    std::cout << std::endl;

    closedatasocket();

   
    resp = readresponse();
    std::cout << "[CONTROL] " << resp << std::endl;
}

void Ftpclient::closedatasocket()
{
    if (datafd >= 0)
    {
        close(datafd);
        datafd = -1;
    }
}

void Ftpclient::pwd()
{
    sendcommand("PWD");
    std::string resp=readresponse();
    std::cout<<"Server response: "<<resp<<std::endl;
}

void Ftpclient::type(char t)
{
    std::string cmd="TYPE ";
    cmd+=t;
    sendcommand(cmd);
    std::string resp=readresponse();
    std::cout<<"Server response: "<<resp<<std::endl;
}
void Ftpclient::cwd(const std::string& dir)
{
    sendcommand("CWD "+dir);
    std::string resp=readresponse();
    std::cout<<"Server response: "<<resp<<std::endl;
}
static bool isdir(const std::string& path)
{
   if(path.empty())
   {
    return false;
   }
   struct stat st;
   if(stat(path.c_str(),&st)==0)
    {
     return S_ISDIR(st.st_mode);
    }
    return path.back()=='/';
}
static std::string repath(const std::string& remotep,const std::string& localp)
{
    if(localp.empty())
    {
        return remotep;

    }
    if(localp=="."||localp=="./")
    {
        return remotep;
    }
    if(isdir(localp))
    {
        std::string dir=localp;
        if(dir.back()!='/')
        {
            dir+='/';
        }
        return dir+remotep;
    }
    return localp;
}
void Ftpclient::retr(const std::string& remotefile,const std::string& localfile)
{
    std::cout << "[DEBUG] remote=" << remotefile
          << " local=" << localfile << std::endl;
    std::string ip;
    int port;
    if(!pasv(ip,port))
    {
        std::cerr<<"PASV failed"<<std::endl;
        return;
    }
    if(!connectdatasocket(ip,port))
    {
        std::cerr<<"connect data failed"<<std::endl;
        return;
    }

      namespace fs=std::filesystem;
    fs::path p=(localfile);
    if(p.has_parent_path())
    {
        fs::create_directories(p.parent_path());
    }
     std::string finalPath = repath(remotefile, localfile);

std::cout << "[DEBUG] finalPath=" << finalPath << std::endl;
    std::ofstream file(finalPath,std::ios::binary);
    
    if(!file)
  {
    std::cerr
        <<"open file failed: "
        <<localfile
        <<std::endl;
    file.close();
    closedatasocket();
    return;
  }
    
    if(!sendcommand("RETR "+remotefile))
    {   closedatasocket();
    return;
   }  

    std::string resp=readresponse();
    std::cout<<"Server response: "<<resp<<std::endl;
    if(resp.substr(0,3)!="150")
    {
        std::cerr<<"RETR failed"<<std::endl;
        closedatasocket();
        return;
    }

 
   
    char buf[4096];
     bool transferok=true;
    while( true)
    {
       
        int n=recv(datafd,buf,sizeof(buf),0);
        if(n<0)
        {
            transferok=false;
            perror("recv");
            break;
        }
        else if(n==0)
        {
            break;
        }
        file.write(buf, n);

      if(!file)
     {
    std::cerr<<"write file failed"<<std::endl;
    transferok=false;
    break;
     }

 }
    file.close();
    closedatasocket();
   
    if(transferok)
    {
    resp=readresponse();
 
    std::cout<<"Server response: "<<resp<<std::endl;
    if(resp.substr(0,3)!="226")
    {
        std::cerr<<"RETR transfer failed"<<std::endl;
    }
    }
}
void Ftpclient::stor(const std::string& localfile,const std::string& remotefile)
{
    char bufer[256];
std::cout << "cwd = " << getcwd(bufer, sizeof(bufer)) << std::endl;
    std::string ip;
    int port;
    if(!pasv(ip,port))
    {
        std::cerr<<"PASV failed"<<std::endl;
        return; 
    }
    if(!connectdatasocket(ip,port))
    {
        std::cerr<<"connect data failed"<<std::endl;
        return;
    }
    std::ifstream infile(localfile, std::ios::binary);
if(!infile.is_open())
{
    std::cerr << "open file failed: [" << localfile << "] "
              << strerror(errno) << std::endl;
    closedatasocket();
    return;
}
    std::cout << "[DEBUG] send STOR: " << remotefile << std::endl;
    if(!sendcommand("STOR "+remotefile))
    {
        infile.close();
        closedatasocket();
        return;
    }
    std::string resp=readresponse();
    
    if(resp.substr(0,3)!="150")
    {
        std::cerr<<"STOR failed"<<std::endl;
        infile.close();
        closedatasocket();
        return;
    }
    std::cout<<"Server response: "<<resp<<std::endl;

    char buf[4096];
    bool transferok=true;
    while(infile)
    {
        infile.read(buf,sizeof(buf));
        std::streamsize n=infile.gcount();
        if(n>0)
        {
           send(datafd,buf,n,0);
        }

    }
    infile.close();
    closedatasocket();
    if(transferok)
    {
        resp=readresponse();
        std::cout<<"Server response: "<<resp<<std::endl;
        if(resp.substr(0,3)!="226")
        {
            std::cerr<<"STOR transfer failed"<<std::endl;
        }
    }
}
void Ftpclient::quit()
{
    sendcommand("QUIT");
    std::string resp=readresponse();
    std::cout<<"Server response: "<<resp<<std::endl;
    close(controlfd);
    controlfd=-1;
}