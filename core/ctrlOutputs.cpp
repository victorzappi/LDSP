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
#include "ctrlOutputs.h"
#include "LDSP.h"
#include "tinyalsaAudio.h" // for LDSPinternalContext
#include "priority_utils.h"

#include <sstream> // for ostream
#include <dirent.h> // to search for files
#include <memory> // smart pointers
#include <regex> // replace substring
#include <cstdlib> // for system()
#include <unistd.h> // for usleep()



using std::ifstream;
using std::ostringstream;
using std::shared_ptr;
using std::make_shared;
using std::regex;
using std::regex_replace;

#define CTRL_BASE_PATH "/sys/class"

bool ctrlOutputsVerbose = false;
bool ctrlOutputsOff = false;


LDSPctrlOutputsContext ctrlOutputsContext;
extern LDSPinternalContext intContext;


// screen control
screenCtrlsCommands screenCmds;
const char *pwrBtn_input_cmd = "input keyevent 26";
const char *tap_input_cmd = "input tap 1 1";
extern bool gShouldStop; // extern from tinyalsaAudio.cpp
pthread_t screenCtl_thread;
constexpr unsigned int ScreenCtrlLoopPrioOrder = 50;
bool initialScreenState;
int nextScreenState = -1;
float nextBrightness = 0;
bool nextStayOn = false;


int initCtrlOutputs(string **ctrlOutputsFiles);
void cleanupCtrlOutputs(ctrlout_struct *ctrlOutputs, int numOfOutputs);
void probeScreenCommands();
bool isScreenOn();
void setScreen(float brightness);
void* screenCtrl_loop(void* arg);




int LDSP_initCtrlOutputs(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
    ctrlOutputsVerbose = settings->verbose;
    ctrlOutputsOff = settings->ctrlOutputsOff;


    if(ctrlOutputsVerbose && !ctrlOutputsOff)
        printf("\nLDSP_initCtrlOutputs()\n");

    // if control outputs are off, we don't init them!
    if(!ctrlOutputsOff)
    {
        int retVal = initCtrlOutputs(hwconfig->ctrlOutputs);
        if(retVal!=0)
            return retVal;
    }

    // screen
    probeScreenCommands();
    initialScreenState = isScreenOn();
    pthread_create(&screenCtl_thread, NULL, screenCtrl_loop, NULL);

    // update context
    intContext.ctrlOutputs = ctrlOutputsContext.ctrlOutBuffer;
    intContext.ctrlOutChannels = chn_cout_count;
    intContext.ctrlOutputsSupported = ctrlOutputsContext.ctrlOutSupported;
    intContext.screenGetStateSupported = (screenCmds.idx != -1);

    return 0;
}


void LDSP_cleanupCtrlOutputs()
{
    // if control outputs were off, nothing to do here
    if(ctrlOutputsOff)
        return;

    if(ctrlOutputsVerbose)
        printf("LDSP_cleanupCtrlOutputs()\n");

    
    // wait for screen thread to finish...
    pthread_join(screenCtl_thread, NULL);

    // ...then reset screen to inital state
    if(isScreenOn() != initialScreenState)
        setScreen(initialScreenState); // brightness is not important as it reset in the next line 

    cleanupCtrlOutputs(ctrlOutputsContext.ctrlOutputs, chn_cout_count);
}


void screenSetState(bool stateOn, float brightness, bool stayOn)
{
    // copy values that will be set in thread
    nextBrightness = brightness;
    if(stateOn)
        nextStayOn = stayOn;
    else
        nextStayOn = false;
    nextScreenState = stateOn;
}

bool screenGetState()
{
    return isScreenOn();
}

//----------------------------------------------------------------------------------------------------------


