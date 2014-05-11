#include "ImageInput.h"

#include <unistd.h>

ImageInputVideo::ImageInputVideo(const char* settings_file) : ImageInput(settings_file)
{
    std::string video_fwd, video_dwn;
    read_mv_setting(settings_file, "VIDEO_FWD", video_fwd);
    read_mv_setting(settings_file, "VIDEO_DWN", video_dwn);

    cam_fwd = new mvCamera (video_fwd.c_str());
    cam_dwn = new mvCamera (video_dwn.c_str());

    printf ("Forward Video File: %s\n", video_fwd.c_str());
    printf ("Down Video File: %s\n", video_dwn.c_str());
}

ImageInputVideo::~ImageInputVideo()
{
    delete cam_fwd;
    delete cam_dwn;
}

bool ImageInputVideo::ready_internal_image (ImageDirection dir)
{
    if (dir == FWD_IMG) {
        return cam_fwd ? cam_fwd->grabFrame() : false;
    }
    if (dir == DWN_IMG) {
        return cam_dwn ? cam_dwn->grabFrame() : false;
    }
    return false;
}

IplImage *ImageInputVideo::get_internal_image(ImageDirection dir)
{
    usleep(100000); // simulate the actual framerate of the webcam images
    if (dir == FWD_IMG) {
        return cam_fwd ? cam_fwd->retrieveFrameResized() : NULL;
    }
    if (dir == DWN_IMG) {
        return cam_dwn ? cam_dwn->retrieveFrameResized() : NULL;
    }
    return NULL;
}
