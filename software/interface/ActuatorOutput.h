/* ActuatorOutput interface.

   This interface represents the limbs of the submarine. It provides an interface
   to actuate the submarine.
*/

#ifndef ACTUATOR_OUTPUT_H
#define ACTUATOR_OUTPUT_H

enum ATTITUDE_CHANGE_DIRECTION {
  FORWARD,
  REVERSE,
  LEFT,
  RIGHT,
  SINK,
  RISE
};

enum ATTITUDE_DIRECTION {
  SPEED,
  YAW,
  DEPTH
};

enum SPECIAL_COMMAND {
  SUB_POWER_ON,
  SUB_STARTUP_SEQUENCE,
  SUB_MISSION_STARTUP_SEQUENCE,
  SUB_POWER_OFF,
  SIM_MOVE_FWD,
  SIM_MOVE_REV,
  SIM_MOVE_LEFT,
  SIM_MOVE_RIGHT,
  SIM_MOVE_RISE,
  SIM_MOVE_SINK,
  SIM_ACCEL_ZERO
};

#define DEFAULT_ATTITUDE_CHANGE 1

/* Actuator Output interface */
class ActuatorOutput {
  public:
    virtual ~ActuatorOutput() {}

    // some methods to actuate the output
    virtual void set_attitude_absolute(ATTITUDE_DIRECTION, int) = 0;
    virtual int get_target_attitude(ATTITUDE_DIRECTION) = 0;
    virtual void stop() = 0;
    virtual void special_cmd(SPECIAL_COMMAND) = 0;
    virtual void set_attitude_change(ATTITUDE_CHANGE_DIRECTION dir, int delta)
    {
      switch(dir) {
        case REVERSE:
        case LEFT:
        case RISE:
          delta *= -1;
          break;
        default:
          break;
      }

      switch(dir) {
        case REVERSE:
        case FORWARD:
          set_attitude_absolute(SPEED, delta); // doesn't make sense to change the speed, set absolute
          break;
        case RIGHT:
        case LEFT:
          set_attitude_absolute(SPEED, 0);
          set_attitude_absolute(YAW, get_target_attitude(YAW) + delta);
          break;
        case SINK:
        case RISE:
          set_attitude_absolute(SPEED, 0);
          set_attitude_absolute(DEPTH, get_target_attitude(DEPTH) + delta);
          break;
      }
    }

    void set_attitude_change(ATTITUDE_CHANGE_DIRECTION dir) { set_attitude_change(dir, DEFAULT_ATTITUDE_CHANGE); }
};

/* A don't care implementation */
class ActuatorOutputNull : public ActuatorOutput {
  public:
    virtual ~ActuatorOutputNull() {}

    virtual void set_attitude_absolute(ATTITUDE_DIRECTION dir, int val) {}
    virtual int get_target_attitude(ATTITUDE_DIRECTION) { return 0; }
    virtual void stop() {}
    virtual void special_cmd(SPECIAL_COMMAND cmd) {}
};

/* Simulator implementation */
class ActuatorOutputSimulator : public ActuatorOutput {
  public:
    ActuatorOutputSimulator();
    virtual ~ActuatorOutputSimulator();

    virtual void set_attitude_absolute(ATTITUDE_DIRECTION, int);
    virtual int get_target_attitude(ATTITUDE_DIRECTION);
    virtual void stop();
    virtual void special_cmd(SPECIAL_COMMAND);
};

/* The real submarine implementation */
class ActuatorOutputSubmarine : public ActuatorOutput {
  public:
    ActuatorOutputSubmarine();
    virtual ~ActuatorOutputSubmarine();

    virtual void set_attitude_absolute(ATTITUDE_DIRECTION, int);
    virtual int get_target_attitude(ATTITUDE_DIRECTION);
    virtual void stop();
    virtual void special_cmd(SPECIAL_COMMAND);

    // Specific commands
    void set_depth_constants(double, double, double, double);
    void set_pitch_constants(double, double, double, double);
    void set_roll_constants(double, double, double, double);
    void set_yaw_constants(double, double, double, double);

  protected:
    void mission_startup_sequence();
};

#endif
