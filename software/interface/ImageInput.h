/* ImageInput interface.

   This interface represents the eyes of the submarine. It can return a forward image
   or a below image.

   Subclasses must implement IplImage* get_internal_image(ImageDirection), which returns either a forward or a below image.
   The base class implements some utility functions that does not depend on how get_internal_image() is implemented:
     - Display the raw image stream when accessed (may be suppressed by the subclass)
     - Write image stream to two video files
     - Take a 'screenshot', stored to two images (see dump_images)
*/

#ifndef IMAGE_INPUT_H
#define IMAGE_INPUT_H

#include <cv.h>
#include <highgui.h>
#include <sstream>
#include <ctime>
#include "mgui.h"

//#define DISABLE_DOUBLE_WEBCAM  // define this if computer can't support 2 webcams at the same time 

enum ImageDirection {
  FWD_IMG,
  DWN_IMG
};

/* Image Input interface */
class ImageInput {
  public:
    ImageInput(const char *settings_file, bool _can_display_images = true) : writer_fwd(NULL), writer_dwn(NULL), window_fwd(NULL), window_dwn(NULL), can_display_images(_can_display_images)
    {
      if (!settings_file) {
        return;
      }

      if (can_display_images) {
        bool show_raw;
        read_mv_setting(settings_file, "IMAGE_SHOW_RAW", show_raw);
        if (show_raw) {
          window_fwd = new mvWindow("Forward Cam");
          window_dwn = new mvWindow("Down Cam");
        }
      }

      // write to video?
      bool write_to_video;
      read_mv_setting(settings_file, "WRITE_TO_VIDEO", write_to_video);
      if (write_to_video) {
        std::string video_file;

	// Add timestamp to file
	time_t rawtime;
  	struct tm * timeinfo;
  	char buffer[80];

  	time(&rawtime);
  	timeinfo = localtime(&rawtime);
  	strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);

	read_mv_setting(settings_file, "FWD_VIDEO_FILE", video_file);
  	std::string file_time_stamp = std::string(buffer) + video_file;
	writer_fwd = new mvVideoWriter(file_time_stamp.c_str());

        read_mv_setting(settings_file, "DWN_VIDEO_FILE", video_file);
        file_time_stamp = std::string(buffer) + video_file;
	writer_dwn = new mvVideoWriter(video_file.c_str());
      }
    }
    virtual ~ImageInput()
    {
      delete writer_fwd;
      delete writer_dwn;
      delete window_fwd;
      delete window_dwn;
    }

    bool can_display() { return can_display_images; }
    virtual bool ready_image(ImageDirection dir = FWD_IMG)
    {
      bool ret = ready_internal_image(dir);

      if (!ret) {
        return ret;
      }

      if (   (dir == FWD_IMG && !writer_fwd)
          || (dir == DWN_IMG && !writer_dwn)) {
        return ret;
      }

      IplImage *frame = get_internal_image(dir);

      // show, write image if configured to
      if (dir == FWD_IMG) {
        if (window_fwd) {
          window_fwd->showImage(frame);
        }
        if (writer_fwd) {
          writer_fwd->writeFrame(frame);
        }
      } else if (dir == DWN_IMG) {
        if (window_dwn) {
          window_dwn->showImage(frame);
        }
        if (writer_dwn) {
          writer_dwn->writeFrame(frame);
        }
      }

      return ret;
    }

    IplImage* get_image(ImageDirection dir = FWD_IMG)
    {
      if (!ready_internal_image(dir)) {
        return NULL;
      }
      IplImage *frame = get_internal_image(dir);
      if (!frame) {
        return frame;
      }
      // show, write image if configured to
      if (dir == FWD_IMG) {
        if (window_fwd) {
          window_fwd->showImage(frame);
        }
        if (writer_fwd) {
          writer_fwd->writeFrame(frame);
        }
      } else if (dir == DWN_IMG) {
        if (window_dwn) {
          window_dwn->showImage(frame);
        }
        if (writer_dwn) {
          writer_dwn->writeFrame(frame);
        }
      }

      return frame;
    }

    void dump_images()
    {
      static int count = 0;

      IplImage *img_fwd = get_image();
      if (img_fwd) {
        std::ostringstream oss;
        oss << "image_fwd_" << count << ".png";
        cvSaveImage (oss.str().c_str(), img_fwd);
      }

      IplImage *img_dwn = get_image(DWN_IMG);
      if (img_dwn) {
        std::ostringstream oss;
        oss << "image_dwn_" << count << ".png";
        cvSaveImage (oss.str().c_str(), img_dwn);
      }

      count++;
    }

  protected:
    // To be implemented by subclasses
    // This readies but does not return the next image. Only useful for video/webcam.
    virtual bool ready_internal_image(ImageDirection dir) = 0;
    // This only returns the next available image. The image is not updated in webcam.
    virtual IplImage* get_internal_image(ImageDirection dir) = 0;

    // member variables
    mvVideoWriter *writer_fwd, *writer_dwn;
    mvWindow *window_fwd, *window_dwn;
    bool can_display_images;
};

/* A don't care implementation */
class ImageInputNull : public ImageInput {
  public:
    ImageInputNull() : ImageInput(NULL, false) {}
    virtual ~ImageInputNull() {}

  protected:
    virtual bool ready_internal_image(ImageDirection dir = FWD_IMG) {return false;}
    virtual IplImage* get_internal_image(ImageDirection dir = FWD_IMG) {return NULL;}
};

/* Simulator implementation */
class ImageInputSimulator : public ImageInput {
  public:
    ImageInputSimulator(const char* settings_file);
    virtual ~ImageInputSimulator();

  protected:
    virtual bool ready_internal_image(ImageDirection dir = FWD_IMG);
    virtual IplImage* get_internal_image(ImageDirection dir = FWD_IMG);
};

/* Read from video file */
class ImageInputVideo : public ImageInput {
  public:
    ImageInputVideo(const char* settings_file);
    virtual ~ImageInputVideo();

  protected:
    virtual bool ready_internal_image(ImageDirection dir = FWD_IMG);
    virtual IplImage* get_internal_image(ImageDirection dir = FWD_IMG);
  
  private:
    mvCamera* cam_fwd;
    mvCamera* cam_dwn;
};

/* Read from webcam */
class ImageInputWebcam : public ImageInput {
  public:
    ImageInputWebcam(const char* settings_file);
    virtual ~ImageInputWebcam();

    bool ready_image(ImageDirection dir = FWD_IMG)
    {
#ifdef DISABLE_DOUBLE_WEBCAM
        return false;
#else
        return ImageInput::ready_image(dir);
#endif
    }

  protected:
    virtual IplImage* get_internal_image(ImageDirection dir = FWD_IMG);
    virtual bool ready_internal_image(ImageDirection dir = FWD_IMG);

  private:
#ifdef DISABLE_DOUBLE_WEBCAM
    int fwd_cam_number, dwn_cam_number;
#endif
    mvCamera *fwdCam;
    mvCamera *dwnCam;
};

#endif
