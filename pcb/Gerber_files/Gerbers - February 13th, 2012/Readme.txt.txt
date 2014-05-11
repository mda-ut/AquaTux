This run of gerbers was produced in the winter of 2012. 

Three unique boards:

Interface board - For interfacing between the DE0 Nano (Cyclone IV FPGA) and what we call the "main stack". This is essentially a breakout board, with a massive 120 pin SMT board-to-board connector. -- Designed by Jordan Varley. 

Power Management Board - Reponsible for switching the battery voltage with a relay controlled via kill switch and the FPGA. Connects to 3 DC-DCs (3.3, 5, and 12V), and filters their outputs with on board LC filters. Also checks that all output voltages are within desirable bounds +/- 5% with a series of on board comparators. -- Designed by Alex Piggott. 

Motor Driver Board (or Motor Driver 6 channel) - Takes 12 GPIOs off the main stack to drive 6 half bridges (arranged as pairs of Full bridges). Used to drive high power actuators: Solenoids, the submarine's motors, steppers, etc... -- Design ed by David Biancolin 


These gerbers should permit you to produce a second run of the boards event if the source files are misplaced or not self -consistent. Sometimes shit gets lost. 

-- David Biancolin (Dec, 2012)  

