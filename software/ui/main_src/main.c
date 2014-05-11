/** FPGA User Inteface Utilities
 *  This file contains functions that communicate with the fpga that the main
 *  program will call.
 * 
 *  IMPORTANT!!
 *    Do not use atoi, which crashes on NULL input. Use atoi_safe.
 *    Always use exit_safe() to exit, which turns power off first.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fpga_ui.h"

#define MAX_TOKENS 3
#define LINE_START "fpga> "

int main (int argc, char *argv[]) {
    spawn_term((argc >= 2) ? argv[1] : NULL);

    char cmd[51];               // stores the command that was entered
    char* token[MAX_TOKENS];    // points to parts of the command
    unsigned i;
    int pwm, motor_num, target_setting;
    
    for (;;) {
        printf (LINE_START);
        cmd_ok = 0;
        assert(fgets (cmd, 50, stdin));
        
        // use strtok to grab each part of the command 
        token[0] = strtok (cmd, " \n");
        for (i = 1; i < MAX_TOKENS; i++)
            token[i] = strtok (NULL, " \n");
        
        if (token[0] == NULL) 
            help ();
        else if (!strcmp(token[0], "q") || !strcmp(token[0], "exit")) 
            exit_safe();
        else if (!strcmp(token[0], ":q"))
            printf ("this isnt vim lol\n");
        else if (!strcmp(token[0], "help")) {
            if (token[1] == NULL) 
                help ();
            else if (!strcmp(token[1],"motor") || !strcmp(token[1],"m"))
                help_motor();
            else if (!strcmp(token[1],"dyn") || !strcmp(token[1],"d"))
                help_dyn();
            else if (!strcmp(token[1],"power") || !strcmp(token[1],"p"))
                help_power();
        }
        else if (!strcmp(token[0], "motor") || !strcmp(token[0], "m")) {
            if (token[1] == NULL || !strcmp (token[1], "status"))
                motor_status();
            else if (!strcmp (token[1], "all")) {
                pwm = atoi_safe(token[2]); // note an invalid token will cause pwm = 0, which is ok
                motor_set (pwm, H_ALL);                    
            }
            else if (!strcmp (token[1], "fwd")) {
                pwm = atoi_safe(token[2]); // note an invalid token will cause pwm = 0, which is ok
                motor_set (pwm, H_FWD);                    
            }
            else if (!strcmp (token[1], "rise")) {
                pwm = atoi_safe(token[2]); // note an invalid token will cause pwm = 0, which is ok
                motor_set (pwm, H_RISE);                    
            }
            else {
                motor_num = atoi_safe (token[1]); // 1-indexed
		motor_num--; // Internally, this is 0-indexed
                pwm = atoi_safe (token[2]);
                switch (motor_num) {
                    case M_FRONT_LEFT:  motor_set (pwm, H_FRONT_LEFT); break;
                    case M_FRONT_RIGHT: motor_set (pwm, H_FRONT_RIGHT); break;
                    case M_FWD_LEFT:    motor_set (pwm, H_FWD_LEFT); break;
                    case M_FWD_RIGHT:   motor_set (pwm, H_FWD_RIGHT); break;
                    case M_REAR:        motor_set (pwm, H_REAR); break;
                    default: printf ("**** Invalid motor number.\n");
                }
            }
        }
        else if (!strcmp(token[0], "power") || !strcmp(token[0], "p")) {
            if (token[1] == NULL || !strcmp (token[1], "status"))
                power_status();
            else if (!strcmp (token[1], "on") || !strcmp (token[1], "1")) {
                power_on();
            }
            else if (!strcmp (token[1], "start") || !strcmp (token[1], "2")) {
                startup_sequence();
            }
            else if (!strcmp (token[1], "off") || !strcmp (token[1], "0")) {
                power_off();
            }
        }
        else if (!strcmp(token[0], "dyn") || !strcmp(token[0], "d")) {
            if (token[1] == NULL)
                dyn_status();
            else if (!strcmp (token[1], "depth")) {
                target_setting = atoi_safe(token[2]); // Note an invalid token will cause target_setting = 0, which is ok
                dyn_set_target_depth(target_setting);
            }
        }
        
        if (cmd_ok != 1) 
            cmd_error();
    }
}

