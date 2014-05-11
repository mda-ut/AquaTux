#include "scripts.h" /* Must be included */

// Other include files (only system header files should be here)
#include <unistd.h>

// Define a main function. Any other code you wish to write in functions
// should be in the same file.

// Once you're done, just type make and it should produce an executable
// called <name of source file>.out
int main()
{
  // Initialize by connecting to the FPGA
  init_fpga();

  // Power on the system
  power_on();

  /* YOUR CODE GOES HERE */

  sleep(2); // Pause for 2 seconds

  int x, y, z, d; // Get acceleration vector and depth
  get_accel(&x, &y, &z);
  d = get_depth();

  printf("Acceleration is: (%d, %d, %d)\n", x, y, z);
  printf("Current depth is: %d\n", d);

  int p = get_power(); // Check if power is still on

  printf("Power system %s\n", p ? "is good": "has failed");

  puts("Exiting...");

  /* END */

  // Power off and release resources
  exit_safe();
  return 0;
}
