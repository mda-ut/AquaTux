#include "ImageInput.h"

ImageInputWebcam::ImageInputWebcam(const char* settings_file) : ImageInput(settings_file), fwdCam(NULL), dwnCam(NULL)
{
   int webfwdCam, webdwnCam;
   read_mv_setting(settings_file, "WEBCAM_FWD", webfwdCam);
   read_mv_setting(settings_file, "WEBCAM_DWN", webdwnCam);

#ifdef DISABLE_DOUBLE_WEBCAM
    fwd_cam_number = webfwdCam;
    dwn_cam_number = webdwnCam;
    fwdCam = new mvCamera (fwd_cam_number);
    dwnCam = NULL;
#else
   fwdCam = new mvCamera(webfwdCam);
   if (webdwnCam != webfwdCam) {
      dwnCam = new mvCamera(webdwnCam);
   }
#endif
}

ImageInputWebcam::~ImageInputWebcam()
{
   delete fwdCam;
   delete dwnCam;
}

bool ImageInputWebcam::ready_internal_image (ImageDirection dir)
{
    if (dir == FWD_IMG) {
#ifdef DISABLE_DOUBLE_WEBCAM
        if (!fwdCam) {
            delete dwnCam;
            dwnCam = NULL;
            fwdCam = new mvCamera (fwd_cam_number);
        }
#endif
        return fwdCam ? fwdCam->grabFrame() : false;
    }
    if (dir == DWN_IMG) {
#ifdef DISABLE_DOUBLE_WEBCAM
        if (!dwnCam) {
            delete fwdCam;
            fwdCam = NULL;
            dwnCam = new mvCamera (dwn_cam_number);
        }
#endif
        return dwnCam ? dwnCam->grabFrame() : false;
    }
    return false;
}

IplImage *ImageInputWebcam::get_internal_image(ImageDirection dir)
{
   if (dir == FWD_IMG) {
#ifdef DISABLE_DOUBLE_WEBCAM
        if (!fwdCam) {
            delete dwnCam;
            dwnCam = NULL;
            fwdCam = new mvCamera (fwd_cam_number);
            fwdCam->grabFrame();
        }
#endif
      return fwdCam ? fwdCam->retrieveFrameResized() : NULL;
   }
   if (dir == DWN_IMG) {
#ifdef DISABLE_DOUBLE_WEBCAM
        if (!dwnCam) {
            delete fwdCam;
            fwdCam = NULL;
            dwnCam = new mvCamera (dwn_cam_number);
            dwnCam->grabFrame();
        }
#endif
      return dwnCam ? dwnCam->retrieveFrameResized() : NULL;
   }
   return NULL;
}
