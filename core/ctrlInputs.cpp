// This file contains functions adapted from the source code of the AOSP tool 'getevent':
// https://android.googlesource.com/platform/system/core.git/+/refs/heads/master/toolbox/getevent.c
// Modifications carried out by Victor Zappi

#include "ctrlInputs.h"
#include "LDSP.h"
#include "tinyalsaAudio.h" // for LDSPinternalContext
#include "thread_utils.h"

#include <sys/poll.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sstream>
#include <string>	

using std::string;
using std::pair;

typedef vector<pair<string, string>> DevInfo;

LDSPctrlInputsContext ctrlInputsContext;
extern LDSPinternalContext intContext;

bool ctrlInputsVerbose = false;
bool ctrlInputsOff = false;

const char *ctrlInput_devPath = "/dev/input";

extern bool gShouldStop; // extern from tinyalsaAudio.cpp
pthread_t ctrlInput_thread = 0;



struct pollfd *ufds;
char **device_names;
int nfds;

enum {
    PRINT_DEVICE_ERRORS     = 1U << 0,
    PRINT_DEVICE            = 1U << 1,
    PRINT_DEVICE_NAME       = 1U << 2,
    PRINT_DEVICE_INFO       = 1U << 3,
    PRINT_VERSION           = 1U << 4,
    PRINT_POSSIBLE_EVENTS   = 1U << 5,
    PRINT_INPUT_PROPS       = 1U << 6,
    PRINT_HID_DESCRIPTOR    = 1U << 7,

    PRINT_ALL_INFO          = (1U << 8) - 1,

    PRINT_LABELS            = 1U << 16,
};
int print_flags = 0;


int initCtrlInputs();
void initCtrlInputBuffers();
void* ctrlInputs_loop(void*);
void closeCtrlInputDevices();

//VIC not important if unseuccessful, we will still run LDSP if no inputs can be read
void LDSP_initCtrlInputs(LDSPinitSettings* settings)
{
    ctrlInputsVerbose = settings->verbose;
    ctrlInputsOff = settings->ctrlInputsOff;

    if(ctrlInputsOff)
        return;

    if(ctrlInputsVerbose && !ctrlInputsOff)
    {
        printf("\nLDSP_initCtrlInputs()\n");
        // print_flags |= PRINT_DEVICE_ERRORS | PRINT_DEVICE | PRINT_DEVICE_NAME;
        // print_flags |= PRINT_DEVICE_ERRORS | PRINT_DEVICE | PRINT_DEVICE_NAME | PRINT_DEVICE_INFO | PRINT_VERSION;
    }

    // if control inputs are off, we don't init them!
    bool inited = false;
    if(!ctrlInputsOff)
        inited = (initCtrlInputs()==0);
    initCtrlInputBuffers();


     // update context
    intContext.ctrlInputs = ctrlInputsContext.ctrlInBuffer;
    intContext.ctrlInChannels = chn_cin_count;
    intContext.buttonsSupported = ctrlInputsContext.buttonSupported;
    intContext.mtInfo = &ctrlInputsContext.mtInfo;
    //VIC user context is reference of this internal one, so no need to update it
        
    // run thread that monitors devices for events, only if we found at least one device that raises events we are interested into
    if(inited && ctrlInputsContext.inputsCount > 0)
         pthread_create(&ctrlInput_thread, NULL, ctrlInputs_loop, NULL);
}

void LDSP_cleanupCtrlInputs()
{
    if(ctrlInputsVerbose && !ctrlInputsOff)
        printf("LDSP_cleanupCtrlInputs()\n");

    // if control inputs were off, not much to do here
    if(!ctrlInputsOff)
    {
        if(ctrlInput_thread != 0) 
        {
            if(!gShouldStop)
                gShouldStop = true;
            pthread_join(ctrlInput_thread, NULL);
        }

        closeCtrlInputDevices();
    }

    // deallocated ctrl input buffers
    if(ctrlInputsContext.ctrlInBuffer != nullptr)
        delete[] ctrlInputsContext.ctrlInBuffer;
    if(ctrlInputsContext.buttonSupported != nullptr)
        delete[] ctrlInputsContext.buttonSupported;
}



//--------------------------------------------------------------------------------------------------

