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
 * priority_utils.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */

#include "thread_utils.h"

// thread, priority and niceness
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h> // getpid
#include <string>
#include <fstream> // Include for std::ofstream
#include <stdio.h>

using std::string;
using std::ifstream;
using std::ofstream;



void set_cpu_affinity(int cpuIndex, std::string name, bool verbose)
{
	cpu_set_t cpuset;
    CPU_ZERO(&cpuset); // initialize the CPU set
    CPU_SET(cpuIndex, &cpuset); // add CPU to the set... 
	// on Android phones CPU 0 is the most performative one

    // Get the thread's PID
    pid_t pid = gettid();

	if(verbose)
		printf("*\n");

	if(verbose)
		printf("Trying to set CPU %d affinity for %s thread (pid %d) \n", cpuIndex, name.c_str(), pid);

    // Set the affinity to ensure the thread runs on CPU 0
    if(sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset) != 0) 
	{
		// Print the error
		printf("Unsuccessful in setting CPU %d affinity for %s thread (pid %d)\n", cpuIndex, name.c_str(), pid);
		if(verbose)
			printf("*\n");
		return;
    }

    // Print thread scheduling priority
    if(verbose)
    	printf("Set CPU %d affinity for %s thread (pid %d)\n", cpuIndex, name.c_str(), pid);

    if(verbose)
    	printf("*\n");
}

//-----------------------------------------------------------------------------------------------------------

// adapted from here:
// http://www.yonch.com/tech/82-linux-thread-priority

// set priority of this thread
void set_priority(int order, std::string name, bool verbose) 
{
	if(verbose)
		printf("*\n");

    // We'll operate on the currently running thread.
    pthread_t this_thread = pthread_self();
    // struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO) - order;

	if(verbose)
	{
		std::string rt = (order == 0) ? " (real-time)" : "";
		printf("Trying to set %s thread prio to %d%s\n", name.c_str(), params.sched_priority, rt.c_str());
	}

	// Attempt to set thread real-time priority to the SCHED_FIFO policy
	if (pthread_setschedparam(this_thread, SCHED_FIFO, &params) != 0) {
		// Print the error
		printf("Unsuccessful in setting %s thread prio\n", name.c_str());
		if(verbose)
			printf("*\n");
		return;
	}

	// Now verify the change in thread priority
    int policy = 0;
    if (pthread_getschedparam(this_thread, &policy, &params) != 0) {
        printf("Couldn't retrieve real-time scheduling parameters\n");
        if(verbose)
        	printf("*\n");
        return;
    }

    // Check the correct policy was applied
    if(policy != SCHED_FIFO) {
        printf("Scheduling is NOT SCHED_FIFO!\n");
    } else {
    	if(verbose)
    		printf("SCHED_FIFO OK\n");
    }

    // Print thread scheduling priority
    if(verbose)
    	printf("Thread priority is %d\n", params.sched_priority);

    if(verbose)
    	printf("*\n");
}

// set niceness of this thread
void set_niceness(int niceness, std::string name, bool verbose) 
{
	if(verbose)
		printf("*\n");

	if(verbose)
		printf("Trying to set %s thread niceness %d\n", name.c_str(), niceness);

	 int which = PRIO_PROCESS;
	 id_t pid = getpid();

	 if(setpriority(which, pid, -20)!=0) 
	 {
		 printf("Unsuccessful in setting %s thread niceness\n", name.c_str());
		 if(verbose)
			 printf("*\n");
		 return;
	 }

	if(verbose)
		printf("Thread niceness is %d\n", getpriority(which, pid));

	if(verbose)
		printf("*\n");
}

//-----------------------------------------------------------------------------------------------------------

void set_to_foreground(std::string name, bool verbose)
{
	if(verbose)
		printf("*\n");

	auto pid = getpid();
    std::string pidStr = std::to_string(pid);

    // Paths to the CPU set and scheduling group files
    std::string cpuSetPath = "/dev/cpuset/foreground/tasks";
    std::string schedGroupPath = "/dev/stune/foreground/tasks";

    // Write the PID to the CPU set file
    std::ofstream cpuSetFile(cpuSetPath, std::ios::out);
    if (cpuSetFile.is_open()) 
	{
        cpuSetFile << pidStr;
        cpuSetFile.close();

		if(verbose)
			printf("%s thread (pid %d) set to foreground in cpuset scheduling\n", name.c_str(), pid);
    } 
	else
    	printf("Could not set %s thread (pid %d) to foreground in cpuset scheduling\n", name.c_str(), pid);

    // Write the PID to the scheduling group file
    std::ofstream schedGroupFile(schedGroupPath, std::ios::out);
    if (schedGroupFile.is_open()) 
	{
        schedGroupFile << pidStr;
        schedGroupFile.close();

		if(verbose)
			printf("%s thread (pid %d) set to foreground in group scheduling\n", name.c_str(), pid);
    } 
	else 
        printf("Could not set %s thread (pid %d) to foreground in group scheduling\n", name.c_str(), pid);

	if(verbose)
		printf("*\n");
}