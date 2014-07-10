#include "mvColorFilter.h"

#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif
    
mvHSVFilter::mvHSVFilter (const char* settings_file) :
    bin_WorkingLoop ("HSV - Working Loop"),
    bin_CvtColor ("HSV - CvtColor")
 {
    read_mv_setting (settings_file, "HUE_MIN", HMIN);
    read_mv_setting (settings_file, "HUE_MAX", HMAX);
    read_mv_setting (settings_file, "SAT_MIN", SMIN);
    read_mv_setting (settings_file, "SAT_MAX", SMAX);
    read_mv_setting (settings_file, "VAL_MIN", VMIN);
    read_mv_setting (settings_file, "VAL_MAX", VMAX);
    
    HMIN = (HMIN>=0) ? HMIN : HMIN+180; 
    HMAX = (HMAX<180) ? HMAX : HMAX-180; 

    scratch_3 = mvGetScratchImage_Color();
}

mvHSVFilter::~mvHSVFilter () {
    mvReleaseScratchImage_Color();
}

void mvHSVFilter::filter_internal (IplImage* HSV_img, IplImage* result) {
    assert (HSV_img != NULL);
    assert (HSV_img->nChannels == 3);
    assert (result != NULL);
    assert (result->nChannels == 1);
    assert (HSV_img->width == result->width);
    assert (HSV_img->height == result->height);
    
      bin_WorkingLoop.start();

    /* go through each pixel, set the result image's pixel to 0 or 255 based on whether the
     * origin HSV_img's HSV values are withing bounds
     */
    unsigned char *imgPtr, *resPtr;
    for (int r = 0; r < result->height; r++) {                         
        imgPtr = (unsigned char*) (HSV_img->imageData + r*HSV_img->widthStep); // imgPtr = first pixel of rth's row
        resPtr = (unsigned char*) (result->imageData + r*result->widthStep);
        
        for (int c = 0; c < result->width; c++) {
            if (hue_in_range (*imgPtr, HMIN, HMAX) && 
                (*(imgPtr+1) >= SMIN) && (*(imgPtr+1) <= SMAX) &&
                (*(imgPtr+2) >= VMIN) && (*(imgPtr+2) <= VMAX))
            {
                *resPtr = 255;
            }
            else 
                *resPtr = 0;
            
            imgPtr+=3; resPtr++;
        }
    }
      bin_WorkingLoop.stop();
}

