#include "WebServer.h"
#include <seasocks/Server.h>
#include <seasocks/PageHandler.h>
#include <seasocks/IgnoringLogger.h>
// #include <seasocks/PrintfLogger.h> //VIC for debug
#include <array>
#include <regex>
//#include <fstream>   // include to use system()
//#include <unistd.h>  // include for close()
#if __ANDROID_API__ >= 24
#include <ifaddrs.h> // for the API specific content of getWiFiIPAddress()
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

WebServer::WebServer() {}

WebServer::WebServer(unsigned int port, std::string resourceRoot) {
    setup("", "", port);
}

WebServer::~WebServer() {
}

void WebServer::setup(unsigned int port, std::string resourceRoot) {
    setup("", "", port, resourceRoot);
}

void WebServer::setup(std::string projectName, std::string serverName, unsigned int port, std::string resourceRoot) {
    _projectName = projectName;
    _serverName = serverName;
    _port = port;
    _resourceRoot = resourceRoot;

    auto logger = std::make_shared<seasocks::IgnoringLogger>();
    // auto logger = std::make_shared<seasocks::PrintfLogger>(); //VIC for debug with proper header up
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


void* WebServer::serve_func(std::string resourceRoot)
{
	// no need to loop, Server::serve is looping already. 
	// also, serve is killed via void WebServer::cleanup(), with server->terminate()
    server->serve(resourceRoot.c_str(), _port);
    printf("%s web server terminated!\n", _serverName.c_str());
	return (void *)0;
}

void* WebServer::serve_func_static(void* arg)
{
    // set thread priority
	set_priority(LDSPprioOrder_wserverServe, "WebServerServe", false);

    // set minimum thread niceness
 	set_niceness(-20, "WebServerServe", false);

	WebServer* webServer = static_cast<WebServer*>(arg);    
    return webServer->serve_func(webServer->getResourceRoot());
}

void WebServer::printServerAddress() {
        std::string lastValidIPAddress;    
        std::string command;
        
        lastValidIPAddress = getWiFiIPAddress();

        // if ifconfig is not available
        if(lastValidIPAddress.empty()) {
            // run ifconfig and parse output
            command = "ifconfig";
            lastValidIPAddress = runIpCommand(command);
        }
        
        // if ifconfig is not available
        if(lastValidIPAddress.empty())
        {
            // run ip addr and parse output
            command = "ip addr";
            lastValidIPAddress = runIpCommand(command);
        }
        
         // if none found, use default local host
        if(lastValidIPAddress.empty())
          lastValidIPAddress = "127.0.0.1";
          
        printf("%s web server listening on: %s:%d\n", _serverName.c_str(), lastValidIPAddress.c_str(), _port);

}

const std::vector<std::string> BLOCKED_INTERFACES = {
    "rmnet",    // Qualcomm cellular
    "dummy",    // dummy interface
    "lo",       // loopback
    "p2p",      // WiFi Direct
    //"tun",      // VPN tunnels
    "ppp",      // PPP (older cellular)
    "ccmni",    // MediaTek cellular
    //"usb",      // USB tethering
    //"rndis",    // USB tethering (Windows style)
    //"bt-pan"    // Bluetooth tethering
};

// this methods uses functionalities available only from API 24 on
// in lower APIs it return an empty string
// Add as a class member or global
std::string WebServer::getWiFiIPAddress() {
    std::string wifiIP;
#if __ANDROID_API__ >= 24
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *temp_addr = nullptr;
    
    if (getifaddrs(&interfaces) == 0) {
        temp_addr = interfaces;
        while (temp_addr != nullptr) {
            if (temp_addr->ifa_addr && temp_addr->ifa_addr->sa_family == AF_INET) {
                std::string ifname = temp_addr->ifa_name;
                
                // Skip blocked interfaces
                bool skip = false;
                for (const auto& blocked : BLOCKED_INTERFACES) {
                    if (ifname.find(blocked) != std::string::npos) {
                        skip = true;
                        break;
                    }
                }
                if (skip) {
                    temp_addr = temp_addr->ifa_next;
                    continue;
                }
                
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,
                         &((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr,
                         addressBuffer, INET_ADDRSTRLEN);
                
                std::string ip(addressBuffer);
                /// returns the first non-localhost
                if (ip.substr(0, 3) != "127") {
                    wifiIP = ip;
                    break;
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    
    if (interfaces != nullptr)
        freeifaddrs(interfaces);
#endif
    return wifiIP;
}


std::string WebServer::runIpCommand(std::string command)
{
    try {
        std::array<char, 128> buffer;
        std::string result;
        std::string lastValidIPAddress;

        // look for all strings in this form: 1-to-3-nums.1-to-3-nums.1-to-3-nums.1-to-3-nums
        std::regex ipRegex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
        std::smatch ipMatch;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if(!pipe)
            throw std::runtime_error("popen() failed!");

        // removes all strings that start and end with 255 and returns the last one
        while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
        {
            result = buffer.data();
            if(std::regex_search(result, ipMatch, ipRegex)) 
            {
                std::string ip = ipMatch[1];
                bool skip = false;
                for (const auto& blocked : BLOCKED_INTERFACES) {
                    if (result.find(blocked) != std::string::npos) {
                        skip = true;
                        break;
                    }
                }
                if (skip) continue;

                // printf("---------> ip %s\n", ip.c_str());
                if(ip.substr(0, 1) != "0" && ip.substr(0, 3) != "255" && 
                   ip.substr(0, 3) != "127" && ip.substr(ip.size() - 4) != ".255")
                    lastValidIPAddress = ip;
            }
        }

        return lastValidIPAddress;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
}

//VIC this is equivalent, but popen() is preferred to system()
/* std::string WebServer::runIpCommand(std::string command) {
    try {
        // Execute command and redirect output to a file
        int ret = system((command + " > temp_output.txt").c_str());
        if (ret != 0) {
            std::cerr << "Command "<< command << " failed with return code: " << ret << std::endl;
            return "";
        }

        // Read output from the file
        std::ifstream file("temp_output.txt");
        if (!file.is_open()) {
            std::cerr << "Failed to open temp_output.txt" << std::endl;
            return "";
        }

        std::string result, line, lastValidIPAddress;
        std::regex ipRegex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
        std::smatch ipMatch;

        while (std::getline(file, line)) {
            if (std::regex_search(line, ipMatch, ipRegex)) {
                std::string ip = ipMatch[1];
                if (ip.substr(0, 1) != "0" && ip.substr(0, 3) != "255" && ip.substr(ip.size() - 4) != ".255") {
                    lastValidIPAddress = ip;
                }
            }
        }

        // Close and delete the temporary file
        file.close();
        std::remove("temp_output.txt");

        return lastValidIPAddress;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
} */



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
