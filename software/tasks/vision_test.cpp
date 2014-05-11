#include "mda_vision.h"

#define ABS(X) (((X)>0) ? (X) : (-(X)))
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

const char MDA_VISION_MODULE_TEST::MDA_VISION_TEST_SETTINGS[] = "vision_test_settings.csv";

/// ########################################################################
/// MODULE_TEST methods
/// ########################################################################
MDA_VISION_MODULE_TEST:: MDA_VISION_MODULE_TEST () :
	window (mvWindow("Test Vision Module")),
    window2 (mvWindow("Test Vision Module 2")),
	HSVFilter (mvHSVFilter(MDA_VISION_TEST_SETTINGS)),
	AdvancedColorFilter (MDA_VISION_TEST_SETTINGS),
    HoughLines (mvHoughLines(MDA_VISION_TEST_SETTINGS)),
    histogram_filter ("vision_gate_settings.csv"),
    bin_test ("Test Module")
{
    color_img = mvCreateImage_Color();
    gray_img = mvCreateImage();
    gray_img_2 = mvCreateImage();
    //_gray_img->origin = 1;
}

MDA_VISION_MODULE_TEST:: ~MDA_VISION_MODULE_TEST () {
    cvReleaseImage(&gray_img);
    cvReleaseImage(&gray_img_2);
    cvReleaseImage(&color_img);
}

void MDA_VISION_MODULE_TEST:: primary_filter (IplImage* src) {   
    /** src is an image that is passed in from the simulator. It is 3 channel
     *  Because it is const you may need to deep copy (not pointer copy) it 
     *  to your own IplImage first before you can modify it.
     *  I've done this with the variable img
     */

    /** YOUR CODE HERE. DO STUFF TO img */
    bin_test.start();
    
    // do the filter - easy!
    WatershedFilter.watershed(src, gray_img);
    window.showImage (gray_img);

    bin_test.stop();

    // this line displays the img in a window
    window2.showImage (gray_img);
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_TEST:: calc_vci () {
    MDA_VISION_RETURN_CODE retval = NO_TARGET;
    return retval;
}
