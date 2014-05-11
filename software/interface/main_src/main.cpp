/*
* Main file that get compiled as the aquatux executable
*   Pass in an argument specifying what settings file to use to configure what the inputs and outputs should be.
*   If no argument is passed in, use the default settings file (default_aquatux.csv).
*   The argument is used to construct an AquaTux object (see AquaTux.h for more details).
*/

#include "AquaTux.h"

int main(int argc, char **argv)
{
  if (argc > 1) {
    AquaTux at = AquaTux(argv[1]);
    at.work();
  } else {
    AquaTux at = AquaTux("default_aquatux.csv");
    at.work();
  }

  return 0;
}