void* ctrlInputs_loop(void* arg)
{
    struct input_event event;
    int slot = 0;
    unordered_map<unsigned short, unordered_map<int, int> > &event_map = ctrlInputsContext.ctrlInputsEvent_channel;

    // set thread priority
    set_priority(LDSPprioOrder_ctrlInputs, "controlInputs", false);

    // set minimum thread niceness
 	set_niceness(-20, "controlInputs", false);

    while(!gShouldStop) 
    {
        int pollres = poll(ufds, nfds, 1);
        if(pollres>0)
        {
            for(int i=0; i<nfds; i++) 
            {
                if(ufds[i].revents) 
                {
                    if(ufds[i].revents & POLLIN) 
                    {
                        int res = read(ufds[i].fd, &event, sizeof(event));
                        // printf("____event %d, code %d, value %d\n", event.type, event.code, event.value);
                        if(res < (int)sizeof(event)) 
                        {
                            //fprintf(stderr, "could not get event\n");
                            continue;
                        }
                        
                        //VIC unfortunately, this has to be done explicitly
                        if(event.code == ABS_MT_SLOT)
                        {
                            slot = event.value;
                            continue;
                        }
                        
                        // let's check if the event that we received is among those that we want to store
                        // even if we are monitoring only the devices that send the events we are interested into, 
                        // it does not mean that such devices could not raise additional/unwanted events!
                        if( event_map.find(event.type) != event_map.end() )  // only correct types
                        {
                            unordered_map<int, int> &code_map = event_map[event.type];    
                            if (code_map.find(event.code) != code_map.end() ) // only correct codes
                            {
                                // let's restrieve associated ctrl input structure, via the associated channel
                                int chn = code_map[event.code];
                                ctrlInput_struct &ctrlIn = ctrlInputsContext.ctrlInputs[chn];
                                
                                // only multievent ctrl inputs have more slots to store parallel events
                                int idx=0;
                                if(ctrlIn.isMultiInput)
                                     idx = slot;
                                //printf("____event %d, code %d, value %d, chn %d, idx %d, vec %d\n", event.type, event.code, event.value, chn, idx, ctrlIn.value.size());
                                ctrlIn.value[idx]->store(event.value); // atomic store, thread-safe!
                            }
                        }
                    }
                }
            }
        }
    }
    return (void *)0;
}

