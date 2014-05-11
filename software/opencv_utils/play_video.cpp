#include <cv.h>
#include <highgui.h>

int main( int argc, char** argv ) {
    CvCapture* g_capture;   // structure to create a video input
    g_capture = cvCreateFileCapture( argv[1] );  // load video 

    cvNamedWindow(argv[1], CV_WINDOW_AUTOSIZE );
  
    IplImage* frame; // will be used store 1 frame for video
    
    while(1) {       // display video frame by frame
        frame = cvQueryFrame( g_capture );  // grab next frame of video
        if( !frame ) break;                 // if end of video then stop loop
        
        cvShowImage( argv[1], frame );      // show the frame

        char c = cvWaitKey(66);      // wait 66 milliseconds 15 fps)
        if( c == 'q' ) break;        // quit if 'q' is pressed during that time
    }

    cvReleaseCapture( &g_capture );
    cvDestroyWindow( argv[1] );
    return(0);
}
