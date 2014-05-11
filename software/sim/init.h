/**
* @file init.h
*
* @brief Provides the starting location in the sim, centred on a chosen task
*
* Both underwater and above water sim task locations are placed here
*/

// only set one of these
#define TASK_GATE        1
#define TASK_LIGHT       0
#define TASK_FROM_LIGHT_TO_PIPE 0
#define TASK_LIGHT_PIPE  0    // This starting position does not work (possibly due to orange ground)
#define TASK_PIPE        0
#define TASK_PIPE_FOLLOW 0
#define TASK_PIPE_OFF    0
#define TASK_BOMB_RUN    0
#define TASK_MACHINE_GUN 0
#define TASK_X           0
#define TASK_BOX         0   // Useful for testing front camera with newPipes
#define TASK_BIN         0

#if TASK_GATE
// these define sub position
#define REF_X -3
#define REF_Y 7.5 // 3
#define REF_Z 16
#define A_P 0
#define A_Y 0
#define A_R 0
#endif

#if TASK_LIGHT
#define REF_X -1.5
#define REF_Y 3.2
#define REF_Z 0
#define A_P 0
#define A_Y 0
#define A_R 0
#endif

#if TASK_FROM_LIGHT_TO_PIPE
#define REF_X -0.061585
#define REF_Y 2.952622
#define REF_Z -3.860169
#define A_P 0
#define A_Y 19
#define A_R 0
#endif

#if TASK_LIGHT_PIPE
#define REF_X 0
#define REF_Y 1.5
#define REF_Z -4.3
#define A_P 0
#define A_Y 135
#define A_R 0
#endif

/*
#if TASK_PIPE
#define REF_X 4.5
#define REF_Y 1.0      // 2.5 is high, 1.0 is low
#define REF_Z -3.8
#define A_P 90
#define A_Y 80
#define A_R 0
#endif
*/

#if TASK_PIPE
#define REF_X 7
#define REF_Y 2.5
#define REF_Z -4.8
#define A_P 90
#define A_Y 80
#define A_R 0
#endif

#if TASK_PIPE_FOLLOW
#define REF_X 7
#define REF_Y 2.5
#define REF_Z -5.8
#define A_P 90
#define A_Y 80
#define A_R 0
#endif

#if TASK_PIPE_OFF
#define REF_X 3.0
#define REF_Y 2.5
#define REF_Z -2.3
#define A_P 90
#define A_Y 45
#define A_R 0
#endif

#if TASK_BOMB_RUN
#define REF_X 11
#define REF_Y 3.5
#define REF_Z -9
#define A_P 90
#define A_Y 0
#define A_R 0
#endif

#if TASK_MACHINE_GUN
#define REF_X 7.5
#define REF_Y 1.5
#define REF_Z -0.8
#define A_P 0
#define A_Y 90
#define A_R 0
#endif

#if TASK_X
#define REF_X 14.5
#define REF_Y 4
#define REF_Z -9
#define A_P 0
#define A_Y 0
#define A_R 0
#endif

#if TASK_BIN
#define REF_X 11
#define REF_Y 2.5
#define REF_Z -8.3
#define A_P 90
#define A_Y 45
#define A_R 0
#endif
