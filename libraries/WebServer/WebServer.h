#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "libraries/WSServer/WSServer.h"


// forward declarations for faster render.cpp compiles
namespace seasocks{
	class Server;
	class PageHandler;
}

class WebServer : public WSServer {
public:
    WebServer();
    WebServer(unsigned int port, std::string resourceRoot="/dev/null");
    ~WebServer();

    void setup(unsigned int port, std::string resourceRoot="/dev/null");
    void setup(std::string projectName, std::string serverName, unsigned int port, std::string resourceRoot="/dev/null");

    void addPageHandler(std::__ndk1::shared_ptr<seasocks::PageHandler> handler);

    void run();

private:
    std::string _projectName;
    std::string _serverName;

    void printServerAddress();
    std::string runIpCommand(std::string command);

	void* serve_func(std::string resourceRoot);
    static void* serve_func_static(void* arg);

    // bool isListening();
};

#endif // WEBSERVER_H
