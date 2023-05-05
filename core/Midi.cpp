// This code is based on the code credited below, but it has been modified
// further by Victor Zappi

/*
 * [2-Clause BSD License]
 *
 * Copyright 2017 Victor Zappi
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

/*
 * Midi.cpp
 *
 *  Created on: 15 Jan 2016
 *      Author: Giulio Moro
 *
 *      Taken from Bela source code
 *
 * Modified on: 12 Sept 2016
 *  	Author: Arvind Vasuvedan and Victor Zappi
 *
 *  	Bela: an embedded platform for ultra-low latency audio and sensor processing
 *      http://bela.io
 *
 * 		A project of the Augmented Instruments Laboratory within the
 * 		Centre for Digital Music at Queen Mary University of London.
 * 		http://www.eecs.qmul.ac.uk/~andrewm
 *
 * 		(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
 *      Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
 * 		Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.
 *
 * 		The Bela software is distributed under the GNU Lesser General Public License
 * 		(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
 *
 * 		The Bela hardware designs are released under a Creative Commons
 * 		Attribution-ShareAlike 3.0 Unported license (CC BY-SA 3.0). Details here:
 * 		https://creativecommons.org/licenses/by-sa/3.0/
 *
 */

#include "Midi.h"
#include <fcntl.h>
#include <errno.h>

#define kMidiInput 0
#define kMidiOutput 1


midi_byte_t midiMessageStatusBytes[midiMessageStatusBytesLength]=
{
	0x80,
	0x90,
	0xA0,
	0xB0,
	0xC0,
	0xD0,
	0xE0,
	0
};

unsigned int midiMessageNumDataBytes[midiMessageStatusBytesLength]={2, 2, 2, 2, 1, 1, 2, 0};

//AV: Change of this
//bool Midi::staticConstructed;
bool Midi::inputThreadRunning;
bool Midi::outputThreadRunning;
pthread_t Midi::inputThread;
pthread_t Midi::outputThread;
bool Midi::shouldStop = false;
int Midi::midiObjectNumber = 0;
//VIC
int	Midi::prioReadThread  = -1;
int	Midi::prioWriteThread  = -1;
int Midi::verbose = 0;


//AV: Removed Auxil
//AuxiliaryTask Midi::midiInputTask;
//AuxiliaryTask Midi::midiOutputTask;
std::vector<Midi *> Midi::objAddrs[2];

int MidiParser::parse(midi_byte_t* input, unsigned int length){
	unsigned int consumedBytes = 0;
	for(unsigned int n = 0; n < length; n++){
		consumedBytes++;
		if(waitingForStatus == true){
			int statusByte = input[n];
			MidiMessageType newType = kmmNone;
			if (statusByte >= 0x80){//it actually is a status byte
				for(int n = 0; n < midiMessageStatusBytesLength; n++){ //find the statusByte in the array
					if(midiMessageStatusBytes[n] == (statusByte&0xf0)){
						newType = (MidiMessageType)n;
						break;
					}
				}
				elapsedDataBytes = 0;
				waitingForStatus = false;
				messages[writePointer].setType(newType);
				messages[writePointer].setChannel((midi_byte_t)(statusByte&0xf));
				consumedBytes++;
			} else { // either something went wrong or it's a system message
				continue;
			}
		} else {
			messages[writePointer].setDataByte(elapsedDataBytes, input[n]);
			elapsedDataBytes++;
			if(elapsedDataBytes == messages[writePointer].getNumDataBytes()){
				// done with the current message
				// call the callback if available
				if(isCallbackEnabled() == true){
					messageReadyCallback(getNextChannelMessage(), callbackArg);
				}
				waitingForStatus = true;
				writePointer++;
				if(writePointer == messages.size()){
					writePointer = 0;
				}
			}
		}
	}

	return consumedBytes;
};


Midi::Midi(){
	outputPort = -1;
	inputPort = -1;
	inputParser = 0;
	parserEnabled = false;
	outputBytesReadPointer = -1;
	outputBytesWritePointer = -1;
	size_t inputBytesInitialSize = 1000;
	inputBytes.resize(inputBytesInitialSize);
	outputBytes.resize(inputBytesInitialSize);
	inputBytesWritePointer = 0;
	inputBytesReadPointer = inputBytes.size() - 1;
	//AV: Static Constructor has been replaced
	/*
	if(!staticConstructed){
		staticConstructor();
	}
	*/
	midiObjectNumber++;
}

/*
void Midi::staticConstructor(){
	staticConstructed = true;
//	midiInputTask = Bela_createAuxiliaryTask(Midi::midiInputLoop, 50, "MidiInput");
//	midiOutputTask = Bela_createAuxiliaryTask(Midi::midiOutputLoop, 50, "MidiOutupt");
}
*/

Midi::~Midi(){
	midiObjectNumber--;

	if (midiObjectNumber <= 0){
		shouldStop = true;
	}
}

void Midi::enableParser(bool enable){
	if(enable == true){
		delete/*[]*/ inputParser;
		inputParser = new MidiParser();
		parserEnabled = true;
	} else {
		delete/*[]*/ inputParser;
		parserEnabled = false;
	}
}

void *Midi::midiInputLoop(void *){
	//AV: Set Priority only
	set_priority(prioReadThread, Midi::verbose);
	for(unsigned int n = 0; n < objAddrs[kMidiInput].size(); n++){
		objAddrs[kMidiInput][n] -> readInputLoop();
	}
	return (void *)0;
}

