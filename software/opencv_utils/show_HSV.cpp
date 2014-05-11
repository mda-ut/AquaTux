#include <stdio.h>
#include <cv.h>
#include <highgui.h>

void callBack (int event, int x, int y, int flags, void* param) {
// param must be the IplImage* pointer, with HSV color space    
    IplImage* img = (IplImage*) param;
    unsigned char * imgPtr;
    
    if (event == CV_EVENT_LBUTTONDOWN) {
        // print the HSV values at x,y
        imgPtr = (unsigned char*) img->imageData + y*img->widthStep + x*img->nChannels;
        printf ("(%d,%d):  %u  %u  %u\n", x,y,imgPtr[0],imgPtr[1],imgPtr[2]);
    }
}

int main (int argc, char** argv) {
	  IplImage* img;

  	if (argc == 2) {
			  printf ("Loading %s\n", argv[1]);
        img = cvLoadImage (argv[1], 1);
    		cvNamedWindow ("Image");
    		cvShowImage ("Image", img);

				printf ("Assuming image is BGR. Use -c as second arg for HSV image.\n");
        cvCvtColor (img, img, CV_BGR2HSV);
		}
		else if (argc == 3) {
			  printf ("Loading %s\n", argv[2]);
        img = cvLoadImage (argv[2], 1);
				//cvCvtColor (img,img, CV_HSV2RGB);
    	  cvNamedWindow ("Image");
    		cvShowImage ("Image", img);

				//cvCvtColor (img,img, CV_RBG2HSV);
	  }
		else {
        printf ("Indicate name of image as argument\n");
        return 1;
    } 
    
		printf ("\nClick to Print HSV values. 'q' to Exit\n");
    
    
    cvSetMouseCallback ("Image", callBack, img);
    
    char key;
    for (key = 0; key != 'q';) 
        key = cvWaitKey(5);
    
    cvDestroyWindow ("Image");
    cvReleaseImage (&img);
    return 0;
}
