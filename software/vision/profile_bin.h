#ifndef __PROFILE_BIN__
#define __PROFILE_BIN__
#include <time.h>
#include <string.h>
#include <stdio.h>

#define PROFILE_ON

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