bool checkEvents(int fd, int print_flags, const char *device, const char *name, DevInfo *devinfo)
{
    bool foundTouchPresent = false;
    bool to_monitor = false;

    uint8_t *bits = NULL;
    ssize_t bits_size = 0;
    //const char* label;
    int res;
    
    //printf("  events:\n");
    // skip EV_SYN since we cannot query its available codes
    for(int event = EV_KEY; event <= EV_MAX; event++) 
    { 
        int count = 0;
        //while(1) //VIC we can never be cautious enough...
        while(!gShouldStop) 
        {
            res = ioctl(fd, EVIOCGBIT(event, bits_size), bits);
            if(res < bits_size)
                break;
            bits_size = res + 16;
            bits = (uint8_t *)realloc(bits, bits_size * 2);
            //if(bits == NULL)
            //    fprintf(stderr, "failed to allocate buffer of size %d\n", (int)bits_size);
        }

        // switch(event) {
        //     case EV_KEY:
        //         label = "EV_KEY";
                
        //         break;
        //     case EV_ABS:
        //         label = "EV_ABS";
                
        //         break;
        //     default:
        //         label = "???";
        // }
        for(int j=0; j <res; j++) 
        {
            for(int k=0; k<8; k++)
            {
                if(bits[j] & 1<<k) 
                {   
                    // on android/linux, buttons can raise both events EV_KEY and events EV_SW
                    if(event!=EV_KEY && event!=EV_SW && event!=EV_ABS)
                        return false;

                    // if(count == 0)
                    //     printf("    %s (%04x):", label, event);
                    // else if((count & (print_flags & PRINT_LABELS ? 0x3 : 0x7)) == 0 || event == EV_ABS)
                    //     printf("\n               ");
                    int code = j * 8 + k;
                    // printf(" %04x", code);
     
                    unordered_map<unsigned short, unordered_map<int, int> > &event_map = ctrlInputsContext.ctrlInputsEvent_channel;
                    if(event_map.find(event) != event_map.end()) 
                    {
                        unordered_map<int, int> &code_map = event_map[event];    

                        if(event != EV_ABS)
                        {
                            // all non multitouch events [e.g., buttons]
                            if(code_map.find(code) != code_map.end())
                            {
                                to_monitor = true; // yes, we will monitor this device, because it raises non multitouch events we are interested into
                                int chn = code_map[code]; // this is the channel that tells us which ctrl input this specific event is associated to
                                // in other words, we can say that the ctrl input is supported, because this device sends the events associated to it!
                                // but more devices can send the same input, so it is possible that this ctrl input was marked as supported already
                                // that's what we check here!
                                if(!ctrlInputsContext.ctrlInputs[chn].supported)
                                {
                                    // if this is the first time we find a device that raises the event associated to this ctrl input, we update our records
                                    ctrlInputsContext.ctrlInputs[chn].supported = true; // for our internal records
                                    ctrlInputsContext.ctrlInputs[chn].isMultiInput = false; // only multi touch ctrl inputs are multi event, cos can receive data from multiple fingers
                                    ctrlInputsContext.inputsCount++;
                                }
                                // store/pass info for verobse printing
                                pair<string, string> info;
                                info.first = device;
                                info.second = name;
                                devinfo[chn].push_back(info);
                            }      
                        }
                        else
                        {
                            struct input_absinfo abs;
                            if(ioctl(fd, EVIOCGABS(j * 8 + k), &abs) == 0) 
                            {
                                //  printf("code %d : value %d, min %d, max %d, fuzz %d, flat %d, resolution %d\n",
                                //      code, abs.value, abs.minimum, abs.maximum, abs.fuzz, abs.flat,
                                //      abs.resolution);

                                // all multitouch events 
                                if(code_map.find(code) != code_map.end())
                                {
                                    to_monitor = true; // yes, we will monitor this device, because it raises multitouch events we are interested into
                                    // same as non multi touch event here
                                    int chn = code_map[code];
                                    if(!ctrlInputsContext.ctrlInputs[chn].supported)
                                    {
                                        ctrlInputsContext.ctrlInputs[chn].supported = true;
                                        ctrlInputsContext.ctrlInputs[chn].isMultiInput = true;  // only difference is taht multi touch ctrl inputs are multi event, cos can receive data from multiple fingers
                                        ctrlInputsContext.inputsCount++;

                                        // update user exposed info for multi ctrl/multitouch
                                        //VIC unfortunately, this has to be done manually
                                        switch(code)
                                        {
                                        case ABS_MT_POSITION_X:
                                                ctrlInputsContext.mtInfo.screenResolution[0] = abs.maximum+1;
                                            break;
                                        case ABS_MT_POSITION_Y:
                                                ctrlInputsContext.mtInfo.screenResolution[1] = abs.maximum+1;
                                            break;
                                        case ABS_MT_TOUCH_MAJOR:
                                                ctrlInputsContext.mtInfo.touchAxisMax = abs.maximum;
                                            break;
                                        case ABS_MT_WIDTH_MAJOR:
                                                ctrlInputsContext.mtInfo.touchWidthMax = abs.maximum;
                                            break;
                                        }
                                    }
                                    // store/pass info for verobse printing
                                    pair<string, string> info;
                                    info.first = device;
                                    info.second = name;
                                    devinfo[chn].push_back(info);
                                }
                                //VIC unfortunately, this has to be done manually
                                else if(code == ABS_MT_SLOT)
                                    ctrlInputsContext.mtInfo.touchSlots = abs.maximum+1;
                            }
                        }
                    }
                    //count++;
                }
            }
        }
        // if(count)
        //     printf("\n");
    }
    free(bits);
    return to_monitor;
}


