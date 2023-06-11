#include "LDSP.h"
#include "PdBase.hpp"

#define PD_MINIMUM_BLOCK_SIZE 64
#define PD_AUDIO_IN_CHANNELS 2

//VIC is this needed?
#define PD_MT_RECEIVE_OBJ_PER_SLOT 1

pd::PdBase lpd;


LDSPcontext * g_ctx;

bool gSensorsEnabled;
bool gControlInEnabled; 
bool gSendMultiTouch;
bool gSendBtnInputs;
std::string pd_enableMtObj = "ldsp_enable_multitouch";
std::string pd_getMtInfoObj = "ldsp_get_mt_info";
std::string pd_mtInfoObj = "ldsp_mt_info";
std::string pd_flashlightObj = "ldsp_flashlight";
std::string pd_backlightObj = "ldsp_backlight";
std::string pd_vibrationObj = "ldsp_vibration";
std::string pd_touchObjPrefix = "ldsp_touch_";
std::string pd_enableBtnObj = "ldsp_enable_buttons";
std::string pd_btnInputObjPrefix = "ldsp_btn_";
std::string pd_screenObj = "ldsp_screen";


class LDSP_EventHandler : public pd::PdReceiver 
{
    // Redirect print statements to console
    void print(const std::string &message) {
        printf("Received print msg:\n - %s\n", message.c_str());
    }

    // Redirect bangs to console
    void receiveBang(const std::string &dest) {
        if (dest == pd_enableMtObj) {
            gSendMultiTouch = true;
        }
        else if (dest == pd_enableBtnObj) {
            gSendBtnInputs = true;
        }
        else if (dest == pd_getMtInfoObj) {
            if (gControlInEnabled) {
                pd::List mtInfoList;
                mtInfoList.addFloat(g_ctx->mtInfo->screenResolution[0]);
                mtInfoList.addFloat(g_ctx->mtInfo->screenResolution[1]);
                mtInfoList.addFloat(g_ctx->mtInfo->touchSlots);
                //mtInfoList.addFloat(g_ctx->mtInfo->touchAxisMax); // for now this are not needed, for only a subset of mt data is passed to the patch
                //mtInfoList.addFloat(g_ctx->mtInfo->touchWidthMax);
                lpd.sendList(pd_mtInfoObj, mtInfoList);
            }
        }
        else {
            printf("Received Bang from %s\n", dest.c_str());
        }
    }

    // Redirect floats to console
    void receiveFloat(const std::string &dest, float num) {
        if (dest == pd_flashlightObj) {
            if (g_ctx->ctrlOutputsSupported[chn_cout_flashlight]) {
                ctrlOutputWrite(g_ctx, chn_cout_flashlight, num);
            }
        }
        else if (dest == pd_vibrationObj) {
            if (g_ctx->ctrlOutputsSupported[chn_cout_vibration]) {
                ctrlOutputWrite(g_ctx, chn_cout_vibration, num);
            }
        }
        else if (dest == pd_backlightObj) {
            if (g_ctx->ctrlOutputsSupported[chn_cout_lcdBacklight]) {
                ctrlOutputWrite(g_ctx, chn_cout_lcdBacklight, num);
            }
        }
        else if (dest == pd_screenObj) {
            if(num <= 0)
                screenSetState(false); // set screen off
            else if(num == 1)
                screenSetState(true, 1, false); // set screen on 
            else if(num >= 2)
                screenSetState(true, 1, true); // set screen on and keep it on
        }
        else {
            std::cout << dest << ": " << std::to_string(num) << std::endl;
        }
                
    }

    // Redirect symbols to console
    void receiveSymbol(const std::string &dest, const std::string &symbol) {
        printf("Received symbol from %s:\n - %s\n", dest.c_str(), symbol.c_str());
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
            "Received Message from %s:\n - %s\n - %s\n - %s\n", 
            dest.c_str(), msg.c_str(), list.toString().c_str(), list.types().c_str()
        );
    }
};

class LDSP_MidiHandler : public pd::PdMidiReceiver
{
    // pd midi receiver callbacks
	void receiveNoteOn(const int channel, const int pitch, const int velocity) {
        printf("LDSP Host Received Midi Message: \n");
        printf(" - Note:     %d\n", pitch);
        printf(" - Velocity: %d\n", velocity);
        printf(" - Channel:  %d\n", channel);
    }