void *Midi::midiOutputLoop(void *){
	//AV: Set Priority only
	set_priority(prioWriteThread, Midi::verbose);
	for(unsigned int n = 0; n < objAddrs[kMidiOutput].size(); n++){
		objAddrs[kMidiOutput][n] -> writeOutputLoop();
	}
	return (void *)0;
}

void Midi::readInputLoop(){
	while(!shouldStop){
		int maxBytesToRead = inputBytes.size() - inputBytesWritePointer;
		int ret = read(inputPort, &inputBytes[inputBytesWritePointer], sizeof(midi_byte_t)*maxBytesToRead);
		if(ret < 0){
			if(errno != EAGAIN){ // read() would return EAGAIN when no data are available to read just now
				//AV: printf change
				printf("Error while reading midi %d\n", errno);
			}
			usleep(1000);
			continue;
		}
		inputBytesWritePointer += ret;
		if(inputBytesWritePointer == inputBytes.size()){ //wrap pointer around
			inputBytesWritePointer = 0;
		}

		if(parserEnabled == true && ret > 0){ // if the parser is enabled and there is new data, send the data to it
			int input;
			while((input=_getInput()) >= 0){
				midi_byte_t inputByte = (midi_byte_t)(input);
				inputParser->parse(&inputByte, 1);
			}
		}
		if(ret < maxBytesToRead){ //no more data to retrieve at the moment
			usleep(1000);
		} // otherwise there might be more data ready to be read (we were at the end of the buffer), so don't sleep
	}
	printf("Input Loop Ended. ");
	inputThreadRunning = false;
}

void Midi::writeOutputLoop(){
	while(!shouldStop){
		usleep(1000);
		int length = outputBytesWritePointer - outputBytesReadPointer;
		if(length < 0){
			length = outputBytes.size() - outputBytesReadPointer;
		}
		if(length == 0){ //nothing to be written
			continue;
		}
		int ret;
		ret = write(outputPort, &outputBytes[outputBytesReadPointer], sizeof(midi_byte_t)*length);
		if(ret < 0){ //error occurred
			//AV: printf change
			printf("error occurred while writing: %d\n", errno);
			usleep(10000); //wait before retrying
			continue;
		}
	}
	printf("Output Loop Ended. ");
	outputThreadRunning = false;
}
int Midi::readFrom(const char* port,/*VIC added*/ int prio){
	objAddrs[kMidiInput].push_back(this);
	inputPort = open(port, O_RDONLY | O_NONBLOCK | O_NOCTTY);
	if(inputPort < 0){
		return -1;
	} else {
		printf("Reading from Midi port %s\n", port);
		//AV: Commented Auxiliary Task
		//Bela_scheduleAuxiliaryTask(midiInputTask);
		if(!inputThreadRunning) {
			prioReadThread = prio; //VIC
			pthread_create(&inputThread, NULL, midiInputLoop, NULL);
			inputThreadRunning = true;
		}
		return 1;
	}
}

int Midi::writeTo(const char* port,/*VIC added*/ int prio){
	objAddrs[kMidiOutput].push_back(this);
	outputPort = open(port, O_WRONLY, 0);
	if(outputPort < 0){
		return -1;
	} else {
		printf("Writing to Midi port %s\n", port);
		//AV: Commented Aux
		//Bela_scheduleAuxiliaryTask(midiOutputTask);
		if(!outputThreadRunning) {
			prioWriteThread = prio; //VIC
			pthread_create(&outputThread, NULL, midiOutputLoop, NULL);
			outputThreadRunning = true;
		}
		return 1;
	}
}

int Midi::_getInput(){
	if(inputPort < 0)
		return -2;
	if(inputBytesReadPointer == inputBytesWritePointer){
		return -1; // no bytes to read
	}
	midi_byte_t inputMessage = inputBytes[inputBytesReadPointer++];
	if(inputBytesReadPointer == inputBytes.size()){ // wrap pointer
		inputBytesReadPointer = 0;
	}
	return inputMessage;
}

int Midi::getInput(){
	if(parserEnabled == true) {
		return -3;
	}
	return _getInput();
}

MidiParser* Midi::getParser(){
	if(parserEnabled == false){
		return 0;
	}
	return inputParser;
};

int Midi::writeOutput(midi_byte_t byte){
	return writeOutput(&byte, 1);
}

int Midi::writeOutput(midi_byte_t* bytes, unsigned int length){
	int ret = write(outputPort, bytes, length);
	if(ret < 0)
		return -1;
	else
		return 1;
}

MidiChannelMessage::MidiChannelMessage(){
	_statusByte = -1;
	_type 	 	= kmmNoteOff;
	_channel 	= -1;
};
MidiChannelMessage::MidiChannelMessage(MidiMessageType type){
	setType(type);
};
MidiChannelMessage::~MidiChannelMessage(){};
MidiMessageType MidiChannelMessage::getType(){
	return _type;
};
int MidiChannelMessage::getChannel(){
	return _channel;
};
//int MidiChannelMessage::set(midi_byte_t* input);
//
//int MidiControlChangeMessage ::getValue();
//int MidiControlChangeMessage::set(midi_byte_t* input){
//	channel = input[0];
//	number = input[1];
//	value = input[2];
//	return 3;
//}

//int MidiNoteMessage::getNote();
//int MidiNoteMessage::getVelocity();

//midi_byte_t MidiProgramChangeMessage::getProgram();