bool ctrlOutputAutoFill_ctrl(int out, shared_ptr<ctrlOutputKeywords> keywords, string &control_file)
{     
    DIR* directory;
    string path;
    
    // first try to open one of the possible container folders
    for(string folder : keywords->folders[out])
    {
        path = string(CTRL_BASE_PATH) + "/" + folder;
        directory = opendir(path.c_str());
        if (directory != nullptr)
            break;
        path = "";
    }
    // cannot find any folder ):
    if(path == "")
    {
        closedir(directory);
        return false;
    }

    // if we found folder, 
    // search for a subfolder whose name contains one of the strings
    dirent* entry;
    string content = "";
    while( (entry = readdir(directory)) != nullptr ) 
    {   
        string d_name = string(entry->d_name);
        for(string str : keywords->subFolders_strings[out])
        {
            // skip if not an actual string to check
            if(str == "")
                continue;

            size_t found = d_name.find(str);
            // if found, we append subfolder to path
            if (found != string::npos) 
            {
                content = path + "/" + d_name;
                break;
            }
        }
        // we can stop at this first string that we found!
        if(content != "")
        {
            path = content;
            break;
        }
    }
    closedir(directory);

    // cannot find any string in names ):
    if(content == "")
        return false;

    // once we have folder and subfolder,
    // finally check if the control file is there
    directory = opendir(path.c_str());
    if (directory == nullptr)
    {
        closedir(directory); 
        fprintf(stderr, "Control output \"%s\", cannot open file \"%s\" during autofill!\n", LDSP_ctrlOutput[out].c_str(), path.c_str());
        return false;
    }

    //TODO complete case of vibration with activate+duration
    // also in the write function!!!
    content = "";
    while( (entry = readdir(directory)) != nullptr ) 
    {
        string d_name = string(entry->d_name);
        // in most cases, there is a single possible name for the control file
        if(d_name == keywords->files[out][0])
        {
            content = "/" + d_name;
            break;
        }
        // only exception is vibraiton, that has two possibilities
        else if(out==chn_cout_vibration && d_name == keywords->files[out][1])
        {
            content = "/" + d_name;
            break;
        }
    }
    // cannot find file ):
    if(content == "")
    {
        closedir(directory);        
        return false;
    }

    // this is the full path to the control file!
    control_file = path + content;
    
    closedir(directory);     

    return true;
}


void ctrlOutputAutoFill_scale(int out, shared_ptr<ctrlOutputKeywords> keywords, string control_file, string &scale_file)
{
    // if vibration is set manually, this will never be called for vibration, cos once vibration is found in hwConfig file scale is set to 1 by default
    // so here we are dealing with the case where vibration is all automatice... and we set default scale to 1
    if(out == chn_cout_vibration)
    {
        scale_file = "1";
        return;
    }
    

    scale_file = "255"; // default value, in case file cannot be found

    // search for max file in same dir where control file is
    // to obtain dir path, remove control file name from path!
    string path = std::regex_replace(control_file, std::regex("/"+keywords->files[out][0]), "");
    
    DIR* directory = opendir(path.c_str());
    if (directory == nullptr)
    {
        closedir(directory); 
        fprintf(stderr, "Control output \"%s\", cannot open file \"%s\" during autofill!\n", LDSP_ctrlOutput[out].c_str(), path.c_str());
        return;
    }

    // then check if the max file is here too!
    dirent* entry;
    while( (entry = readdir(directory)) != nullptr ) 
    {
        string d_name = string(entry->d_name);
        if(d_name == keywords->files[out][1])
        {
            scale_file = path + "/" + d_name;
            break;
        }
    }            
    
    closedir(directory); 
}