    // receive a MIDI control change
    void receiveControlChange(
                            const int channel,
                            const int controller,
                            const int value) {
        printf("LDSP Host Received Midi Control Change: \n");
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

// Button Input
std::vector<float> buttonInputStates;
std::vector<std::string> pdButtonNames;


// MultiTouch 
#define PD_MULTITOUCH_INPUTS 4
int gNumTouchSlots;

std::vector<float> multiTouchState;
int mtStateBufferSize; 
float anyTouchState;

multiTouchInputChannel pdMTChannelMappings[PD_MULTITOUCH_INPUTS];
// Be careful adding more channels, the channel names and mappings MUST be manually updated too
enum pdMTChannel {
    pd_mt_x,
    pd_mt_y,
    pd_mt_id,
    pd_mt_pressure
};



bool setup(LDSPcontext *context, void *userData)
{
    // Store a reference to context so we can access it during receive hooks
    g_ctx = context;


    lpd.setReceiver(&eventHandler);
    lpd.setMidiReceiver(&midiHandler);

    if (context->audioFrames < PD_MINIMUM_BLOCK_SIZE) {
        std::cout << "Setup Failed: The minimum pure-data block size is " << PD_MINIMUM_BLOCK_SIZE  << ", " << context->audioFrames << " was given." << std::endl;
        return false;
    }


    // Check if sensors are enabled
    gSensorsEnabled = false;
    for (int i = 0; i < context->sensorChannels; i++) {
        if (context->sensorsSupported[i]) {
            gSensorsEnabled = true;
        }
    }

    // Check if control inputs are on globaly (buttons + multitouch)
    gControlInEnabled = context->ctrlInChannels > 0;

    if (gSensorsEnabled) {
        std::cout << "Sensor inputs are on" << std::endl;
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
        std::cout << "Sensor inputs are off" << std::endl;
        // Process audio as normal if not using sensors
        pdInChannels = context->audioInChannels;
    }

    if (!lpd.init(pdInChannels, context->audioOutChannels, context->audioSampleRate, true)) {
        std::cout << "Failed to initialize libpd instance" << std::endl;
    }

    // Listen to control output sends
    lpd.subscribe(pd_flashlightObj);
    lpd.subscribe(pd_backlightObj);
    lpd.subscribe(pd_vibrationObj);
    // Listen to screen controls
    lpd.subscribe(pd_screenObj);
    // List to bangs sent to query mt info object
    lpd.subscribe(pd_getMtInfoObj);
    // Listen to bangs sent to the enable multitouch object
    lpd.subscribe(pd_enableMtObj);
    // Listen to bangs sent to the enable buttons object
    lpd.subscribe(pd_enableBtnObj);
    



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



    if (gControlInEnabled) {    
        std::cout << "Control Inputs are on" << std::endl;
        // Initialize button input resource
        buttonInputStates.resize(chn_btn_count);
        pdButtonNames.resize(chn_btn_count);
        // Be careful, these need to be manually updated!
        // There might be a better way to do these mappings
        pdButtonNames[chn_btn_power] = "power";
        pdButtonNames[chn_btn_volDown] = "volDown";
        pdButtonNames[chn_btn_volUp] = "volUp";


        // Initliaze multitouch resources
        gNumTouchSlots = context->mtInfo->touchSlots;
        //printf("%d\n", gNumTouchSlots);
        // Set initial multitouch states
        mtStateBufferSize = PD_MULTITOUCH_INPUTS*gNumTouchSlots;
        multiTouchState.resize(mtStateBufferSize);
        // init
        for (int i = 0; i < multiTouchState.size(); i++)
            multiTouchState[i] = -1.0;
        anyTouchState = 0.0;

        // Map the position of each value in the pd list
        // to its respective ldsp mt channel number
        pdMTChannelMappings[pd_mt_x] = chn_mt_x;
        pdMTChannelMappings[pd_mt_y] = chn_mt_y;
        pdMTChannelMappings[pd_mt_id] = chn_mt_id;
        pdMTChannelMappings[pd_mt_pressure] = chn_mt_pressure;
    }
    else {
        std::cout << "Control Inputs are off" << std::endl;
    }

    // MultiTouch is not passed into pure-data by default,
    // The user has to enable it by sending a bang to the `pd_enableMtObj` object
    gSendMultiTouch = false;
    gSendBtnInputs = false;


    // Turn on audio
    lpd.computeAudio(true);
    pdBlockSize = libpd_blocksize(); 

    return true;
}


void render(LDSPcontext *context, void *userData)
{

    int pdBlocks = context->audioFrames / pdBlockSize;

    // If control inputs are off globally (via command line), ignore them completely
    // even if user tries to enable them from inside pure-data 
    // trying to access them will kill the loop
    if (gControlInEnabled) {

        // send button inputs to pure data if user has enabled them
        if (gSendBtnInputs) {
            for (int i = 0; i < chn_btn_count; i++) {
                int newButtonVal = buttonRead(context, (btnInputChannel) i);
                if (newButtonVal != buttonInputStates[i] && newButtonVal != -1) {
                    lpd.sendFloat(pd_btnInputObjPrefix + pdButtonNames[i], newButtonVal);
                    buttonInputStates[i] = newButtonVal;
                }
            }
        }


        // send multitouch inputs to pure data if user has enabled them
        if (gSendMultiTouch) {

            // Send anyTouch through to PD if it changed since last render call
            float anyTouch = multiTouchRead(context, chn_mt_anyTouch, 0);
            if (anyTouch != anyTouchState && anyTouch != -1) {
                lpd.sendFloat("ldsp_mt_anyTouch", anyTouch);
                anyTouchState = anyTouch;
            }

            int numElements;
            // Send each touch slot to its respective pd receive element
            for (int slot = 0; slot < gNumTouchSlots; slot++) {
                int chunkStart = slot*PD_MULTITOUCH_INPUTS;
                pd::List touchList;
                numElements = 0;
                for (int i = 0; i < PD_MULTITOUCH_INPUTS; i++) {
                    float newVal = multiTouchRead(context, pdMTChannelMappings[i], slot); 
                    touchList.addFloat(newVal);
                    if (newVal != multiTouchState[chunkStart+i]  && (newVal != -1 || i == (int)pd_mt_id) ) { // no -1 values, except for id=-1; it is a good way to signal that there is no touch in that slot
                        numElements++;             
                        multiTouchState[chunkStart+i] = newVal;
                    }
                }
                // Don't send the list if nothing has changed, 
                // but send the whole list if at least one element has changed
                if (numElements > 0) {
                    std::string destObj = pd_touchObjPrefix + std::to_string(slot);
                    lpd.sendList(destObj, touchList);
                }
            }
        }
    }

    if (gSensorsEnabled) {
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