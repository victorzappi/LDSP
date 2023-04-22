#include "LDSP.h"
#include "PdBase.hpp"

#define PD_MINIMUM_BLOCK_SIZE 64
#define PD_AUDIO_IN_CHANNELS 2

pd::PdBase lpd;

class LDSP_EventHandler : public pd::PdReceiver 
{
    // Redirect print statements to console
    void print(const std::string &message) {
        printf("Recieved print msg:\n - %s\n", message.c_str());
    }

    // Redirect bangs to console
    void receiveBang(const std::string &dest) {
        printf("Recieved Bang from %s\n", dest.c_str());
    }

    // Redirect floats to console
    void receiveFloat(const std::string &dest, float num) {
        printf("Recieved Float from %s:\n - %f\n", dest.c_str(), num);
    }

    // Redirect symbols to console
    void receiveSymbol(const std::string &dest, const std::string &symbol) {
        printf("Recieved symbol from %s:\n - %s\n", dest.c_str(), symbol.c_str());
    }

    // Redirect lists to console
    void receiveList(const std::string &dest, const pd::List &list) {
        for(int i = 0; i < list.len(); ++i) {
            if(list.isFloat(i)) {
                std::cout << list.getFloat(i);
            }
            else if(list.isSymbol(i)) {
                std::cout << list.getSymbol(i);
            }
            if(i < list.len()-1) {
                std::cout << " ";
            }
        }
    }

    // Redirect messages to console
    void receiveMessage(const std::string &dest, const std::string &msg, const pd::List &list) {
        printf(
            "Recieved Message from %s:\n - %s\n - %s\n - %s\n", 
            dest.c_str(), msg.c_str(), list.toString().c_str(), list.types().c_str()
        );
    }
};

class LDSP_MidiHandler : public pd::PdMidiReceiver
{
    // pd midi receiver callbacks
	void receiveNoteOn(const int channel, const int pitch, const int velocity) {
        printf("LDSP Host Recieved Midi Message: \n");
        printf(" - Note:     %d\n", pitch);
        printf(" - Velocity: %d\n", velocity);
        printf(" - Channel:  %d\n", channel);
    }

    // receive a MIDI control change
    void receiveControlChange(
                            const int channel,
                            const int controller,
                            const int value) {
        printf("LDSP Host Recieved Midi Control Change: \n");
        printf(" - Note:     %d\n", channel);
        printf(" - Velocity: %d\n", controller);
        printf(" - Channel:  %d\n", value);
    }

};

LDSP_EventHandler eventHandler;
LDSP_MidiHandler midiHandler;

int pdBlockSize;
int pdInChannels;

float * inBuffer;
int inBufferSize;

bool sensorsEnabled;

bool setup(LDSPcontext *context, void *userData)
{


    lpd.setReceiver(&eventHandler);
    lpd.setMidiReceiver(&midiHandler);

    if (context->audioFrames < PD_MINIMUM_BLOCK_SIZE) {
        std::cout << "Setup Failed: The minimum pure-data block size is " << PD_MINIMUM_BLOCK_SIZE  << ", " << context->audioFrames << " was given." << std::endl;
        return false;
    }

    // Check if sensors are enabled
    sensorsEnabled = false;
    for (int i = 0; i < context->sensorChannels; i++) {
        if (context->sensorsState[i] != -1) {
            sensorsEnabled = true;
        }
    }

    if (sensorsEnabled) {
        // Force 2 audio channels, even if R channel isn't used, 
        // so that users won't have to refactor adc channel routing if switching between the two
        pdInChannels = PD_AUDIO_IN_CHANNELS+context->sensorChannels;
        // Try to allocate memory to hold our input and output buffers that will be used to pass sensor+audio data to pure-data
        inBufferSize = context->audioFrames*pdInChannels;
        printf("Pure data input buffer size is: %d\n" , inBufferSize);
        inBuffer = (float *) malloc(inBufferSize * sizeof(float));
        if (inBuffer == NULL) {
            std::cout << "Failed to allocate memory for input channels" << std::endl;
            return false;
        }
    }
    else {
        // Process audio as normal if not using sensors
        pdInChannels = context->audioInChannels;
    }

    if (!lpd.init(pdInChannels, context->audioOutChannels, context->audioSampleRate, true)) {
        std::cout << "Failed to initialize libpd instance" << std::endl;
    }

    // Subscribe to any messages sent to "ldsp_out"
    lpd.subscribe("ldsp_out");

    // turn on audio
    lpd.computeAudio(true);


    // Open and Load Patch
    char patchName[] = "_main.pd";
    char patchPath[] = ".";
    pd::Patch patch = lpd.openPatch(patchName, patchPath);
    if (!patch.isValid()) {
        std::cout << "Failed to Load PD Path " << patch.filename() << std::endl;
    }
    else {
        std::cout << "Loaded PD Patch " << patch.filename() << std::endl;
    }


    pdBlockSize = libpd_blocksize();

    return true;
}

void render(LDSPcontext *context, void *userData)
{

    int pdBlocks = context->audioFrames / pdBlockSize;

    if (sensorsEnabled) {
        int audioChunkStart, sensorChunkStart;
        for (int i = 0; i < context->audioFrames; i++) {

            // Fill the first N channels with audio data (force 2 even if unused)
            audioChunkStart = i*pdInChannels;
            for (int audioChannel = 0; audioChannel < context->audioInChannels; audioChannel++) {
                inBuffer[audioChunkStart+audioChannel] = audioRead(context, i, audioChannel);
            }

            // Fill the rest of the channels with the sensor data
            sensorChunkStart = audioChunkStart+PD_AUDIO_IN_CHANNELS;
            for (int sensorIdx = 0; sensorIdx < context->sensorChannels; sensorIdx++) {
                inBuffer[sensorChunkStart+sensorIdx] = sensorRead(context, (sensorChannel) sensorIdx); 
            }

        }

        lpd.processFloat(pdBlocks, inBuffer, context->audioOut);
    }
    else {
        // Process audio as normal if not using sens
        lpd.processFloat(pdBlocks, context->audioIn, context->audioOut);
    }

    lpd.receiveMessages();
    lpd.receiveMidi();

}

void cleanup(LDSPcontext *context, void *userData)
{

    
}