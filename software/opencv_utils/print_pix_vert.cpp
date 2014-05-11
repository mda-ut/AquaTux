#include <cv.h>
#include <highgui.h>
#include <fstream>

#define FILENAME "pix_vert.txt"
#define COL 10
#define COL2 200
#define COL3 300

int main( int argc, char** argv ) {
    IplImage* img = cvLoadImage( argv[1], 
        (CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH));
    // load image with name argv[1]
    //cvNamedWindow( argv[1], CV_WINDOW_AUTOSIZE );
    //cvShowImage( argv[1], img );
    
    cvCvtColor (img, img, CV_BGR2HSV);

    FILE* fp = fopen (FILENAME, "w");
// greyscale 
    
    unsigned char* rowPtr;
    for (int r = 0; r < img->height; r++) {
        // points to rth rol and COL column
        rowPtr = (unsigned char*) img->imageData + r*img->widthStep + COL*img->nChannels; 
        for (int ch = 0; ch < img->nChannels; ch++) // print each channel
            fprintf (fp, "%u ", *rowPtr++);
        fprintf (fp, "   ");
        
        rowPtr = (unsigned char*) img->imageData + r*img->widthStep + COL2*img->nChannels;
        for (int ch = 0; ch < img->nChannels; ch++) 
            fprintf (fp, "%u ", *rowPtr++);
        fprintf (fp, "   ");
        
        rowPtr = (unsigned char*) img->imageData + r*img->widthStep + COL3*img->nChannels;
        for (int ch = 0; ch < img->nChannels; ch++) 
            fprintf (fp, "%u ", *rowPtr++);
        fprintf (fp, "   ");
        
        fprintf (fp, "\n");
    }
    return 0;
}
