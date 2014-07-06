#include <math.h>
#include <string.h>
#include <stdio.h>

#define SAMPLINGFREQUENCY 1000000
#define SPEEDOFSOUNDINWATER 1500
#define pi 3.141592
#define MAX_OUTPUT_LEN 27
#define ORG_STR_LEN 11
#define MAX_DISTANCE_LEN 11
#define FILE_EXT_LEN 4
#define CHAR_LEN 1
#define LEN_TRY 3

/*

Take SinHertz, distanceFromPinger(m), pulseTime(s), arrayLength, timeBetweenPulses(s) and an array c.

Let timeDelay = distanceFromPinger / SPEEDOFSOUNDINWATER

Until i = arrayLength:

	Put zero into c until i = timeDelay*SAMPLINGFREQUENCY.

	Then put in pulseTime*SAMPLINGFREQUENCY data entries that vary depending on SinHertz, and distanceFromPinger 
	c[i]= sin(2piSinHertz(i-(timeDelay+n*timeBetweenPulses*SAMPLEINGFREQUENCY))) / (distanceFromPinger*distanceFromPinger)
	where n=number of pulses found

	(We are using Inverse Squared Law as found on http://en.wikipedia.org/wiki/Underwater_acoustics#Propagation_of_sound 
	and http://en.wikipedia.org/wiki/Near_and_far_field.)

	When we get out of pulseTime, add n by 1.


*/

int main (void)
{
	double SinHertz = 22000, distanceFromPinger = 10.1, pulseTime = 0.0013, timeBetweenPulses = 2;
	int arrayLength = 7383;
        
        double c[arrayLength];
	
	int i = 0;
        int n = 0;
        
        char title[MAX_OUTPUT_LEN];
        char distance[MAX_DISTANCE_LEN];
        char try[CHAR_LEN] = "2";
        
        strncpy(title, "MatLabInput", ORG_STR_LEN);
        snprintf(distance, MAX_DISTANCE_LEN,  "%g", distanceFromPinger);
        strncat(title, distance, MAX_DISTANCE_LEN);
        strncat(title, "Try", LEN_TRY);
        strncat(title, try, CHAR_LEN);
        strncat(title, ".csv", FILE_EXT_LEN);
        title[MAX_OUTPUT_LEN] = '/0';
        
        FILE* OutputToMatLab = fopen(title, "w");
	
	double timeDelay = distanceFromPinger / SPEEDOFSOUNDINWATER * SAMPLINGFREQUENCY;
	
	for ( ; i < arrayLength; i++)
	{
		double masterShift = (timeDelay + n*timeBetweenPulses*SAMPLINGFREQUENCY);
		if (i < (int)masterShift)
		{
			c[i] = 0.0;
		}
		else if((i - (int)masterShift) <= (int)(pulseTime*SAMPLINGFREQUENCY))
		{
                        c[i] = ( sin(2*pi*SinHertz*(i - (int)masterShift)) / (distanceFromPinger * distanceFromPinger) );
		}
                else
                {
                    n++;
                    i--;
                }
		
		//printf("%d) %g ", i, c[i]);
                fprintf(OutputToMatLab, "%d, %g\n", i, c[i]);
	}
	
	return 0;
}
