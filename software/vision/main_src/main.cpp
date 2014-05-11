/** The main function is just used to test various parts of vision. Rewrite
 *  as needed 
 */
#include <stdlib.h>
#include <stdio.h>
#include <highgui.h>
#include <cv.h>
//#include <highgui.h>

#include "mv.h"
#include "mgui.h"
#include "mvColorFilter.h"
#include "mvLines.h"
#include "mvShapes.h"

int main (int argc, char** argv) {
    // We want to do HSV color filter, take the gradient of result,
    // and run line finding on the gradient img
    /*assert (argc == 2); // need image as first argument
    
    /// Grab the filters and data structs we need
    mvHSVFilter HSVFilter ("HSVFilter_settings.csv"); // color filter
    mvHoughLines HoughLines ("HoughLines_settings.csv");
    mvLines lines; // data struct to store lines

    // windows to display stuff
    mvWindow win1("img");
    mvWindow win2("filtered");
    mvWindow win3("gradient & lines");
    
    unsigned width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
    printf ("Image Size: %dx%d\n", width,height);
    
    // process the img
    IplImage * temp = cvLoadImage(argv[1], CV_LOAD_IMAGE_COLOR);
    IplImage * img = mvCreateImage_Color (); // img will be common width and height
    cvResize (temp, img);
    win1.showImage (img);
    
    IplImage * res = mvCreateImage (img); // res is greyscale 
    HSVFilter.filter (img, res);
    win2.showImage (res);
    
    HoughLines.findLines (res, &lines);
    lines.drawOntoImage (res);
    
    win3.showImage(res);
        
    cvWaitKey(0);
    */
    
    mvWindow window ("My First Window...yay...");
    IplImage* colourImage = cvLoadImage("color1.jpg", CV_LOAD_IMAGE_COLOR);
    IplImage* colourResize = mvCreateImage_Color();
    cvResize (colourImage, colourResize);
    
    
    window.showImage(colourResize);
    printf ("Width: %d, Height: %d, nChannels: %d, origin: %d, widthStep: %d\n", colourImage->width, colourImage->height, colourImage->nChannels, colourImage->origin, colourImage->widthStep);
    cvWaitKey(0);
/*    
    IplImage* binaryImage = cvCreateImage (cvSize(colourImage->width,colourImage->height),
					   IPL_DEPTH_8U, 
					   1
					  );
*/
    IplImage* binaryResize = mvCreateImage();
    
    mvHSVFilter HSVFilter ("test_settings.csv");
    HSVFilter.filter_non_common_size (colourResize, binaryResize);
    window.showImage(binaryResize);
    cvWaitKey(0);
    
    mvBinaryMorphology morph(5,5,MV_KERN_ELLIPSE);
    morph.close (binaryResize, binaryResize);
    window.showImage(binaryResize);
    cvWaitKey(0);
    
    return 0;
}