int initCtrlOutputs(string **ctrlOutputsFiles)
{
    shared_ptr<ctrlOutputKeywords> keywords = make_shared<ctrlOutputKeywords>(); // will be useful in auto config

    if(ctrlOutputsVerbose)
        printf("Control output list:\n");

    ifstream readFile;
    for (int out=0; out<chn_cout_count; out++)
    {
        int autoConfig_ctrl = false;
        int autoConfig_scale = false;
        ctrlout_struct &ctrlOutput = ctrlOutputsContext.ctrlOutputs[out];

        // control file first

        // if empty, try auto fill
        if(ctrlOutputsFiles[DEVICE_CTRL_FILE][out].compare("") == 0)
        {
            // and if still empty, ctrlOutput not configured
            if( !ctrlOutputAutoFill_ctrl(out, keywords, ctrlOutputsFiles[DEVICE_CTRL_FILE][out]) )
            {
                ctrlOutput.configured = false;
                
                if(ctrlOutputsVerbose)
                    printf("\t%s not configured\n", LDSP_ctrlOutput[out].c_str());

                //-------------------------------------------------------------------
                // context update
                ctrlOutputsContext.ctrlOutSupported[out] = false;
                continue;
            }
            autoConfig_ctrl = true;
        }
      
        string fileName = ctrlOutputsFiles[DEVICE_CTRL_FILE][out];

        // configured
        ctrlOutput.configured = true;
        
        // initial value
        readFile.open(fileName);
        if(!readFile.is_open())
        {
            fprintf(stderr, "Control output \"%s\", cannot open associated control file \"%s\"\n", LDSP_ctrlOutput[out].c_str(), fileName.c_str());
            
            LDSP_cleanupCtrlOutputs();
            return -1;
        }
        char val[6];
        readFile >> val;
        ctrlOutput.initialVal = atoi(val);  
        readFile.close();

        // prev value
        ctrlOutput.prevVal = 0; // the write buffer is filled with 0s and new values are only written if different from prev one
        // hence the initial value is preserved until we explicitly make a change
        
        // control file
        ctrlOutput.file.open(fileName);


        // max file/max value then

        // if empty, autofill!
        // worst case scenario, autofill will put default "255"
        // note that if vibration is manually set in hwConfig file, the parser always puts "1", so no autofill is invoked
        // the scale is a non-meaningful and dangerous param for user, that's why we sort of hide it, so that it is never set in the hwConfig file
        if(ctrlOutputsFiles[DEVICE_SCALE][out].compare("") == 0)
        {
            ctrlOutputAutoFill_scale(out, keywords, ctrlOutputsFiles[DEVICE_CTRL_FILE][out], ctrlOutputsFiles[DEVICE_SCALE][out]);
            autoConfig_scale = true;
        }
        
        // scale value
        int scaleVal;
        string content = ctrlOutputsFiles[DEVICE_SCALE][out]; 
        int isNumber = isdigit(content.c_str()[0]);
        // if this is a number, then use it as custom max value
        if(isNumber)
            scaleVal = stoi(content);
        else // otherwise this is the path to the file that contains the max value
        {
            readFile.open(content);
            if(!readFile.is_open())
            {
                fprintf(stderr, "Control output \"%s\", cannot open associated max file \"%s\"\n", LDSP_ctrlOutput[out].c_str(), ctrlOutputsFiles[DEVICE_SCALE][out].c_str());
                
                LDSP_cleanupCtrlOutputs();
                return -1;
            }
            readFile >> val;
            scaleVal = atoi(val);  
            readFile.close();
        }
        
        // assign scale value               
        ctrlOutput.scaleVal = scaleVal;

        if(ctrlOutputsVerbose)
        {
            printf("\t%s configured!\n", LDSP_ctrlOutput[out].c_str());
            
            string automatic = "";
            if(autoConfig_ctrl)
                automatic = " (set automatically)";
            printf("\t\tcontrol file \"%s\"%s\n", ctrlOutputsFiles[DEVICE_CTRL_FILE][out].c_str(), automatic.c_str());
            
            // we skip this info if vibration, as reading about the default scale value may be confusing  
            if(out!=chn_cout_vibration)
            {
                if(isNumber)
                {
                    automatic = "automatically";
                    if(!autoConfig_scale)
                        automatic = "manually";

                    printf("\t\t%s set max value: %d\n", automatic.c_str(), ctrlOutput.scaleVal);
                }
                else
                {
                    automatic = "";
                    if(autoConfig_scale)
                        automatic = " (set automatically)";
                    printf("\t\tmax value file \"%s\"%s, containing value: %d\n", ctrlOutputsFiles[DEVICE_SCALE][out].c_str(), automatic.c_str(), ctrlOutput.scaleVal);
                }
                printf("\t\tinitial value: %d\n", ctrlOutput.initialVal);
            }
        }

        //-------------------------------------------------------------------
        // context update
        ctrlOutputsContext.ctrlOutSupported[out] = true;
        ostringstream stream;
        stream << LDSP_ctrlOutput[out] << ", with max value: " << ctrlOutput.scaleVal;
    }

    return 0;
}


void writeCtrlOutputs()
{
    //VIC output values are persistent! 
    // we don't reset the output buffers once written to the devices
    // but we keep track of last value set and do not write again if same as requested value
    // NOTE: the only exception is vibration control, that is timed, so its value is reset to 0 once written

    // write to analog devices
    for(int out=0; out<chn_cout_count; out++)
    {
        ctrlout_struct *ctrlOutput = &ctrlOutputsContext.ctrlOutputs[out];
        
        // nothing to do on non-configured devices
        if(!ctrlOutput->configured)
            continue;

        unsigned int outInt = ctrlOutputsContext.ctrlOutBuffer[out]*ctrlOutput->scaleVal;
        // if scaled (de-normalized) value is same as previous one, no need to update
        if(outInt == ctrlOutput->prevVal)
            continue;

        // write
        write(&ctrlOutput->file, outInt);

        // vibration is timed based and the value that we write is the duration of vibration
        // once vibration ends, the written file goes automatically to zero 
        // hence, we reset the value in our buffer once written, to align with the file's behavior
        // this allows us to set vibration repeatedly regardless of previous value
        if(out == chn_cout_vibration)
        {
            ctrlOutputsContext.ctrlOutBuffer[out] = 0; // reset buffer value
            outInt = 0; // set prev value to 0, so that same value can be set in subsequent calls
        }

        // update prev val for next call
        ctrlOutput->prevVal = outInt;
    }
}


