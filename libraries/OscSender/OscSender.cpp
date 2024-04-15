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


/***** OscSender.cpp *****/
#include "OscSender.h"
#include "libraries/UdpClient/UdpClient.h"
#include "oscpkt.hh"

#include <unistd.h> // usleep

#define OSCSENDER_MAX_ARGS 1024
#define OSCSENDER_MAX_BYTES 65536

constexpr unsigned int OscSenderSleepUs = 5000;


OscSender::OscSender(){}
OscSender::OscSender(int port, std::string ip_address){
	setup(port, ip_address);
}
OscSender::~OscSender() {
	stop = true;
	if(send_thread != 0)
		pthread_join(send_thread, NULL);
}

void OscSender::setup(int port, std::string ip_address){

    pw = std::unique_ptr<oscpkt::PacketWriter>(new oscpkt::PacketWriter());
    msg = std::unique_ptr<oscpkt::Message>(new oscpkt::Message());
    msg->reserve(OSCSENDER_MAX_ARGS, OSCSENDER_MAX_BYTES);

    socket = std::unique_ptr<UdpClient>(new UdpClient());
	socket->setup(port, ip_address.c_str());

	//send_task = std::unique_ptr<AuxTaskNonRT>(new AuxTaskNonRT());
	//send_task->create(std::string("OscSndrTsk_") + ip_address + std::to_string(port), [this](void* buf, int size){send_task_func(buf, size); });

	pw_immediate = std::unique_ptr<oscpkt::PacketWriter>(new oscpkt::PacketWriter());

	pthread_create(&send_thread, NULL, send_thread_func, this);
}

OscSender &OscSender::newMessage(std::string address){
	msg->init(address);
	return *this;
}

OscSender &OscSender::add(int payload){
	msg->pushInt32(payload);
	return *this;
}
OscSender &OscSender::add(float payload){
	msg->pushFloat(payload);
	return *this;
}
OscSender &OscSender::add(std::string payload){
	msg->pushStr(payload);
	return *this;
}
OscSender &OscSender::add(bool payload){
	msg->pushBool(payload);
	return *this;
}
OscSender &OscSender::add(void *ptr, size_t num_bytes){
	msg->pushBlob(ptr, num_bytes);
	return *this;
}

void OscSender::send(){
	//pw->init().addMessage(*msg);
	//send_task->schedule(pw->packetData(), pw->packetSize());
	queue.push(*msg);
}

void OscSender::send(const oscpkt::Message& extMsg){
	//pw->init().addMessage(extMsg);
	//send_task->schedule(pw->packetData(), pw->packetSize());
	queue.push(extMsg);
}


void OscSender::sendNow(){
	pw_immediate->init().addMessage(*msg);
	socket->send(pw_immediate->packetData(), pw_immediate->packetSize());
}

void OscSender::sendNow(const oscpkt::Message& extMsg){
	pw_immediate->init().addMessage(extMsg);
	socket->send(pw_immediate->packetData(), pw_immediate->packetSize());
}

void *OscSender::send_thread_func(void* ptr){
	OscSender *instance = (OscSender*)ptr;
    
	// set thread priority
	set_priority(instance->prioOrder, "OSCsender", false);

	// set minimum thread niceness
 	set_niceness(-20, "OSCsender", false);


    while(!instance->stop){
        instance->empty_queue();
        usleep(OscSenderSleepUs);
    }
    return (void *)0;
}

//TODO switch to atomic non-blocking circular buffer, like in arduino
void OscSender::empty_queue(){
    if (!queue.empty()){
        pw->init().startBundle();
        while(!queue.empty()){
            pw->addMessage(queue.front());
            queue.pop();
        }
        pw->endBundle();
        socket->send(pw->packetData(), pw->packetSize());
    }
}


// void OscSender::send_task_func(void* buf, int size){
// 	OscSender* instance = this;
// 	instance->socket->send(buf, size);
// }
