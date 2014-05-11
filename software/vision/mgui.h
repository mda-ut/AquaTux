/** mgui - MDA vision gui and utils
 *  includes window management and txt file reading
 *  Author - Ritchie Zhao
 */

#ifndef __MDA_VISION_MGUI__
#define __MDA_VISION_MGUI__

#include <highgui.h>
#include <stdio.h>
#include <cv.h>

#include "profile_bin.h"

// Hopefully this is the only hardcoded settings file
#define MDA_SETTINGS_DIR_ENV_VAR "MDA_VISION_SETTINGS_PATH"
#define MDA_BACKUP_SETTINGS_DIR "../settings/"
#define COMMON_SETTINGS_FILE "common_settings.csv"

// Call back to show HSV when an image pixel is clicked
void show_HSV_call_back (int event, int x, int y, int flags, void* param);

/** read_mv_setting and co. - Functions to read settings from .csv files **/
// This function searches through the file and looks for the setting, getting
//   its value if found. Template allow for int,unsigned,float versions 
#define LINE_LEN 80
#define COMMENT_CHAR '#'
template <typename TYPE>
void read_mv_setting (const char filename[], const char setting_name[], TYPE &value);
void read_mv_setting (const char filename[], const char setting_name[], std::string &value);
template <typename TYPE>
void read_common_mv_setting (const char setting_name[], TYPE &value) 
{
    return read_mv_setting (COMMON_SETTINGS_FILE, setting_name, value);
}
inline void read_common_mv_setting (const char setting_name[], std::string &value)
{
    return read_mv_setting (COMMON_SETTINGS_FILE, setting_name, value);
}

/** mvWindow - class and infrastructure for managing opencv windows **/
// This class represents a single window and lets you display images
// The first 4 windows created are moved automatically to good locations
// on the screen
#define NUM_SUPPORTED_WINDOWS 4 
#define WINDOW_NAME_LEN 50
class mvWindow {
    PROFILE_BIN bin_showImage;
    
    char _name[WINDOW_NAME_LEN];
    int _window_number;

    public:
    mvWindow (const char name[]);
    ~mvWindow (); 
    void showImage (const CvArr* image) {
        bin_showImage.start();
      if (mvWindow::show_image_val) {
        cvShowImage(_name, image);
        cvSetMouseCallback(_name, show_HSV_call_back, const_cast<void *>(image));
      }
        bin_showImage.stop();
    }
    static void setShowImage (bool val) {
        show_image_val = val;
    }
    void move (unsigned x, unsigned y) { cvMoveWindow(_name, x, y); }

    private:
    static bool show_image_val;
};

/** mvVideoWriter - class for writing to disk **/
class mvVideoWriter {
  CvVideoWriter* _writer;

  PROFILE_BIN bin_writeFrame;

public:
    mvVideoWriter (const char* filename, unsigned framerate = 30);
    ~mvVideoWriter ();

    void writeFrame (IplImage* frame) {
        bin_writeFrame.start();
      if (frame->origin) {
        IplImage *flipped = cvCreateImage (cvSize(frame->width,frame->height), IPL_DEPTH_8U, 3);
        cvConvertImage(frame, flipped, CV_CVTIMG_FLIP);
        cvWriteFrame (_writer, flipped);
        cvReleaseImage(&flipped);
      } else {
        cvWriteFrame (_writer, frame);
      }
        bin_writeFrame.stop();
    }
};

/** mvCamera - class for managing webcams and video writers **/
// This class lets you open a camera and write video to disk 
// Usage is simple. 
class mvCamera {
    static const unsigned FRAMERATE = 30;
    CvCapture* _capture;
    IplImage* _imgResized;

    PROFILE_BIN bin_resize;
    PROFILE_BIN bin_getFrame;

    public:
    mvCamera (unsigned cam_number);
    mvCamera (const char* video_file);
    ~mvCamera ();

    /* In a monsterous betrayal of good coding, the behaviour of getFrame and 
     * getFrameResized are a bit different.
     * getFrame will return a pointer to the frame in the camera's buffer. 
     * Meaning the returned img must not be freed/modified by the user.
     * getFrameResized by necessity uses a created image, and every time it is
     * called the image will be overwritten. This img also must not be freed/
     * modified by the user.
     */ 

    // grabFrame readies the latest frame only - it does not return the frame
    int grabFrame () {
        if (!_capture) return 0;
          bin_getFrame.start();
        return cvGrabFrame (_capture);
          bin_getFrame.stop();        
    }

    // retrieveFrame returns the latest frame - the frame is not updated before or after the return
    IplImage* retrieveFrame () {
        assert (0); /// RetrieveFrame is no longer safe to use with current algorithms;

        if (!_capture) return NULL;
          bin_getFrame.start();
        return cvRetrieveFrame (_capture);
          bin_getFrame.stop();        
    }
    IplImage* retrieveFrameResized () {
        if (!_capture) return NULL;
          bin_getFrame.start();
        IplImage* frame = cvRetrieveFrame (_capture);
          bin_getFrame.stop(); 

        if (!frame) {
            return NULL;
        }

          bin_resize.start();
        cvResize (frame, _imgResized, CV_INTER_NN); // nearest neighbour interpolation
          bin_resize.stop();

        return _imgResized;
    }

    // getFrame is what you want most of the time - it grabs the latest frame and returns it.
    IplImage* getFrame () {
        if (!grabFrame()) return NULL;
        return retrieveFrame();
    } 
    IplImage* getFrameResized () {
        if (!grabFrame()) return NULL;
        return retrieveFrameResized();
    }

    int set_relative_position (float pos) {
        if (!_capture) return -1;
        assert (pos >= 0 && pos <= 1);
        return cvSetCaptureProperty(_capture, CV_CAP_PROP_POS_AVI_RATIO, pos);
    }
};

#endif
