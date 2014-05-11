#include <highgui.h>
#include <cv.h>

int main( int argc, char** argv ) {
    IplImage* img = cvLoadImage( argv[1]);  
    cvNamedWindow( "window1", CV_WINDOW_AUTOSIZE );
    cvShowImage( "window1", img );       
    cvWaitKey(0);                   

    // additional code
    cvSmooth (img, img, CV_GAUSSIAN, 15);   // blur image
    cvShowImage( "window1", img );          // show image using window
    cvWaitKey(0);

    cvReleaseImage( &img );       
    cvDestroyWindow("window1" ); 
}
