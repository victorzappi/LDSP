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

#include "thread_utils.h"
#include "oscpkt.hh"

#include <memory>
#include <vector>
#include <string>
#include <queue> //VIC

class UdpClient;
class AuxTaskNonRT;
namespace oscpkt{
	class Message;
	class PacketWriter;
}

/**
 * \brief OscSender provides functions for sending OSC messages from Bela.
 *
 * Functionality is provided for sending messages with int, float, bool, 
 * std::string and binary blob arguments. Sending a stream of floats is
 * also supported.
 *
 * Uses oscpkt (http://gruntthepeon.free.fr/oscpkt/) underneath
 */
class OscSender{
	public:
		OscSender();
		OscSender(int port, std::string ip_address=std::string("127.0.0.1"));
		~OscSender();
		
        /**
		 * \brief Initialises OscSender
		 *
		 * Must be called once during setup()
		 *
		 * If address is left blank it will default to localhost (127.0.0.1)
		 *
		 * @param port the UDP port number used to send OSC messages
		 * @param address the IP address OSC messages are sent to (defaults to 127.0.0.1)
		 *
		 */
		void setup(int port, std::string ip_address=std::string("127.0.0.1"));
		
		/**
		 * \brief Creates a new OSC message
		 *
		 * @param address the address which the OSC message will be sent to
		 *
		 */
		OscSender &newMessage(std::string address);
		/**
		 * \brief Adds an int argument to a message
		 *
		 * Can optionally be chained with other calls to add(), newMessage() and
		 * send()
		 *
		 * @param payload the argument to be added to the message
		 */
		OscSender &add(int payload);
		/**
		 * \brief Adds a float argument to a message
		 *
		 * Can optionally be chained with other calls to add(), newMessage() and
		 * send()
		 *
		 * @param payload the argument to be added to the message
		 */
		OscSender &add(float payload);
		/**
		 * \brief Adds a string argument to a message
		 *
		 * Can optionally be chained with other calls to add(), newMessage() and
		 * send()
		 *
		 * @param payload the argument to be added to the message
		 */
		OscSender &add(std::string payload);
		/**
		 * \brief Adds a boolean argument to a message
		 *
		 * Can optionally be chained with other calls to add(), newMessage() and
		 * send()
		 *
		 * @param payload the argument to be added to the message
		 */
		OscSender &add(bool payload);
		/**
		 * \brief Adds a binary blob argument to a message
		 *
		 * Copies binary data into a buffer, which is sent as a binary blob.
		 * Can optionally be chained with other calls to add(), newMessage() and
		 * send()
		 *
		 * @param ptr pointer to the data to be sent
		 * @param num_bytes the number of bytes to be sent
		 */
		OscSender &add(void *ptr, size_t num_bytes);
		/**
		 * \brief Sends the message
		 *
		 * After creating a message with newMessage() and adding arguments to it
		 * with add(), the message is sent with this function. It is safe to call
		 * from the audio thread.
		 *
		 */
		void send();
		/**
		 * \brief Sends a message
		 *
		 * Sends the message you pass in, which you will have created
		 * externally. It is safe to call from the audio thread.
		 */
		void send(const oscpkt::Message& extMsg);

		std::unique_ptr<UdpClient> socket;
	
		std::unique_ptr<oscpkt::Message> msg;
		std::unique_ptr<oscpkt::PacketWriter> pw;
		std::unique_ptr<oscpkt::PacketWriter> pw_immediate;

		//void send_task_func(void* buf, int size);

		//VIC

		void sendNow();
		void sendNow(const oscpkt::Message& extMsg);	

		unsigned int prioOrder = LDSPprioOrder_oscSend;
		bool stop = false;
		std::queue<oscpkt::Message> queue;
		pthread_t send_thread;
		
		static void *send_thread_func(void*);
		void empty_queue();
};
