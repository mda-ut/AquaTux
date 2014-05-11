#include <cv.h>
#include <highgui.h>
main( int argc, char* argv[] ) {

CvCapture* capture = NULL;

cvNamedWindow( "Recording ...press ESC to stop !", CV_WINDOW_AUTOSIZE );

capture = cvCreateCameraCapture( 1 );

IplImage *frames = cvQueryFrame(capture);

// get a frame size to be used by writer structure
CvSize size = cvSize (
    (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH),
    (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT)
);
CvVideoWriter *writer = cvCreateVideoWriter(
    argv[1],

    CV_FOURCC('M','J','P','G'),
    30, // set fps
    size,
    1
);

// get frame rate, doesnt work for some reason
//double frate = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
//int delay = (int)(1000.0/frate);
/*
cvReleaseCapture ( &capture );

capture = cvCreateCameraCapture( 1 );
*/
while(1) {

 frames = cvQueryFrame( capture );
 if( !frames ) break;
 cvShowImage( "Recording ...press ESC to stop !", frames );
 cvWriteFrame( writer, frames );

 char c = cvWaitKey(33);
   if( c == 27 ) break;
}
cvReleaseVideoWriter( &writer );
cvReleaseCapture ( &capture );
cvDestroyWindow ( "Recording ...press ESC to stop !");

return 0;

}
