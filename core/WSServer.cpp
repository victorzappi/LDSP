// This code is based on the code credited below, but it has been modified
// further by Victor Zappi

 /*
 ___  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the Centre for Digital Music at Queen Mary University of London. http://instrumentslab.org

(c) 2016-2020 Augmented Instruments Laboratory: Andrew McPherson, Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack, Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License (LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt */

#include "WSServer.h"
#include <seasocks/IgnoringLogger.h>
#include <seasocks/Server.h>
#include <seasocks/WebSocket.h>
#include <unistd.h>


constexpr unsigned int WSServerClientSleepUs = 100;

WSServer::WSServer(){}
WSServer::WSServer(unsigned int port, std::string resourceRoot){
	setup(port, resourceRoot);
}
WSServer::~WSServer(){
	cleanup();
}

struct GuiWSHandler : seasocks::WebSocket::Handler {
	std::shared_ptr<seasocks::Server> server;
	std::set<seasocks::WebSocket*> connections;
	std::string address;
	std::function<void(std::string, void*, int)> on_receive;
	std::function<void(std::string)> on_connect;
	std::function<void(std::string)> on_disconnect;
	bool binary;
	void onConnect(seasocks::WebSocket *socket) override {
		connections.insert(socket);
		//std::cout << "Connection established" << std::endl;
		if(on_connect)
			on_connect(address);
	}
	void onData(seasocks::WebSocket *socket, const char *data) override {
		on_receive(address, (void*)data, std::strlen(data));
	}
	void onData(seasocks::WebSocket *socket, const uint8_t* data, size_t size) override {
		on_receive(address, (void*)data, size);
	}
	void onDisconnect(seasocks::WebSocket *socket) override {
		connections.erase(socket);
		//std::cout << "Connection closed" << std::endl;
		if (on_disconnect)
			on_disconnect(address);
	}
};


void WSServer::setup(unsigned int port, std::string resourceRoot) {
	_port = port;
	_resourceRoot = resourceRoot;

	auto logger = std::make_shared<seasocks::IgnoringLogger>();
	server = std::make_shared<seasocks::Server>(logger);

    // prepare client loop vars
    outputs_writePtr.store(-1);
	outputs_readPtr = -1;

	shouldStop = false;
	pthread_create(&client_thread, NULL, client_func_static, this);
	pthread_create(&serve_thread, NULL, serve_func_static, this);
}


void WSServer::addAddress(std::string address, std::function<void(std::string, void*, int)> on_receive, std::function<void(std::string)> on_connect, std::function<void(std::string)> on_disconnect, bool binary){
	auto handler = std::make_shared<GuiWSHandler>();
	handler->server = server;
	handler->address = address;
	handler->on_receive = on_receive;
	handler->on_connect = on_connect;
	handler->on_disconnect = on_disconnect;
	handler->binary = binary;
	server->addWebSocketHandler((std::string("/")+address).c_str(), handler);

	address_book[address] = handler;
}

int WSServer::send(const char* address, const char* str) {
	return send(address, (const void*)str, strlen(str));
}

int WSServer::send(const char* address, const void* buf, unsigned int size) 
{
	// ensure the size does not exceed the buffer capacity
    if (size > WSOutDataMax) {
		printf("Web socket server warning! The data buffer sent to %s is too long (size %d) and will be truncated (new size %d)!\n", address, size, WSOutDataMax);
		size = WSOutDataMax; // truncate data
    }


	// pack up arguments	
	WSOutputData out;
	out.address = (char *)address;
	memcpy(out.buff, buf, size);    // Copy the data into the buffer; no need to erase it, because we store size too!
	out.size = size;

	// read the index of the last value we wrote
	int writePtr = outputs_writePtr.load();
	writePtr = (writePtr + 1) % output_queue_size;

	//printf("---------------> buffer %d[%d]: %s\n", writePtr, size, (char *)out.buff);

	outputs[writePtr].store(out); // push output into queue
	outputs_writePtr.store(writePtr);  // update index of the last value in queue

	return 0;
}

void WSServer::cleanup()
{
	shouldStop = true;
	server->terminate();
	// wait for completion
	pthread_join(client_thread, NULL);
    pthread_join(serve_thread, NULL);
}



void* WSServer::serve_func(std::string resourceRoot)
{
	// no need to loop, Server::serve is looping already. 
	// also, serve is killed via void WSServer::cleanup(), with server->terminate()
	server->serve(resourceRoot.c_str(), _port);
	return (void *)0;
}

void* WSServer::serve_func_static(void* arg)
{
    // set thread priority
	set_priority(LDSPprioOrder_wserverServe, "WebSocketServe", false);

	// set minimum thread niceness
 	set_niceness(-20, "WebSocketServe", false);

	WSServer* wsServer = static_cast<WSServer*>(arg);    
    return wsServer->serve_func(wsServer->getResourceRoot());
}

void* WSServer::client_func()
{
	while(!shouldStop)
	{
		// here we use a non-blocking circular buffer to make sure that all output values are sent
        // this is guaranteed by the large size of the buffer, i.e., the store index [write pointer] will never overtake by a lap the load index [read pointer]

        // read the index of the latest value that was written
        int writePtr = outputs_writePtr.load();

        // until the index of the last written value is equal to the index of the last value we read/sent...
        while(outputs_readPtr != writePtr) 
        { 
			outputs_readPtr = (outputs_readPtr + 1) % output_queue_size;  // move the read pointer forward, it will be the latest value we send

			WSOutputData output = outputs[outputs_readPtr].load(); // get current output
			// unpack 
			std::shared_ptr<GuiWSHandler> handler = address_book.at(output.address);
			const void* buf = output.buff;
			unsigned int size = output.size;

			//printf("---------------> %s --- buffer %d[%d]: %s\n", output.address, outputs_readPtr, size, (char *)output.buff);
			 
			try  
			{
				// send, via execute
				if(handler->binary)
				{
					// make a copy of the data before we send it out
					auto data = std::make_shared<std::vector<void*> >(size);
					memcpy(data->data(), buf, size);
					handler->server->execute([handler, data, size] {
						for (auto c : handler->connections){
							c->send((uint8_t*) data->data(), size);
						}
					});
				} else {
					// make a copy of the data before we send it out
					std::string str = (const char*)buf;
					handler->server->execute([handler, str] {
						for (auto c : handler->connections) {
							c->send(str.c_str());
						}
					});
				}
			} catch(std::exception& e) 
			{
				std::cerr << "Could not send data via web server, exception caught: " << e.what() << std::endl;
			}
		}

		usleep(WSServerClientSleepUs);
	}
	return (void *)0;
}

void* WSServer::client_func_static(void* arg)
{
    // set thread priority
    set_priority(LDSPprioOrder_wserverClient, "WebSocketClient", false);

	// set minimum thread niceness
 	set_niceness(-20, "WebSocketClient", false);

	WSServer* weServer = static_cast<WSServer*>(arg);    
    return weServer->client_func();
}