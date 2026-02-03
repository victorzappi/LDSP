// LineageDSP
#include <iostream>
#include <signal.h> //SIGINT, SIGTERM
#include <atomic>

#include "LDSP.h"
#include "commandLineArgs.h"
#include "mixer.h"
#include "ctrlOutputs.h"

// defined in root's CMakeLists.txt
#ifdef PROJECT_NAME
	#define PRJ_NAME PROJECT_NAME
#else
	#define PRJ_NAME ""
#endif


using std::string;
using std::cout;


std::atomic<bool> shutdown_requested(false);
// Handle Ctrl-C by requesting that the audio rendering stop
void interrupt_handler(int sig)
{
	bool expected = false;
    if(!shutdown_requested.compare_exchange_strong(expected, true))
        return;  // Already handling shutdown, ignore
    
	printf("--->Signal caught!<---\n");
	LDSP_requestStop();
}


int main(int argc, char** argv)
{
 	cout << "Hello, LDSP here!\n" << "\n";

	LDSPinitSettings* settings = LDSP_InitSettings_alloc();	// Standard audio settings
	LDSP_defaultSettings(settings);

	settings->projectName = PRJ_NAME;
	cout << "Project: " << settings->projectName << "\n" << "\n";

	if(LDSP_parseArguments(&argc, argv, settings) < 0)
	{
		// in case help is printed
		LDSP_InitSettings_free(settings);
		cout << "\nBye!" << "\n";
		return 0;
	}

	LDSPhwConfig* hwconfig = LDSP_HwConfig_alloc();
	if(LDSP_parseHwConfigFile(settings, hwconfig)<0)
	{
		LDSP_HwConfig_free(hwconfig);
		LDSP_InitSettings_free(settings);
		fprintf(stderr, "Error: unable to parse hardwar configuration file\n");
	}

	if(LDSP_setMixerPaths(settings, hwconfig) < 0)
	{
		LDSP_HwConfig_free(hwconfig);
		LDSP_InitSettings_free(settings);
		fprintf(stderr, "Error: unable to set mixer paths\n");
		return 1;
	}

	LDSP_initSensors(settings);

	LDSP_initCtrlInputs(settings);

	if(LDSP_initCtrlOutputs(settings, hwconfig) < 0)
	{
		LDSP_resetMixerPaths(hwconfig);
		LDSP_HwConfig_free(hwconfig);
		LDSP_cleanupSensors();
		LDSP_cleanupCtrlInputs();
		LDSP_InitSettings_free(settings);
		fprintf(stderr, "Error: unable to intialize control outputs\n");
		return 1;
	}

	if(LDSP_initAudio(settings, 0) != 0) 
	{
		LDSP_cleanupCtrlOutputs();
		LDSP_cleanupCtrlInputs();
		LDSP_cleanupSensors();
		LDSP_resetMixerPaths(hwconfig);
		LDSP_HwConfig_free(hwconfig);
		fprintf(stderr, "Error: unable to initialize audio\n");
		return 1;
	}

	LDSP_InitSettings_free(settings);

	// Set up interrupt handler to catch Control-C and SIGTERM
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);

	// this can be used to pass user data to all functions in reader.cpp
	void *userData = argv; // by deafult, we pass unrecognized cmd line arguments

	// Start the audio device running
	if(LDSP_startAudio(userData)) 
	{
		// Clean up any resources allocated
	 	LDSP_cleanupAudio();
		LDSP_cleanupCtrlOutputs();
		LDSP_cleanupCtrlInputs();
		LDSP_cleanupSensors();
		LDSP_resetMixerPaths(hwconfig);
		LDSP_HwConfig_free(hwconfig);
		cout << "\nBye /:" << "\n";
	 	return -1;
	}


	LDSP_cleanupAudio();
	LDSP_cleanupCtrlOutputs();
	LDSP_cleanupCtrlInputs();
	LDSP_cleanupSensors();
	LDSP_resetMixerPaths(hwconfig);
	LDSP_HwConfig_free(hwconfig);
	
	cout << "\nBye!" << "\n";

	return 0;
}
