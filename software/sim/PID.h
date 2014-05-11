#ifndef _PID_H
#define _PID_H

#include <sys/time.h>

// returns microseconds since unix era
unsigned long long gettimeofday ();

class PID {
    static const int PID_NUM_OLD_VALUES = 5;
    static const int PID_NUM_DIFF_VALUES = 2; // num of old values to use during differentiation

	protected:
		float P, I, D;                          //PID values 
		float Kp, Ki, Kd;						//PID constants

		unsigned long long times[PID_NUM_OLD_VALUES];

        int valid_oldValues;
		float oldValues[PID_NUM_OLD_VALUES];                	//store old values
		float alpha;											//decay constant for I

	public:
		PID();
		PID(float _Kp, float _Ki, float _Kd, float _alpha=0);
		~PID();

		void setK(float _Kp, float _Ki, float _Kd);
		void setAlpha(float _alpha);

		void PID_Reset();

		void PID_Update(float value);    	
		float PID_Output();

		void debug();
};
#endif
