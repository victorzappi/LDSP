#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "WSServer.h"


// forward declarations for faster render.cpp compiles
namespace seasocks{
	class Server;
	class PageHandler;
}

class WebServer : public WSServer {
public:
    WebServer();
    WebServer(unsigned int port);
    ~WebServer();

    void setup(unsigned int port);
    void setup(std::string projectName, unsigned int port);

private:
    std::string _projectName;

    void printServerAddress();

	void* serve_func();
    static void* serve_func_static(void* arg);

    // bool isListening();
};

#endif // WEBSERVER_H
