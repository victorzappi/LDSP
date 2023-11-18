#include "WebServer.h"
#include <seasocks/Server.h>
#include <seasocks/PageHandler.h>
#include <seasocks/IgnoringLogger.h>
#include <seasocks/ResponseBuilder.h>
#include <seasocks/StringUtil.h>

#include <fstream>
#include <array>
#include <regex>

//#include <unistd.h>  // Include for close function


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
    try {
        // run ifconfig and parse output
        std::string command = "ifconfig";
        std::array<char, 128> buffer;
        std::string result;
        std::string lastValidIPAddress;

        // look for all strings in this form: 1-to-3-nums.1-to-3-nums.1-to-3-nums.1-to-3-nums
        std::regex ipRegex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
        std::smatch ipMatch;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        // removes all strings that start and end with 255 and returns the last one
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result = buffer.data();
            if (std::regex_search(result, ipMatch, ipRegex)) {
                std::string ip = ipMatch[1];
                if (ip.substr(0, 3) != "255" && ip.substr(ip.size() - 4) != ".255") {
                    lastValidIPAddress = ip;
                }
            }
        }

        printf("GUI web server listening on: %s:%d\n", lastValidIPAddress.c_str(), port);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
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
