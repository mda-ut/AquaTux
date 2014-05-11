#include "ImageInput.h"
#include "SimulatorSingleton.h"

#include <unistd.h>

ImageInputSimulator::ImageInputSimulator(const char *settings_file) : ImageInput(settings_file, false)
{
  SimulatorSingleton::get_instance().register_instance();
}

ImageInputSimulator::~ImageInputSimulator()
{
}

bool ImageInputSimulator::ready_internal_image(ImageDirection dir)
{
    return true;
}

IplImage *ImageInputSimulator::get_internal_image(ImageDirection dir)
{
  usleep(100000); // simulate the actual framerate of the webcam images
  return SimulatorSingleton::get_instance().get_image(dir);
}
