#include "PID.h"
#include <stdio.h>
#include <algorithm> // for std::min

// wrapper to make <sys/time> gettimeofday easier to use
unsigned long long gettimeofday () {
    struct timeval T;
    gettimeofday (&T, NULL);
    return (T.tv_sec*1000000 + T.tv_usec);
}

PID::PID(){
	Kp =  Ki =  Kd =  alpha = 0;
	P = I = D = 0;
    valid_oldValues = 0;
    unsigned long long t_us = gettimeofday();
	
	for(int i=0; i<PID_NUM_OLD_VALUES; i++)
		times[i] = (t_us-i);	//initialize to now, with an offset such that 1/(t[i]-t[i+1]) != inf
}

PID::PID(float _Kp, float _Ki, float _Kd, float _alpha){
	Kp = _Kp; Ki = _Ki; Kd = _Kd; alpha = _alpha;
	P = I = D = 0;
    valid_oldValues = 0;
	unsigned long long t_us = gettimeofday();
	
	for(int i=0; i<PID_NUM_OLD_VALUES; i++)
		times[i] = (t_us-i);	//initialize to now, with an offset such that 1/(t[i]-t[i+1]) != inf
}


PID::~PID(){}

void PID::setK(float _Kp, float _Ki, float _Kd){
	Kp = _Kp; Ki = _Ki; Kd = _Kd;
}

void PID::setAlpha(float _alpha){
	alpha = _alpha;
}

void PID::PID_Reset(){
/*Resets controller while preserving PID constants*/
	P = I = D = 0;
    valid_oldValues = 0;

	unsigned long long t_us = gettimeofday();

	for(int i=0; i<PID_NUM_OLD_VALUES; i++){
		times[i] = (t_us-i);	//initialize to now, with an offset such that 1/(t[i]-t[i+1]) != inf
	}
}

void PID::PID_Update(float value){	
/*Updates PID signals internally for each controlled value
  Expects that values is of size PID_DEG_FREEDOM*/

	for(int j=PID_NUM_OLD_VALUES-1; j>0; j--){ // shift every time value back one slot
		times[j] = times[j-1];
        oldValues[j] = oldValues[j-1];
	}

	times[0] = gettimeofday();
    oldValues[0] = value;

    if (valid_oldValues < PID_NUM_OLD_VALUES)
        valid_oldValues++;
}

float PID::PID_Output(){
/*Output control signals for each controlled vlaue
  Output is of length PID_DEG_FREEDOM*/
    float P=0, I=0, D=0;
   
    P = oldValues[0];

    // use trapezoid rule.
    for (int i = 0; i < valid_oldValues-1; i++) {
        I = I*(1-alpha) + (times[i] - times[i+1]) * (oldValues[i] + oldValues[i+1]);
    }
    I /= 2.0; // divide by 2 due to trapezoid rule 
    I /= 1000000.0;

    for (int i = 0; i < std::min(PID_NUM_DIFF_VALUES,valid_oldValues-1); i++) {
        unsigned long long delta_t = times[i] - times[i+1];
        delta_t = (delta_t > 0) ? delta_t : 1;
        D += (oldValues[i] - oldValues[i+1]) / delta_t;
    }
    D /= PID_NUM_DIFF_VALUES;
    D *= 1000000.0;
    
	return (Kp*P + Ki*I + Kd*D);
}

void PID::debug(){
	printf(" num_valid: %d\n", valid_oldValues);
    printf(" times: ");
	for(int j=0; j<PID_NUM_OLD_VALUES; j++){
		printf("%lld ",times[j]);
	}
	printf("\n");
}
