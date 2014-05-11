#include <unistd.h>

#include "mgui.h"
#include "mv.h"

/** mvWindow methods **/
static bool WINDOWS_ARRAY[NUM_SUPPORTED_WINDOWS] = {false,false,false,false};

bool mvWindow::show_image_val = true;

mvWindow:: mvWindow (const char name[]) :
    bin_showImage ("mvWindow - showImage"), _window_number(-1)
{ // this has to be the h file
    assert (strlen(name) < WINDOW_NAME_LEN);
    sprintf (_name, "%s", name);

    cvNamedWindow (_name, CV_WINDOW_AUTOSIZE);

    unsigned i = 0;
    while (i < NUM_SUPPORTED_WINDOWS && WINDOWS_ARRAY[i] == true) 
        i++;                    // find next free slot

    if (i >= NUM_SUPPORTED_WINDOWS) return; // return if no slot
    WINDOWS_ARRAY[i] = true;    // mark slot as used
    _window_number = i;
    
    switch (_window_number) {
        case 0: cvMoveWindow (_name, 400,10); break;
        case 1: cvMoveWindow (_name, 810,10); break;
        case 2: cvMoveWindow (_name, 400,360); break;
        case 3: cvMoveWindow (_name, 810,360); break;
    }
}

mvWindow:: ~mvWindow () { 
    cvDestroyWindow (_name); 
    if (_window_number != -1) {
        WINDOWS_ARRAY[_window_number] = false;
    }
}

/** mvVideoWriter methods */
mvVideoWriter:: mvVideoWriter (const char* filename, unsigned framerate) :
    bin_writeFrame ("mvVideoWriter - writeFrame")
{
    unsigned width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);

    _writer = cvCreateVideoWriter (
        filename,               // video file name
        CV_FOURCC('P','I','M','1'), // codec
        framerate,                  // framerate
        cvSize(width,height),       // size
        1
    );
}

mvVideoWriter:: ~mvVideoWriter () {
    cvReleaseVideoWriter (&_writer);
}

/** mvCamera methods **/
mvCamera:: mvCamera (unsigned cam_number) :
    bin_resize ("mvCamera - resize"),
    bin_getFrame ("mvCamera - getFrame")
{
    //unsigned width, height;
    //read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    //read_common_mv_setting ("IMG_HEIGHT_COMMON", height);

    _capture = cvCreateCameraCapture (cam_number);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH, 400);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT, 300);
    _imgResized = mvGetScratchImage_Color();
}

mvCamera:: mvCamera (const char* video_file) :
    bin_resize ("mvCamera - resize"),
    bin_getFrame ("mvCamera - getFrame")
{
    unsigned width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
    
    _capture = cvCreateFileCapture (video_file);   
    _imgResized = mvGetScratchImage_Color();
}

mvCamera:: ~mvCamera () {
    cvReleaseCapture (&_capture);
    mvReleaseScratchImage_Color();
}

void show_HSV_call_back (int event, int x, int y, int flags, void* param) {
// param must be the IplImage* pointer, with HSV color space    
    IplImage* img = (IplImage*) param;
    unsigned char * imgPtr;

    if (img == NULL) // if the image was already deallocated
        return;
    
    if (event == CV_EVENT_LBUTTONDOWN) {
        // print the HSV values at x,y
        imgPtr = (unsigned char*) img->imageData + y*img->widthStep + x*img->nChannels;
        unsigned char b = imgPtr[0];
        unsigned char g = imgPtr[1];
        unsigned char r = imgPtr[2];
        unsigned char h, s, v;
        tripletBGR2HSV(b, g, r, h, s, v);
        printf ("(%3d,%3d):  HSV:|%3hhu|%3hhu|%3hhu|  BGR:|%3hhu|%3hhu|%3hhu|\r\n", x,y,h,s,v,b,g,r);
        fflush(stdout);
    }

    if (event == CV_EVENT_RBUTTONDOWN) {
        usleep (500000);
    }
}
