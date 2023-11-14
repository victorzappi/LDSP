/***** WSServer.cpp *****/
#include "WSServer.h"
#include <seasocks/IgnoringLogger.h>
#include <seasocks/Server.h>
#include <seasocks/WebSocket.h>
#include <cstring>
#include <unistd.h>


constexpr unsigned int WebServerClientSleepUs = 100;

WSServer::WSServer(){}
WSServer::WSServer(int port){
	setup(port);
}
WSServer::~WSServer(){
	cleanup();
}

struct WSServerDataHandler : seasocks::WebSocket::Handler {
	std::shared_ptr<seasocks::Server> server;
	std::set<seasocks::WebSocket*> connections;
	std::string address;
	std::function<void(std::string, void*, int)> on_receive;
	std::function<void(std::string)> on_connect;
	std::function<void(std::string)> on_disconnect;
	bool binary;
	void onConnect(seasocks::WebSocket *socket) override {
		connections.insert(socket);
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
		if (on_disconnect)
			on_disconnect(address);
	}
};

// void WSServer::client_task_func(std::shared_ptr<WSServerDataHandler> handler, const void* buf, unsigned int size){
// 	if (handler->binary){
// 		// make a copy of the data before we send it out
// 		auto data = std::make_shared<std::vector<void*> >(size);
// 		memcpy(data->data(), buf, size);
// 		handler->server->execute([handler, data, size]{
// 			for (auto c : handler->connections){
// 				c->send((uint8_t*) data->data(), size);
// 			}
// 		});
// 	} else {
// 		// make a copy of the data before we send it out
// 		std::string str = (const char*)buf;
// 		handler->server->execute([handler, str]{
// 			for (auto c : handler->connections){
// 				c->send(str.c_str());
// 			}
// 		});
// 	}
// }

void WSServer::setup(int _port) {
	port = _port;
	auto logger = std::make_shared<seasocks::IgnoringLogger>();
	server = std::make_shared<seasocks::Server>(logger);
	//make this a regular pthread. does not need to be looped, necause serve is looping already. serve if killed with void WSServer::cleanup(){server->terminate();
	//server_task = std::unique_ptr<AuxTaskNonRT>(new AuxTaskNonRT());
	//server_task->create(std::string("WSServer_")+std::to_string(_port), [this](){ server->serve("/dev/null", port); });
	//server_task->schedule();

    // prepare client loop vars
    outputs_writePtr.store(-1);
	outputs_readPtr = -1;

	shouldStop = false;
	pthread_create(&client_thread, NULL, client_func_static, this);
	pthread_create(&serve_thread, NULL, serve_func_static, this);
}

void WSServer::addAddress(std::string _address, std::function<void(std::string, void*, int)> on_receive, std::function<void(std::string)> on_connect, std::function<void(std::string)> on_disconnect, bool binary){
	auto handler = std::make_shared<WSServerDataHandler>();
	handler->server = server;
	handler->address = _address;
	handler->on_receive = on_receive;
	handler->on_connect = on_connect;
	handler->on_disconnect = on_disconnect;
	handler->binary = binary;
	server->addWebSocketHandler((std::string("/")+_address).c_str(), handler);
	
	//turn this into a pthrea BUT create new one everytime or loop? check sendR, it is where it's schedules right now
	// we can do the same trick with the static version of the task function, but we also need to somehow pass the params to the function
	// we will not pass them, they will be fields [so we will remove from client_task_func signature] ---> all atomics queues
	// let's loop the thread and wait for new elements in queues 
	// sendRT calls add elements in queues [there will be only sendRT]
	// thread function loops until the queues are empty
	// address_book[_address] = {
	// 	//.thread = std::unique_ptr<AuxTaskNonRT>(new AuxTaskNonRT()),
	// 	.handler = handler,
	// };
	//address_book[_address].thread->create(std::string("WSClient_")+_address, [this, handler](void* buf, int size){ client_task_func(handler, buf, size); });
	address_book[_address] = handler;
}

int WSServer::sendNonRt(const char* _address, const char* str) {
	return sendNonRt(_address, (const void*)str, strlen(str));
}

int WSServer::sendNonRt(const char* _address, const void* buf, unsigned int size) 
{
	// pack up arguments	
	output_struct out;
	out.address = (char *)_address;
	out.buff = (void *)buf;
	out.size = size;

	// read the index of the last value we wrote
	int writePtr = outputs_writePtr.load();
	writePtr = (writePtr + 1) % output_queue_size;
	outputs[writePtr].store(out); // push output into queue
	outputs_writePtr.store(writePtr);  // update index of the last value in queue

	return 0;
}

int WSServer::sendRt(const char* _address, const char* str) {
	return sendNonRt(_address, (const void*)str, strlen(str));
}

int WSServer::sendRt(const char* _address, const void* buf, unsigned int size){
	return sendNonRt(_address, buf, size);
}

void WSServer::cleanup()
{
	shouldStop = true;
	server->terminate();
	// wait for completion
	pthread_join(client_thread, NULL);
    pthread_join(serve_thread, NULL);
}




//VIC
void* WSServer::serve_func()
{
	// no need to loop, Server::serve is looping already. 
	// also, serve is killed via void WSServer::cleanup(), with server->terminate()
	server->serve("/dev/null", port);
	return (void *)0;
}

void* WSServer::serve_func_static(void* arg)
{
	// set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
	set_priority(LDSPprioOrder_wsserverServe, false);

	WSServer* wsServer = static_cast<WSServer*>(arg);    
    return wsServer->serve_func();
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

			output_struct output = outputs[outputs_readPtr].load(); // get current output
			// unpack 
			std::shared_ptr<WSServerDataHandler> handler = address_book.at(output.address);
			const void* buf = output.buff;
			unsigned int size = output.size;
			 
			try  
			{
				// send, via execute
				if (handler->binary)
				{
					// make a copy of the data before we send it out
					auto data = std::make_shared<std::vector<void*> >(size);
					memcpy(data->data(), buf, size);
					handler->server->execute([handler, data, size]{
						for (auto c : handler->connections){
							c->send((uint8_t*) data->data(), size);
						}
					});
				} else {
					// make a copy of the data before we send it out
					std::string str = (const char*)buf;
					handler->server->execute([handler, str]{
						for (auto c : handler->connections){
							c->send(str.c_str());
						}
					});
				}
			} catch (std::exception& e) 
			{
				std::cerr << "Could not send data via web server, exception caught: " << e.what() << std::endl;
			}
		}

		usleep(WebServerClientSleepUs);
	}
	return (void *)0;
}

void* WSServer::client_func_static(void* arg)
{
	// set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
    set_priority(LDSPprioOrder_wsserverClient, false);

	WSServer* weServer = static_cast<WSServer*>(arg);    
    return weServer->client_func();
}