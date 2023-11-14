#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <memory>
#include <string>
#include "thread_utils.h"

// forward declarations for faster render.cpp compiles
namespace seasocks{
	class Server;
	class PageHandler;
}

class WebServer {
public:
    WebServer();
    WebServer(int _port);
    ~WebServer();

    void setup(int _port);

private:
    int port;
    std::shared_ptr<seasocks::Server> server;

    void cleanup();
    void printServerAddress();

    pthread_t serve_thread;
	void* serve_func();
	static void* serve_func_static(void* arg);

    // bool isListening();
};

#endif // WEBSERVER_H
