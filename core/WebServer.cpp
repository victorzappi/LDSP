#include "WebServer.h"
#include <seasocks/Server.h>
#include <seasocks/PageHandler.h>

#include <seasocks/IgnoringLogger.h>
#include <seasocks/ResponseBuilder.h>
#include <seasocks/StringUtil.h>

#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h> 
#include <arpa/inet.h>


#include <cstring>
#include <memory>
#include <sstream>
#include <fstream>

// #include <unistd.h>  // Include for close function


// class GuiPageHandler : public seasocks::PageHandler {
// public:
//     std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request) override {
//         // Create and return the response
//         std::string content = "<html><body style='background-color:orange;display:flex;justify-content:center;align-items:center;height:100vh;'><div>LDSP</div></body></html>";
//         return seasocks::Response::htmlResponse(content);
//     }
// };

class GuiPageHandler : public seasocks::PageHandler {
public:
    std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request) override {
        const std::string uri = request.getRequestUri();

        if (uri == "/sketch.js") {
            return serveFile("/data/ldsp/sketch.js", "application/javascript");
        } else if (uri == "/p5.min.js") {
            return serveFile("/data/ldsp/gui/p5.min.js", "application/javascript");
        } else if (uri == "/LDSPWebSocket.js") {
            return serveFile("/data/ldsp/gui/LDSPWebSocket.js", "application/javascript");
        } else if (uri == "/LDSPData.js") {
            return serveFile("/data/ldsp/gui/LDSPData.js", "application/javascript");
        } else if (uri == "/LDSP.js") {
            return serveFile("/data/ldsp/gui/LDSP.js", "application/javascript");
        } else {
            // Default: serve index.html
            return serveFile("/data/ldsp/gui/index.html", "text/html");
        }
    }

private:
    std::shared_ptr<seasocks::Response> serveFile(const std::string& path, const std::string& mimeType) {
    std::ifstream file(path, std::ios::binary);

        if (file) {
            seasocks::ResponseBuilder builder(seasocks::ResponseCode::Ok);
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            builder.withContentType(mimeType);
            builder << content;
            return builder.build();
        } else {
            seasocks::ResponseBuilder builder(seasocks::ResponseCode::NotFound);
            builder.withContentType("text/plain");
            builder << "File not found";
            return builder.build();
        }
    }
};

WebServer::WebServer() {}

WebServer::WebServer(int _port) {
    setup(_port);
}

WebServer::~WebServer() {
    cleanup();
}

void WebServer::setup(int _port) {
    port = _port;
    server = std::make_shared<seasocks::Server>(std::make_shared<seasocks::IgnoringLogger>());
    server->addPageHandler(std::make_shared<GuiPageHandler>());

    pthread_create(&serve_thread, NULL, serve_func_static, this);
    printServerAddress();
}


void* WebServer::serve_func()
{
	// no need to loop, Server::serve is looping already. 
	// also, serve is killed via void WebServer::cleanup(), with server->terminate()
    server->serve("/dev/null", port);
    printf("GUI web server terminated!\n");
	return (void *)0;
}

void WebServer::cleanup() {
	server->terminate();
	// wait for completion
	pthread_join(serve_thread, NULL);
}

void* WebServer::serve_func_static(void* arg)
{
	// set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
	set_priority(LDSPprioOrder_webserverServe, false);

	WebServer* webServer = static_cast<WebServer*>(arg);    
    return webServer->serve_func();
}

void WebServer::printServerAddress() {
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *temp_addr = nullptr;
    int success = 0;

    success = getifaddrs(&interfaces);
    if (success == 0) {
        temp_addr = interfaces;
        while(temp_addr != nullptr) {
            if(temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is UP and not a loopback interface
                if((temp_addr->ifa_flags & IFF_UP) && !(temp_addr->ifa_flags & IFF_LOOPBACK)) {
                    char *address = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                    printf("GUI web server listening on: %s:%d\n", address, port);
                    break;
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    freeifaddrs(interfaces);
}



// bool WebServer::isListening() {
//     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd < 0) return false;

//     sockaddr_in serv_addr;
//     memset(&serv_addr, 0, sizeof(serv_addr));
//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//     serv_addr.sin_port = htons(port);

//     bool listening = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0;
//     close(sockfd);
//     return listening;
// }
