#include "mda_vision.h"

#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

#define ABS(X) (((X)>0) ? (X) : (-(X)))

const char MDA_VISION_MODULE_MARKER::MDA_VISION_MARKER_SETTINGS[] = "vision_marker_settings.csv";

/// ########################################################################
/// MODULE_MARKER methods
/// ########################################################################
MDA_VISION_MODULE_MARKER::MDA_VISION_MODULE_MARKER () :
	window (mvWindow("Marker Vision Module")),
	HSVFilter (mvHSVFilter(MDA_VISION_MARKER_SETTINGS))
{
    targets_found[0] = false;
    targets_found[1] = false;
    filtered_img = mvGetScratchImage();
    //_filtered_img->origin = 1;
}

MDA_VISION_MODULE_MARKER::~MDA_VISION_MODULE_MARKER () {
    mvReleaseScratchImage();
}

void MDA_VISION_MODULE_MARKER::primary_filter (IplImage* src) {   
    /** src is an image that is passed in from the simulator. It is 3 channel
     *  Because it is const you may need to deep copy (not pointer copy) it 
     *  to your own IplImage first before you can modify it.
     *  I've done this with the variable img
     */

    /** YOUR CODE HERE. DO STUFF TO img */

    HSVFilter.filter (src, filtered_img);
    filtered_img->origin = src->origin;

    // this line displays the img in a window
    window.showImage (filtered_img);
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_MARKER::calc_vci () {
    MDA_VISION_RETURN_CODE retval = NO_TARGET;
    double target[2][7] = {
                           {0.256755, 0.001068, 0.006488, 0.001148, 0.000003, 0.000037, -0.000000}, //the plane
                           {0.240729, 0.027823, 0.000024, 0.000029, -0.000000, 0.000004, 0.000000} //the tank
                          };
    
    double hu[7];
    mvHuMoments(filtered_img, hu);

    double dist = 0;

    for (int j = 0; j < 2; ++j)
    {
        printf("{");
        for (int i = 0; i < 7; ++i)
        {
            printf("%lf, ", hu[i]);
            dist += fabs(hu[i] - target[j][i]);
        }
        printf("\b\b}: %lf\n",dist);
    
        if (dist < HU_THRESH){
            retval = FULL_DETECT;
            printf("Match found!! :)\n");
            targets_found[j] = true;
        }
        dist = 0;
    }

    return retval;
}
