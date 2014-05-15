#include "mvColorFilter.h"
#include <math.h>

#define USE_BGR_COLOR_SPACE
#define FLOOD_IMAGE_COMPARE_WITH_ORIG

#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

bool triple_has_more_pixels (COLOR_TRIPLE t1, COLOR_TRIPLE t2) {
    return (t1.n_pixels > t2.n_pixels);
}
void cvt_img_to_HSV (IplImage* src, IplImage* dst) {
    #ifndef USE_BGR_COLOR_SPACE
        cvCvtColor(src, dst, CV_BGR2HSV);
    #endif
}
void cvt_img_to_BGR (IplImage* src, IplImage* dst) {
    #ifndef USE_BGR_COLOR_SPACE
        cvCvtColor(src, dst, CV_HSV2BGR);
    #endif
}
