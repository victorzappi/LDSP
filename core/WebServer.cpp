#include "WebServer.h"
#include <seasocks/Server.h>
#include <seasocks/PageHandler.h>
#include <seasocks/IgnoringLogger.h>
#include <array>
#include <regex>

//#include <unistd.h>  // Include for close function

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

    // prepare client loop vars
    outputs_writePtr.store(-1);
	outputs_readPtr = -1;
}

void WebServer::addPageHandler(std::__ndk1::shared_ptr<seasocks::PageHandler> handler) {
    server->addPageHandler(handler);
}

void WebServer::run() {
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
