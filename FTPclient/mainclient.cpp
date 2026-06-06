#include <iostream>
#include "ftpclient.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cerrno>
#include <sys/stat.h>
#include <sstream>
int main()
{
    Ftpclient client;
    if(client.connectserver("127.0.0.1", 2100))
    {
        std::cout<<"Connected to FTP server successfully!"<<std::endl;
    }
    else
    {
        std::cerr<<"Failed to connect to FTP server!"<<std::endl;
        return 1;
    }

    std::string user;
    std::string pass;

    std::cout << "USER: ";
    std::getline(std::cin, user);

    std::cout << "PASS: ";
    std::getline(std::cin, pass);

    // ====================== 关键修复：加上这一行 ======================
    if (!client.login(user, pass)) {
        std::cerr << "Login failed!\n";
        return 0;
    }
    std::cout << "Login success!\n";
    // ================================================================

    while(true)
    {
        std::cout<<"ftp> ";
        std::string line;
        std::string cmd;
        std::getline(std::cin,line);
        std::istringstream iss(line);
        iss>>cmd;
        if(cmd=="quit"||cmd=="exit")
        {
            client.quit();
            break;
        }
        else if(cmd=="list"||cmd=="ls")
        {
            client.list();
        }
        else if(cmd=="pwd")
        {
            client.pwd();
        }
        else if(cmd=="cwd")
        {
            std::string dir;
            iss>>dir;
            client.cwd(dir);
        }
        else if(cmd=="type")
        {
            char t;
            iss>>t;
            client.type(t);
        }
        else if(cmd=="retr")
        {
            std::string remotefile,localfile;
            iss>>remotefile;
            iss>>localfile;
            client.retr(remotefile,localfile);
        }
        else if(cmd=="stor")
        {
            std::string localfile, remotefile;
            iss >> localfile >> remotefile;

            if(remotefile.empty())
                remotefile = localfile;
            std::cout << "[DEBUG] local=" << localfile
                      << " remote=" << remotefile << std::endl;
            client.stor(localfile, remotefile);
        }
        else
        {
            std::cout<<"Unknown command: "<<cmd<<std::endl;
        }
    }
    return 0;
}
