#include "WebServer.h"
#include <seasocks/Server.h>
#include <seasocks/PageHandler.h>
#include <seasocks/IgnoringLogger.h>
#include <seasocks/ResponseBuilder.h>
#include <seasocks/StringUtil.h>
#include <seasocks/Request.h>
#include <seasocks/Response.h>

#include <fstream>
#include <array>
#include <regex>
#include <filesystem>
namespace fs = std::__fs::filesystem;

//#include <unistd.h>  // Include for close function

class GuiPageHandler : public seasocks::PageHandler {
public:
    GuiPageHandler(std::string projectName) : _projectName(projectName) {}

    std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request) override {
        const std::string uri = request.getRequestUri();

        // this is needed to pass web socket requests to the web socket handler
        if (request.verb() == seasocks::Request::Verb::WebSocket) 
            return seasocks::Response::unhandled();

        // printf("___________%s\n", uri.c_str());

        // Function to check if a string ends with another string
        auto endsWith = [](const std::string &fullString, const std::string &ending) {
            if (fullString.length() >= ending.length()) {
                return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
            } else {
                return false;
            }
        };

        // Handling for css files in the /gui/js/ directory
        if (uri.find("/gui/css/") == 0) {
            std::string filePath = "/data/ldsp/resources" + uri; 
            // printf("/gui/css/___________%s\n", filePath.c_str());
            return serveFile(filePath, "text/css");
        }

        // Handling for js files in the /gui/js/ directory
        if (uri.find("/gui/js/") == 0) {
            std::string filePath = "/data/ldsp/resources" + uri; 
            // printf("/gui/js/___________%s\n", filePath.c_str());
            return serveFile(filePath, "application/javascript");
        }

        // Handling for js files in the /js/ directory
        if (uri.find("/js/") == 0) {
            std::string filePath = "/data/ldsp/resources" + uri;
            // printf("/js/___________%s\n", filePath.c_str());
            return serveFile(filePath, "application/javascript");
        }
        
        // Handling for /gui/gui-template.html
        if (uri == "/gui/gui-template.html") {
            return serveFile("/data/ldsp/resources/gui/gui-template.html", "text/html");
        }
        
        // Handling for /gui/p5-sketches/sketch.js
        if (uri == "/gui/p5-sketches/sketch.js") {
            return serveFile("/data/ldsp/resources/resources/gui/p5-sketches/sketch.js", "application/javascript");
        }
        
        // Dynamic handling for project-specific files
        if (uri.find("/projects/") == 0) {
            std::string filePath;
            if (endsWith(uri, "/main.html")) {
                filePath = "/data/ldsp/projects/" + _projectName + "/main.html";
            } else if (endsWith(uri, ".js")) {
                filePath = "/data/ldsp/projects/" + _projectName + "/sketch.js";
            }

            //  printf("/projects/___________%s\n", filePath.c_str());

            // Check if file exists
            if (fs::exists(filePath)) {
                return serveFile(filePath, endsWith(uri, ".js") ? "application/javascript" : "text/html");
            } else {
                // File not found handling
                seasocks::ResponseBuilder builder(seasocks::ResponseCode::NotFound);
                builder.withContentType("text/plain");
                builder << "File not found";
                return builder.build();
            }
        }

        // Default case for serving the main HTML file
        if (uri == "/" || uri == "/gui/index.html" || uri == "/gui/") {
            // printf("/___________%s\n", uri.c_str());
            // printf("/___________/data/ldsp/resources/gui/index.html\n");
            return serveFile("/data/ldsp/resources/gui/index.html", "text/html");
        }


        // Default case for URIs that don't match any known patterns
        seasocks::ResponseBuilder builder(seasocks::ResponseCode::NotFound);
        builder.withContentType("text/plain");
        builder << "Resource not found for URI: " << uri;
        return builder.build();
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

    std::string _projectName;
};

WebServer::WebServer() {}

WebServer::WebServer(unsigned int port) {
    setup("", port);
}

WebServer::~WebServer() {
    cleanup();
}

void WebServer::setup(unsigned int port) {
    setup("", port);
}

void WebServer::setup(std::string projectName, unsigned int port) {
    _projectName = projectName;
    _port = port;
    auto logger = std::make_shared<seasocks::IgnoringLogger>();
	server = std::make_shared<seasocks::Server>(logger);
    server->addPageHandler(std::make_shared<GuiPageHandler>(_projectName));

    // prepare client loop vars
    outputs_writePtr.store(-1);
	outputs_readPtr = -1;

    shouldStop = false;
	pthread_create(&client_thread, NULL, client_func_static, this);
	pthread_create(&serve_thread, NULL, serve_func_static, this);

    printServerAddress();
}

void* WebServer::serve_func()
{
	// no need to loop, Server::serve is looping already. 
	// also, serve is killed via void WebServer::cleanup(), with server->terminate()
    server->serve("/dev/null", _port);
    printf("GUI web server terminated!\n");
	return (void *)0;
}

void* WebServer::serve_func_static(void* arg)
{
	// set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
	set_priority(LDSPprioOrder_wserverServe, false);

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

        printf("GUI web server listening on: %s:%d\n", lastValidIPAddress.c_str(), _port);
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
