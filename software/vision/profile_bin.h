/*

This is a class that creates a time profile. 
If PROFILE_ON is true, this class will collect:
	- the name,
	- if this specific thing needs to be saved for now,
	- the current time of start.
	- how many times something is called (cont.),
	- how long something ran for in total (cont.).
This class has two functions - START and STOP.
START add the numCalls by 1 and collects the current time and stores it in startTime.
STOP collects the current time and subtracts that from the time stored in startTime divided by CLOCKS_PER_SEC and stores it in totalTime.
When a partciluar "on" PROFILE_BIN gets destroyed, then the destructor displays:
	- the name,
	- the number of calls,
	- The total seconds it was on.
	- And the average time it was on.

*/
#ifndef __PROFILE_BIN__
#define __PROFILE_BIN__
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "../common.h"

class PROFILE_BIN {

#ifdef PROFILE_ON    
    char binName[50];
    unsigned startTime, nCalls;
    float totalTime;
    bool on;
    
    public:
    
    PROFILE_BIN (const char* Name, bool On=true) {
	assert(strlen(Name) < 50);
      
        on = On;
        startTime = 0;
        totalTime = 0;
        nCalls = 0;
	strcpy(binName, Name);
    }

    ~PROFILE_BIN () {
        if (on && nCalls > 0) {
            printf ("%-30s: ", binName);
            printf ("%6d calls; ", nCalls);
            printf ("%7.3f secs total time; ", totalTime);
            printf ("%7.4f msecs average time;\n", 1000*totalTime/nCalls);
        }
    }
    
    void start() {
        startTime = clock();
        nCalls++;
    }
    
    void stop() {
        totalTime += float(clock() - startTime)/CLOCKS_PER_SEC;
    }
#else
    public:
    PROFILE_BIN (const char* Name, bool On=true) {}
    void start() {}
    void stop() {}
#endif
};

#endif
