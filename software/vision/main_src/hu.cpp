#include <stdio.h>
#include <cv.h>

#include "mgui.h"
#include "mvColorFilter.h"
#include "mv.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s <image file>\n", argv[0]);
        return 1;
    }

    mvHSVFilter HSVFilter ("HSVFilter_settings.csv"); // color filter

    // windows to display stuff
    mvWindow win1("filtered");

    IplImage * temp = cvLoadImage(argv[1], CV_LOAD_IMAGE_COLOR);
    IplImage * resized = cvCreateImage (cvSize(temp->width,(temp->height)*2),IPL_DEPTH_8U,3);
    IplImage * filtered = mvCreateImage(resized); // filtered is greyscale 

	win1.showImage (temp);
    printf("Here's your image. Press a key to resize.\n");
    cvWaitKey(0);

    cvResize(temp,resized,CV_INTER_CUBIC);

    win1.showImage (resized);
    printf("Here's your resized image. Press a key to filter.\n");
    cvWaitKey(0);

    HSVFilter.filter_non_common_size (resized, filtered);

    double hus[7];
    mvHuMoments(filtered,hus);

	printf("Hu moments:\n\n{");
    for(int i=0;i<7;++i){
    	printf("%lf, ", hus[i]);
    }
    printf("\b\b}\n\n");

    win1.showImage(filtered);
    printf("Here's the filtered image that was used. Press a key to exit.\n");
    cvWaitKey(0);

    cvReleaseImage(&temp);

    return 0;
}
