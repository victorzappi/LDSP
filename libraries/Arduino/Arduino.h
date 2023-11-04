/*
 * [2-Clause BSD License]
 *
 * Copyright 2022 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ARDUINO_H_
#define ARDUINO_H_

#include <string>
#include <memory> // for unique_ptr
#include <atomic>
#include "thread_utils.h"
#include "serial/serial.h"

// arduino boards often connect to this port
#define ARDUINO_PORT "/dev/ttyACM0"

constexpr int output_buffer_size = 100; // this is the number of outputs that can be written in a row without the risk of skipping transmission

class Arduino {
public:
    static void listPorts();

    Arduino();
    Arduino(int numOfInputs);
    ~Arduino();
    
    inline bool setNumOfInputs(int numOfInputs);

    bool open(int baudrate=9600, std::string port=ARDUINO_PORT);
    void close();

    int read(int* values=nullptr); // we typically receive integers 0-1023
    void write(float value); // sending floats it's easy and Arduino can then turn them into different types
    void writeNow(float value); // this is faster but more prone to i/o locks if high reading at the same time
 
private:
    static std::unique_ptr<serial::Serial> my_serial;
    static bool shouldStop;

    pthread_t arduinoRead_thread;
    static std::vector< std::unique_ptr< std::atomic<unsigned int> > > inputs; // two vectors of atomics where to store incoming data in a thread-safe manner
    // we have to use pointers though, because vector cannot deal with atomics directly
    static std::atomic<unsigned int> readBufferIdx;
    static unsigned int _numOfInputs;
    static unsigned int maxInPacketSize;
    
    pthread_t arduinoWrite_thread;
    static std::atomic<float> outputs[output_buffer_size];
    static int outputs_readPtr;
    static std::atomic<int> outputs_writePtr;

    static void* readLoop(void*);
    static void* writeLoop(void*);
};



bool Arduino::setNumOfInputs(int numOfInputs)
{
    if(my_serial != nullptr)
	{
		if(!my_serial->isOpen())
            return false; // too late to set number of inputs!
    }

    _numOfInputs = numOfInputs;

    return true;
}




#endif /* ARDUINO_H_ */