int openCtrlInputDev(const char *device, int print_flags, DevInfo *devinfo) 
{
    int version;
    int fd;
    int clkid = CLOCK_MONOTONIC;
    struct pollfd *new_ufds;
    char **new_device_names;
    char name[80];
    char location[80];
    char idstr[80];
    struct input_id id;

    fd = open(device, O_RDONLY | O_CLOEXEC);
    if(fd < 0) 
    {
        close(fd);
        if(print_flags & PRINT_DEVICE_ERRORS)
            fprintf(stderr, "could not open %s, %s\n", device, strerror(errno));
        return -1;
    }
    
    if(ioctl(fd, EVIOCGVERSION, &version)) 
    {
        close(fd);
        if(print_flags & PRINT_DEVICE_ERRORS)
            fprintf(stderr, "could not get driver version for %s, %s\n", device, strerror(errno));
        return -1;
    }
    if(ioctl(fd, EVIOCGID, &id)) 
    {
        close(fd);
        if(print_flags & PRINT_DEVICE_ERRORS)
            fprintf(stderr, "could not get driver id for %s, %s\n", device, strerror(errno));
        return -1;
    }
    name[sizeof(name) - 1] = '\0';
    location[sizeof(location) - 1] = '\0';
    idstr[sizeof(idstr) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) 
    {
        //fprintf(stderr, "could not get device name for %s, %s\n", device, strerror(errno));
        name[0] = '\0';
    }
    if(ioctl(fd, EVIOCGPHYS(sizeof(location) - 1), &location) < 1) {
        //fprintf(stderr, "could not get location for %s, %s\n", device, strerror(errno));
        location[0] = '\0';
    }
    if(ioctl(fd, EVIOCGUNIQ(sizeof(idstr) - 1), &idstr) < 1) {
        //fprintf(stderr, "could not get idstring for %s, %s\n", device, strerror(errno));
        idstr[0] = '\0';
    }

    if (ioctl(fd, EVIOCSCLOCKID, &clkid) != 0) {
        //fprintf(stderr, "Can't enable monotonic clock reporting: %s\n", strerror(errno));
        // a non-fatal error
    }

    // check all the events supported by this file/device
    // and see if this is a device we need to poll to get ctrl inputs
    if(!checkEvents(fd, print_flags, device, name, devinfo))
    {
        close(fd);
        return -2;
    }


    new_ufds = (pollfd *)realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL) {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    ufds = new_ufds;
    new_device_names = (char **)realloc(device_names, sizeof(device_names[0]) * (nfds + 1));
    if(new_device_names == NULL) {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    device_names = new_device_names;

    if( ctrlInputsVerbose && (print_flags & PRINT_DEVICE) )
        printf("\tMonitoring control input device %d: %s\n", nfds, device);
    if( ctrlInputsVerbose && (print_flags & PRINT_DEVICE_INFO) )
        printf("\t\tbus: %04x\n"
               "\t\tvendor: %04x\n"
               "\t\tproduct: %04x\n"
               "\t\tversion: %04x\n",
               id.bustype, id.vendor, id.product, id.version);
    if( ctrlInputsVerbose && (print_flags & PRINT_DEVICE_NAME) )
        printf("\t\tname: \"%s\"\n", name);
    if( ctrlInputsVerbose && (print_flags & PRINT_DEVICE_INFO) )
        printf("\t\tlocation: \"%s\"\n"
               "\t\tid: \"%s\"\n", location, idstr);
    if( ctrlInputsVerbose && (print_flags & PRINT_VERSION) )
        printf("\t\tversion: %d.%d.%d\n",
               version >> 16, (version >> 8) & 0xff, version & 0xff);


    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    device_names[nfds] = strdup(device);
    nfds++;
    

    return 0;
}


int openCtrlInputDevices(const char *dirname, int print_flags, DevInfo *devinfo)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    int opened = 0;
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        // count how many ctrl input devices we manage to open and monitor
        if(openCtrlInputDev(devname, print_flags, devinfo)==0)
            opened++;
    }
    closedir(dir);
    
    // signal if cannot open any
    return (opened>0) ? 0 : -1;
}

