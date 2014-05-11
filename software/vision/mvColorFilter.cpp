#include "mvColorFilter.h"

#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

struct mvTarget {
    unsigned char h, s, v;
};

char mDistance(int a, int b, int c, int x, int y, int z) {
    return((4*std::min(abs(x-a),180-abs(x-a)) + abs(y-b) + abs(z-c))/5);
}

void ManhattanDistanceFilter(IplImage* src, IplImage* dst) {
    mvTarget targets[] = {{50,130,60},{100,80,40}};

    unsigned char minDist, tempDist;
    unsigned char* imgPtr, *resPtr;

    IplImage * HSVImg = mvGetScratchImage_Color();
    cvCvtColor (src, HSVImg, CV_BGR2HSV);

    for (int r = 0; r < HSVImg->height; r++) {                         
        imgPtr = (unsigned char*) (HSVImg->imageData + r*HSVImg->widthStep); // imgPtr = first pixel of rth's row
        resPtr = (unsigned char*) (dst->imageData + r*dst->widthStep);
        
        for (int c = 0; c < dst->width; c++) {
            minDist = 255;
            for(int i =0; i<2; i++){
                tempDist = mDistance(*imgPtr, *(imgPtr+1), *(imgPtr+2), targets[i].h, targets[i].s, targets[i].v);
                if(tempDist < minDist) minDist = tempDist;
            }
            *resPtr = minDist;
            imgPtr+=3; resPtr++;
        }
    }

    mvReleaseScratchImage_Color();
}
