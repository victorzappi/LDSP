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


#include "OscReceiver.h"
#include "libraries/UdpServer/UdpServer.h"

constexpr unsigned int OscReceiverBlockReadUs = 50000;
constexpr unsigned int OscReceiverSleepBetweenReadsUs = 5000;
constexpr unsigned int OscReceiverInBufferSize = 65536; // maximum UDP packet size

OscReceiver::OscReceiver(){}
OscReceiver::OscReceiver(int port, std::function<void(oscpkt::Message* msg, const char* addr, void* arg)> on_receive, void* callbackArg){
	setup(port, on_receive, callbackArg);
}

OscReceiver::~OscReceiver(){
	lShouldStop = true;
	// allow in-progress read to complete before destructing
	// if(receive_task && receive_task->joinable())
	// {
	// 	receive_task->join();
	// }
	if(receive_thread != 0)
		pthread_join(receive_thread, NULL);

}

void *OscReceiver::receive_thread_func(void *ptr){
	OscReceiver* instance = (OscReceiver*)ptr;
	
	// set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
	set_priority(instance->prioOrder, false);

	while(!instance->lShouldStop){
		int ret = instance->waitForMessage(OscReceiverBlockReadUs / 1000);
		if(ret < 0)
			break; // error. Abort
		else if(ret >= 0)
			continue; // message retrieved successfully. Try again immediately
		else
			//(0 == ret) no message retrieved. Briefly sleep before retrying
			usleep(OscReceiverSleepBetweenReadsUs);
	}
	return (void *)0;
}

void OscReceiver::setup(int port, std::function<void(oscpkt::Message* msg, const char* addr, void* arg)> _on_receive, void* callbackArg)
{
	inBuffer.resize(OscReceiverInBufferSize);

	onReceiveArg = callbackArg;
	on_receive = _on_receive;
	pr = std::unique_ptr<oscpkt::PacketReader>(new oscpkt::PacketReader());

	socket = std::unique_ptr<UdpServer>(new UdpServer());
	if(!socket->setup(port)){
		fprintf(stderr, "OscReceiver: Unable to initialise UDP socket: %d %s\n", errno, strerror(errno));
		return;
	}

	//receive_task = std::unique_ptr<std::thread>(new std::thread(&OscReceiver::receive_thread_func, this));
	pthread_create(&receive_thread, NULL, receive_thread_func, this);

}

int OscReceiver::waitForMessage(int timeout){
	int ret = socket->waitUntilReady(true, timeout);
	if (ret == -1){
		fprintf(stderr, "OscReceiver: Error polling UDP socket: %d %s\n", errno, strerror(errno));
		return -1;
	} else if(ret == 1){
		int msgLength = socket->read(inBuffer.data(), inBuffer.size(), false);
		if (msgLength < 0){
			fprintf(stderr, "OscReceiver: Error reading UDP socket: %d %s\n", errno, strerror(errno));
			return -1;
		}
		std::string addr = std::string(socket->getLastRecvAddr()) + ":" + std::to_string(socket->getLastRecvPort());
		pr->init(inBuffer.data(), msgLength);
		if (!pr->isOk()){
			fprintf(stderr, "OscReceiver: oscpkt error parsing received message: %i\n", pr->getErr());
			return ret;
		}
		on_receive(pr->popMessage(), addr.c_str(), onReceiveArg);
	}
	return ret;
}
