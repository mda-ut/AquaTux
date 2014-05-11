#include <stdio.h>
#include <cv.h>
#include <highgui.h>

// initialize global variables
int g_slider_position = 1; // trackbar position
CvCapture* g_capture = NULL; // structure to create a video input

// routine to be called when user moves a trackbar slider
void onTrackbarSlide(int pos) {
  cvSetCaptureProperty(
  g_capture,
  CV_CAP_PROP_POS_FRAMES,

  pos
  );
}

int main( int argc, char** argv ) {
if (argc != 3) {
    printf ("Needs 3 Arguments\n");
    return 1;
}
// create a window with appropriate size. Windows name is determined by file name
// supplied as an argument
cvNamedWindow( argv[1], CV_WINDOW_AUTOSIZE );
// open video
g_capture = cvCreateFileCapture( argv[1] );
// set read position in units of frames and retrieve total number of frames
int frames = (int) cvGetCaptureProperty(

  g_capture,
  CV_CAP_PROP_FRAME_COUNT
);


// do not create treackbar if video does not include an information
// about number of frames
if( frames!=0 ) {

  cvCreateTrackbar(
  "Position",
  argv[1],
  &g_slider_position,
  frames,

  onTrackbarSlide
  );
}

// display video frame by frame
IplImage* frame;
while(1) {

frame = cvQueryFrame( g_capture );
if( !frame ) break;
// set trackbar to a current frame position
cvSetTrackbarPos("Position", argv[1], g_slider_position);
cvShowImage (argv[1], frame);
g_slider_position++;

char c = cvWaitKey(0);

if( c == 'q' )
    break;
else if( c == 'z') {
    cvSaveImage (argv[2], frame, 0);
    break;
}

}
// free memory
cvReleaseCapture( &g_capture );
cvDestroyWindow( argv[1] );
return(0);
}