int initCtrlInputs()
{
     /* disable buffering on stdout */
    setbuf(stdout, NULL);

    DevInfo devinfo[chn_cin_count];

    ctrlInputsContext.mtInfo.touchSlots = 1; // let's set at least 1 touch slot, because some single touch phones do not provide this info
    ctrlInputsContext.mtInfo.touchAxisMax = -1;
    ctrlInputsContext.mtInfo.touchWidthMax = -1;
    ctrlInputsContext.mtInfo.anyTouchSupported = false;

    int res = openCtrlInputDevices(ctrlInput_devPath, print_flags, devinfo);
    if(res < 0) 
    {
        fprintf(stderr, "Opening control input devices - scan dir failed for %s\n", ctrlInput_devPath);
        return -1;
    }

    if(ctrlInputsVerbose)
        printf("Control input list:\n");

    for(int i=0; i<chn_cin_count; i++)
    {
        ctrlInput_struct &ctrlIn = ctrlInputsContext.ctrlInputs[i];
        // prepare ctrl inputs to receive/store a single value or multiple values
        int len = 1;
        if(ctrlIn.isMultiInput)
            len = ctrlInputsContext.mtInfo.touchSlots; // only multievent ctrl inputs have more slots to store parallel events
        ctrlIn.value.resize(len);
        for(auto &v : ctrlIn.value)
        {
            v = std::make_shared< atomic<signed int> >(); // we allocate the atomic container
            if(i < chn_btn_count)
                v->store(0); // init all button values to 0 [not pressed]
            else
                v->store(-1); // init all other values to -1 [not set]
        }

        if(ctrlInputsVerbose)
        {
            string s;

            if(!ctrlIn.supported)
            {
                printf("\t%s not supported\n", LDSP_ctrlInput[i].c_str());
                continue;
            }
            
            printf("\t%s supported!\n", LDSP_ctrlInput[i].c_str());
            if(ctrlIn.isMultiInput) 
            {
                if(ctrlInputsContext.mtInfo.touchSlots > 1)
                    printf("\t\tup to %d parallel inputs (multi touch)\n", ctrlInputsContext.mtInfo.touchSlots);
                else
                    printf("\t\tsingle input (single touch)\n");
            }
            if(devinfo[i].size()==1)
                s = "device";
            else
                s = "devices";
            printf("\t\tAssociated with the following %s:\n", s.c_str());
            for( auto info : devinfo[i])
            {
                if(info.second != "")
                    s = " ("+info.second+")";
                else
                    s = "";
                printf("\t\t\t%s%s\n", info.first.c_str(), s.c_str());
            }
        }
    }

    return 0;
}

void initCtrlInputBuffers()
{
    // allocate buffers for ctrl input values
    // starting from single ctrl/buttons
    int len = chn_btn_count+1; // all single events, includes chn_mt_anyTouch
    ctrlInputsContext.buttonSupported  = new bool[len];
    // update user exposed states
    for(int chn=0; chn<len-1; chn++)
    {
        if(ctrlInputsContext.ctrlInputs[chn].supported)
            ctrlInputsContext.buttonSupported[chn] = true;

    }
    if(ctrlInputsContext.ctrlInputs[len-1].supported)
        ctrlInputsContext.mtInfo.anyTouchSupported = true;

    // then multi ctrl/multitouch
    len +=  (chn_mt_count-1)*ctrlInputsContext.mtInfo.touchSlots; // plus all multi events, excludes chn_mt_anyTouch
    ctrlInputsContext.ctrlInBuffer = new int[len];
    
    for(int i=0; i<len; i++)
        ctrlInputsContext.ctrlInBuffer[i] = 0;
    // user exposed info is set in initCtrlInputs() already
}

void closeCtrlInputDevice(int dev)
{
    if( ctrlInputsVerbose && (print_flags & PRINT_DEVICE) )
        printf("\tClosing control input device %d: %s\n", dev, device_names[dev]);
    free(device_names[dev]);
    close(ufds[dev].fd);
}

void closeCtrlInputDevices()
{
    for(int i = 0; i < nfds; i++) 
        closeCtrlInputDevice(i);
    
    free(ufds);   
}

void readCtrlInputs()
{
    // BE CAREFUL, mapping is manual!
    // and must be the same in LDSP.h buttonRead() and multitouchRead()
    const int offset = chn_btn_count+1;
    // put in context buffer lastest single event values first
    for(int chn=0; chn<offset; chn++) // includes chn_mt_anyTouch
    {
        if(ctrlInputsContext.ctrlInputs[chn].supported)
            ctrlInputsContext.ctrlInBuffer[chn] = ctrlInputsContext.ctrlInputs[chn].value[0]->load();
    }

    int touchSlots = ctrlInputsContext.mtInfo.touchSlots;
    for(int chn=0; chn<chn_mt_count-1; chn++) // excludes chn_mt_anyTouch
    {
        if(!ctrlInputsContext.ctrlInputs[offset+chn].supported)
            continue;

        for(int slot=0; slot<touchSlots; slot++)
            ctrlInputsContext.ctrlInBuffer[offset+chn*touchSlots+slot] = ctrlInputsContext.ctrlInputs[offset+chn].value[slot]->load();
        
    }    
}
