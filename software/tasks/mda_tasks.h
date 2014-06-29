#ifndef __MDA_TASK__MDA_TASK__
#define __MDA_TASK__MDA_TASK__

#include <time.h>
#include <sys/time.h>

#include "mda_vision.h"
#include "AttitudeInput.h"
#include "ImageInput.h"
#include "ActuatorOutput.h"
#include "CharacterStreamSingleton.h"

enum MDA_TASK {
	TASK_TEST,
	TASK_GATE,
	TASK_MARKER,
	TASK_PATH,
	TASK_BUOY,
	TASK_GOALPOST
};

enum MDA_TASK_RETURN_CODE {
	TASK_ERROR, 
	TASK_DONE,
	TASK_MISSING,
	TASK_REDO,
	TASK_QUIT
};

// simple timer class 
class TIMER {
    unsigned long long starting_time;
    unsigned long long gettimeofday2 () {
	    struct timeval T;
	    gettimeofday (&T, NULL);
	    return (T.tv_sec*1000000 + T.tv_usec);
	}
public:
    TIMER() { restart(); }
    void restart() { starting_time = gettimeofday2(); }
    int get_time() { return static_cast<int>((gettimeofday2()-starting_time)/1000000); }
};


class MDA_TASK_BASE {
protected:
	AttitudeInput* attitude_input;
	ImageInput* image_input;
	ActuatorOutput* actuator_output;

	// Move or set attitude until stable
	void move (ATTITUDE_CHANGE_DIRECTION direction, int delta_accel);
	void set_yaw_change (int delta) {
		//int curr_yaw = actuator_output->get_target_attitude(YAW);
    	int curr_yaw = attitude_input->yaw();
    	actuator_output->set_attitude_absolute(YAW, curr_yaw+delta);
	}
	void set (ATTITUDE_DIRECTION dir, int val);
	void stop();
	void stabilize (ATTITUDE_DIRECTION dir);

	static const int HARDCODED_DEPTH = 400;

// Wait for user to press a key
#ifdef DEBUG_FRAME_BY_FRAME
	static const int TASK_WK = 0;
#else
	static const int TASK_WK = 5;
#endif
	
public:
	MDA_TASK_BASE (AttitudeInput* a, ImageInput* i, ActuatorOutput * o) :
		attitude_input (a),
		image_input (i),
		actuator_output (o)
	{
	}
	virtual ~MDA_TASK_BASE() {}

	// the following function loops until it completes the task and returns
	virtual MDA_TASK_RETURN_CODE run_task () = 0; 
	static int starting_depth;
};

/// ########################################################################
/// this class is to test stuff. Write anything
/// ########################################################################
class MDA_TASK_TEST : public MDA_TASK_BASE {
	mvWindow window;

public:
	MDA_TASK_TEST (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_TEST ();

	MDA_TASK_RETURN_CODE run_task ();
};

/// ########################################################################
/// Gate
/// ########################################################################
class MDA_TASK_GATE : public MDA_TASK_BASE {

public:
	MDA_TASK_GATE (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_GATE ();

	MDA_TASK_RETURN_CODE run_task ();
};

/// ########################################################################
/// Marker
/// ########################################################################
class MDA_TASK_MARKER : public MDA_TASK_BASE {

public:
	MDA_TASK_MARKER (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_MARKER ();

	MDA_TASK_RETURN_CODE run_task ();
};


/// ########################################################################
/// Path
/// ########################################################################
class MDA_TASK_PATH : public MDA_TASK_BASE {
	int pix_x_old;
	int pix_y_old;

public:
	MDA_TASK_PATH (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_PATH ();

	MDA_TASK_RETURN_CODE run_task ();
};

/// ########################################################################
/// Skip until path is out of view
/// ########################################################################
class MDA_TASK_PATH_SKIP : public MDA_TASK_BASE {

public:
	MDA_TASK_PATH_SKIP (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_PATH_SKIP ();

	MDA_TASK_RETURN_CODE run_task ();
};

/// ########################################################################
/// Buoy
/// ########################################################################
class MDA_TASK_BUOY : public MDA_TASK_BASE {
	static const int BUOY_RANGE_WHEN_DONE = 70; // range to the buoy to consider the tracking finished, in cm

	enum BUOY_COLOR {
		BUOY_RED,
		BUOY_YELLOW,
		BUOY_GREEN
	};

public:
	MDA_TASK_BUOY (AttitudeInput* a, ImageInput* i, ActuatorOutput* o); // defaults to red buoy
	~MDA_TASK_BUOY ();

	MDA_TASK_RETURN_CODE run_task ();
	MDA_TASK_RETURN_CODE run_single_buoy (int buoy_index, BUOY_COLOR color);

};

/// ########################################################################
/// Frame task
/// ########################################################################
class MDA_TASK_GOALPOST : public MDA_TASK_BASE {

public:
	MDA_TASK_GOALPOST (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_GOALPOST ();

	MDA_TASK_RETURN_CODE run_task ();
};

/// ########################################################################
/// Other tasks
/// ########################################################################
class MDA_TASK_SURFACE : public MDA_TASK_BASE {

public:
	MDA_TASK_SURFACE (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_SURFACE ();

	MDA_TASK_RETURN_CODE run_task();
};


class MDA_TASK_RESET : public MDA_TASK_BASE {

public:
	MDA_TASK_RESET (AttitudeInput* a, ImageInput* i, ActuatorOutput* o);
	~MDA_TASK_RESET ();

	MDA_TASK_RETURN_CODE run_task(int depth_in_mv, int yaw, int fwd_movement_time);
	MDA_TASK_RETURN_CODE run_task() {
		return run_task(MDA_TASK_BASE::starting_depth, 0, 0);
	}	
};

#endif