void cleanupCtrlOutputs(ctrlout_struct *ctrlOutputs, int numOfOutputs)
{
    for (int out=0; out<numOfOutputs; out++)
    {
        // reset ctrlOutput to its initial state
        ctrlout_struct *ctrlOutput = &ctrlOutputs[out];
        // nothing to do on non-configured devices
        if(!ctrlOutput->configured)
            continue;
        write(&ctrlOutput->file, ctrlOutput->initialVal);

        // close file
        if(!ctrlOutput->configured)
            continue;
        if(!ctrlOutput->file.is_open())
            continue;
        ctrlOutput->file.close();
    }
}


void probeScreenCommands()
{
    for(int i=0; i<screenCmds.idx_cnt; i++)
    {
        // try all dumpsys commands stored in service
        FILE* pipe = popen(screenCmds.service[i].c_str(), "r");
        if(!pipe) 
        {
            fprintf(stderr,"Error: unable to run \"%s\"\n", screenCmds.service[i].c_str());
            return;
        }
        char buffer[128];

        // cycle all lines
        while(!feof(pipe)) 
        {
            if(fgets(buffer, 128, pipe) != NULL) 
            {
                // if line containes one of tehe properties saved in prop
                if(strstr(buffer, screenCmds.prop[i].c_str()) != NULL) 
                {
                    // and result is string stored in either on or off
                    if(strstr(buffer, screenCmds.on[i].c_str()) != NULL ||
                       strstr(buffer, screenCmds.off[i].c_str()) != NULL ) 
                    {
                        screenCmds.idx = i; // then this is where we can get and set the state of the screen
                        break;
                    }
                }
            }
        }
        pclose(pipe);
        if(screenCmds.idx != -1)
            break;
    }
}

bool isScreenOn()
{
    int idx = screenCmds.idx;
    if(idx == -1)
        return true;

    bool screenIsOn = false;
    // check the dumpsys command
    FILE* pipe = popen(screenCmds.service[idx].c_str(), "r");
    if (!pipe) 
    {
        fprintf(stderr,"Error: unable to run \"%s\"\n", screenCmds.service[idx].c_str());
        return false;
    }
    char buffer[128];
    
    // cycle all lines
    while (!feof(pipe)) 
    {
        if (fgets(buffer, 128, pipe) != NULL) 
        {
            // check the line that includes the property
            if (strstr(buffer, screenCmds.prop[idx].c_str()) != NULL) 
            {
                // check the value of the property
                if (strstr(buffer, screenCmds.on[idx].c_str()) != NULL)
                    screenIsOn = true;
                else if (strstr(buffer, screenCmds.off[idx].c_str()) != NULL) 
                    screenIsOn = false;
            }
        }
    }
    pclose(pipe);

    return screenIsOn;
}


void setScreen(float brightness)
{
    ctrlOutputWrite((LDSPcontext *)&intContext, chn_cout_lcdBacklight, brightness);
    system(pwrBtn_input_cmd);
}

void* screenCtrl_loop(void* arg)
{
    static const useconds_t sleepTime_us = 100000;
    static const unsigned int tapInterval = 20; // as a multiple of sleep time [sleep cycles]!
    // low prio
    set_priority(ScreenCtrlLoopPrioOrder, false);
    bool tapActive = false;
    unsigned int tapCounter = 0;

    while(!gShouldStop) 
    {
        // -1 means values set already, nothing to do
        if(nextScreenState != -1)
        {
            // if change is requested, first check current state of screen
            bool screenIsOn = isScreenOn();

            // if requested state is different than current state 
            if(screenIsOn!=nextScreenState)
            {
                if(!nextScreenState)
                {
                    setScreen(0); // turn off
                    // and disable tap, just in case
                    tapActive = false;
                    nextStayOn = false;
                }
                else
                    setScreen(nextBrightness); // turn on
            }
            // if keep-screen-on setting has been changed
            if(tapActive!=nextStayOn)
            {
                if(tapActive)
                {
                    tapActive = false;
                    nextStayOn = false;
                }
                else
                {
                    tapActive = true;
                    nextStayOn = true;
                }
            }

            // then we flag that the change was made
            nextScreenState = -1; 
        }
        else if(tapActive) // if auto tapping mechanism
        {
            // count sleep cycles
            if(tapCounter >= tapInterval)
            {
                // time to tap to keep screen on
                system("input tap 1 1"); // top left area of screen... this tap is not detected as multitouch event!
                tapCounter = 0;
            }
            else
                tapCounter++; 
        }

        //Zzz... for quite a while
        usleep(sleepTime_us);
    }

    return (void *)0;
}