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


#ifndef WSSERVER_H
#define WSSERVER_H

#include <string>
#include <memory>
#include <map>
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
	char buff[WSOutDataMax];
    unsigned int size;
};

class WSServer{
	public:
		WSServer();
		WSServer(unsigned int port);
		~WSServer();
		
		virtual void setup(unsigned int port);

		void addAddress(std::string address, std::function<void(std::string, void*, int)> on_receive = nullptr, std::function<void(std::string)> on_connect = nullptr, std::function<void(std::string)> on_disconnect = nullptr, bool binary = false);
		
		int send(const char* address, const char* str);
		int send(const char* address, const void* buf, unsigned int size);
		
	protected:
		void cleanup();

		static constexpr int output_queue_size = 100; // this is the number of outputs that can be written in a row without the risk of skipping transmission

		unsigned int _port;	
		std::shared_ptr<seasocks::Server> server;
		std::map< std::string, std::shared_ptr<GuiWSHandler> > address_book;
		
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
