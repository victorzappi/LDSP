#ifndef WSSERVER_H
#define WSSERVER_H

#include <string>
#include <memory>
// #include <set>
#include <map>
// #include <vector>
// #include <functional>
// #include <atomic>
#include "thread_utils.h"

// forward declarations for faster render.cpp compiles
namespace seasocks{
	class Server;
}
//class AuxTaskNonRT;
struct GuiWSHandler;

constexpr unsigned int WSOutDataMax = 200;

struct WSOutputData {
	char* address;
	//void* buff;
	char buff[WSOutDataMax];
    unsigned int size;
};

class WSServer{
	//friend struct GuiWSHandler;
	public:
		WSServer();
		WSServer(unsigned int port);
		~WSServer();
		
		virtual void setup(unsigned int port);

		void addAddress(std::string address, std::function<void(std::string, void*, int)> on_receive = nullptr, std::function<void(std::string)> on_connect = nullptr, std::function<void(std::string)> on_disconnect = nullptr, bool binary = false);
		
		int sendNonRt(const char* address, const char* str);
		int sendNonRt(const char* address, const void* buf, unsigned int size);
		int sendRt(const char* address, const char* str);
		int sendRt(const char* address, const void* buf, unsigned int size);
		
	protected:
		void cleanup();

		static constexpr int output_queue_size = 100; // this is the number of outputs that can be written in a row without the risk of skipping transmission

		unsigned int _port;
		//std::string _address;
		std::shared_ptr<seasocks::Server> server;
		
		// struct AddressBookItem {
		// 	std::unique_ptr<AuxTaskNonRT> thread;
		// 	std::shared_ptr<GuiWSHandler> handler;
		// };
		//std::map<std::string, AddressBookItem> address_book;
		std::map< std::string, std::shared_ptr<GuiWSHandler> > address_book;
		//std::unique_ptr<AuxTaskNonRT> server_task;
		
		//void client_task_func(std::shared_ptr<GuiWSHandler> handler, const void* buf, unsigned int size);
		
		//VIC
	    bool shouldStop;

		pthread_t client_thread;
		std::atomic<WSOutputData> outputs[output_queue_size];
    	int outputs_readPtr;
    	std::atomic<int> outputs_writePtr;
		void* client_func();
		static void* client_func_static(void* arg);


		pthread_t serve_thread;
		virtual void* serve_func();
		static void* serve_func_static(void* arg);

};

#endif // WSSERVER_H
