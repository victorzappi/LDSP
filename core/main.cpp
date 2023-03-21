// LineageDSP
#include <iostream>
#include <signal.h> //SIGINT, SIGTERM

#include "LDSP.h"
#include "commandLineArgs.h"
#include "mixer.h"
#include "ctrlOutputs.h"


using std::string;
using std::cout;


// Handle Ctrl-C by requesting that the audio rendering stop
void interrupt_handler(int sig)
{
	printf("--->Signal caught!<---\n");
	LDSP_requestStop();
}


//TODO move mixer setup/reset into audio calls
// and combine sensors ctrl inputs and outputs calls into a single container file
// this will simplify a lot the default main file

int main(int argc, char** argv)
{
 	cout << "Hello, LDSP here!\n" << "\n";

	LDSPinitSettings* settings = LDSP_InitSettings_alloc();	// Standard audio settings
	LDSP_defaultSettings(settings);

	if(LDSP_parseArguments(argc, argv, settings) < 0)
	{
		LDSP_InitSettings_free(settings);
		fprintf(stderr,"Error: unable to parse command line arguments\n");
		return 1;
	}

	LDSPhwConfig* hwconfig = LDSP_HwConfig_alloc();
	if(LDSP_parseHwConfigFile(settings, hwconfig)<0)
	{
		LDSP_HwConfig_free(hwconfig);
		LDSP_InitSettings_free(settings);
		fprintf(stderr,"Error: unable to parse hardwar configuration file\n");
	}

	if(LDSP_setMixerPaths(settings, hwconfig) < 0)
	{
		LDSP_HwConfig_free(hwconfig);
		LDSP_InitSettings_free(settings);
		fprintf(stderr,"Error: unable to set mixer paths\n");
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
		fprintf(stderr,"Error: unable to intialize control outputs\n");
		return 1;
	}

	if(LDSP_initAudio(settings, 0) != 0) 
	{
		LDSP_cleanupCtrlOutputs();
		LDSP_cleanupCtrlInputs();
		LDSP_cleanupSensors();
		LDSP_resetMixerPaths(hwconfig);
		LDSP_HwConfig_free(hwconfig);
		fprintf(stderr,"Error: unable to initialize audio\n");
		return 1;
	}

	LDSP_InitSettings_free(settings);

	// Set up interrupt handler to catch Control-C and SIGTERM
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);

	void *userData = 0; // this can be used to pass user data to setup function in reander.cpp

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
	 	return 1;
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
