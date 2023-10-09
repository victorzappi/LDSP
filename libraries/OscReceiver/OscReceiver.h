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


#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "oscpkt.hh"
#include "thread_utils.h" //VIC


class UdpServer;
namespace std {
	class thread;
};
/**
 * \brief OscReceiver provides functions for receiving OSC messages in Bela.
 *
 * When an OSC message is received over UDP on the port number passed to
 * OscReceiver::setup() it is passed in the form of an oscpkt::Message
 * to the onreceive callback. This callback, which must be passed to
 * OscReceiver::setup() by the user, is run off the audio thread at
 * non-realtime priority.
 *
 * For documentation of oscpkt see http://gruntthepeon.free.fr/oscpkt/
 */
class OscReceiver{
public:
	OscReceiver();
	OscReceiver(int port, std::function<void(oscpkt::Message* msg, const char* addr, void* arg)> on_receive, void* callbackArg = nullptr);
	~OscReceiver();

	/**
	* \brief Initiliases OscReceiver
	*
	* Must be called once during setup()
	*
	* @param port the port number used to receive OSC messages
	* @param on_receive the callback function which received OSC messages are passed to
	* @param callbackArg an argument to pass to the callback
	*
	*/
	void setup(int port, std::function<void(oscpkt::Message* msg, const char* addr, void* arg)> on_receive, void* callbackArg = nullptr);

private:
	bool lShouldStop = false;

	volatile bool waitingForMessage = false;
	int waitForMessage(int timeout_ms);

	std::unique_ptr<UdpServer> socket;
	//std::unique_ptr<std::thread> receive_task;
	pthread_t receive_thread;


	static void *receive_thread_func(void*);

	std::unique_ptr<oscpkt::PacketReader> pr;
	std::vector<char> inBuffer;

	std::function<void(oscpkt::Message* msg, const char* addr, void* arg)> on_receive;
	void* onReceiveArg = nullptr;

	unsigned int prioOrder = LDSPprioOrder_oscReceive;
};
