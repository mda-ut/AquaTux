#include <highgui.h>
#include <cv.h>

int main( int argc, char** argv ) {
    IplImage* img = cvLoadImage( argv[1]);  // load an image. Memory is allocated automatically
    cvNamedWindow( "window1", CV_WINDOW_AUTOSIZE ); // create a window
    cvShowImage( "window1", img );          // show image using window
    cvWaitKey(0);                   // wait for user to press a key

    cvReleaseImage( &img );       // deallocate image memory
    cvDestroyWindow("window1" );  // destroy the window
}
