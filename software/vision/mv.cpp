#include "mv.h"
#include <stdlib.h>

#define MV_DEBUG
#ifdef MV_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

void mvDumpPixels (IplImage* img, const char* file_name, char delimiter) {
    FILE* fp = fopen (file_name, "w");
    if (fp == NULL)
        return;

    const char d = delimiter;
    fprintf (fp, "Height:%c%d%c%cWidth:%c%d%c%cnChannels:%c%d\n\n", d,img->height,d,d,d,img->width,d,d,d,img->nChannels);

    unsigned char* imgPtr;
    for (int i = 0; i < img->nChannels; i++) {
        fprintf (fp, "channel %d\n\n", i+1);
        for (int r = 0; r < img->height; r++) {
            imgPtr = (unsigned char*) (img->imageData + r*img->widthStep );
            for (int c = 0; c < img->width; c++) {
                fprintf (fp, "%d%c", *imgPtr, d);
                imgPtr += img->nChannels;
            }
            fprintf (fp, "\n");
        }
        fprintf (fp, "\n\n");
    }
        
    fclose (fp);
    printf ("Dumped pixels to %s\n", file_name);
}

void mvDumpHistogram (IplImage* img, const char* file_name, char delimiter) {
    FILE* fp = fopen (file_name, "w");
    if (fp == NULL)
        return;    

    const int step_size = 5;
    const int nbins = 255/step_size;

    int size[] = {nbins};
    float range[] = {0, 255};
    float* ranges[] = {range};

    IplImage* channel1 = cvCreateImage (cvGetSize(img), IPL_DEPTH_8U, 1);
    IplImage* channel2 = cvCreateImage (cvGetSize(img), IPL_DEPTH_8U, 1);
    IplImage* channel3 = cvCreateImage (cvGetSize(img), IPL_DEPTH_8U, 1);
    
    // Split image onto the color planes.
    cvSplit(img, channel1, channel2, channel3, NULL);
    IplImage* planes[] = {channel1, channel2, channel3};

    // here we create 3 1-dimensional histograms. Each hist only looks at one of the 3 colors of RGB
    for (int i = 0; i < 3; i++) {
        CvHistogram* hist = cvCreateHist (1, size, CV_HIST_ARRAY, ranges, 1);
        IplImage* curr_plane[] = {planes[i]};
        cvCalcHist (curr_plane, hist, 0, NULL);
        cvNormalizeHist (hist, 10000);

        fprintf (fp, "\nChannel %d Histogram\nRange%cBin #%cCount\n", i+1,delimiter,delimiter);

        // print histogram
        for (int j = 0; j < nbins; j++) {
            int binval = cvQueryHistValue_1D(hist, j);
            fprintf (fp, "%d-%d%c%d%c%d\n", j*step_size,(j+1)*step_size,delimiter,j+1,delimiter,binval);
        }
    }

    cvReleaseImage (&channel1);
    cvReleaseImage (&channel2);
    cvReleaseImage (&channel3);
    fclose (fp);
    printf ("Dumped histogram to %s\n", file_name);
}

void mvHuMoments(IplImage *src, double *hus){
    CvMoments moments;

    cvMoments(src, &moments, true);
    cvGetHuMoments(&moments, (CvHuMoments*)hus); 
}

/// TERRIBLE TERRIBLE CODING
#define IN_SRC(x) (((x)>=src_addr_first) && ((x)<src_addr_last))
#define IN_DST(x) (((x)> dst_addr_first) && ((x)<dst_addr_last))

void mvBinaryDilate (IplImage* src, IplImage* dst, int kernel_point_array[], unsigned kernel_width, unsigned kernel_height, unsigned kernel_area);
void mvBinaryErode (IplImage* src, IplImage* dst, int kernel_point_array[], unsigned kernel_width, unsigned kernel_height, unsigned kernel_area);
void mvBinaryGradient (IplImage* src, IplImage* dst, int kernel_point_array[], unsigned kernel_width, unsigned kernel_height, unsigned kernel_area);

mvBinaryMorphology::mvBinaryMorphology (int Kernel_Width, int Kernel_Height, MV_KERNEL_SHAPE Shape) :
    bin_morph ("mvMorphology - morph"),
    bin_gradient ("mvMorphology - gradient")
{
    assert (Kernel_Width % 2 == 1); // odd number
    assert (Kernel_Height % 2 == 1);

    temp = mvCreateImage();
    kernel_width = Kernel_Width;
    kernel_height = Kernel_Height;   
    kernel_area = Kernel_Width * Kernel_Height;
    int widthStep = temp->widthStep;

    if (Shape == MV_KERN_RECT) {
        /// calculate an array of linear shifts for each point in the kernel
        kernel_point_array = new int[kernel_area];
        unsigned array_index = 0;
        for (int j = -(kernel_height-1)/2; j <= (kernel_height-1)/2; j++)
            for (int i = -(kernel_width-1)/2; i <= (kernel_width-1)/2; i++)
                kernel_point_array[array_index++] = i*widthStep + j;    
    }
    else if (Shape == MV_KERN_ELLIPSE) {
        /// calculate an elliptical kernel, then use the valid kernel points for the kernel point array
        int* temp_array = new int[kernel_area];
        unsigned array_index = 0;
        unsigned valid_count = 0;
        unsigned W2 = Kernel_Width*Kernel_Width/4;
        unsigned H2 = Kernel_Height*Kernel_Height/4;
        unsigned WH2 = W2*H2;

        for (int j = -(kernel_height-1)/2; j <= (kernel_height-1)/2; j++) {
            for (int i = -(kernel_width-1)/2; i <= (kernel_width-1)/2; i++) {
                if (i*i*H2 + j*j*W2 <= WH2) { // check if i,j are in the ellipse
                    temp_array[array_index++] = i*widthStep + j;
                    valid_count++;   
                }
                else {
                    temp_array[array_index++] = MV_UNDEFINED_VALUE;
                }                
            }
        }

        /// copy the valid points to kernel_point_array
        kernel_point_array = new int[valid_count];
        array_index = 0;
        for (unsigned i = 0; i < kernel_area; i++) {
            if (temp_array[i] != MV_UNDEFINED_VALUE) {
                kernel_point_array[array_index++] = temp_array[i];
            }
        }
        delete []temp_array;
        kernel_area = valid_count;
   
    }
    else {
        fprintf (stderr, "%s: %d: Unrecognized kernel shape.\n", __FILE__, __LINE__);
        exit (1);
    }
}

mvBinaryMorphology::~mvBinaryMorphology () {
	delete []kernel_point_array;
	cvReleaseImage(&temp);
}

void mvBinaryMorphology::mvBinaryMorphologyMain (
        MV_MORPHOLOGY_TYPE morphology_type,
        IplImage* src, IplImage* dst
)
{
    assert (src->width == temp->width);
    assert (src->height == temp->height);
    assert (src->width == dst->width);
    assert (src->height == dst->height);
    assert (src->nChannels == 1);
    assert (dst->nChannels == 1);
    
    cvZero (temp);

    if (morphology_type == MV_DILATE) {
        mvBinaryDilate (src, temp, kernel_point_array, kernel_width, kernel_height, kernel_area);
    }
    else if (morphology_type == MV_ERODE) {
        mvBinaryErode (src, temp, kernel_point_array, kernel_width, kernel_height, kernel_area);
    }
    else if (morphology_type == MV_CLOSE) {
        mvBinaryDilate (src, temp, kernel_point_array, kernel_width, kernel_height, kernel_area);
        cvZero (dst);
        mvBinaryErode (temp, dst, kernel_point_array, kernel_width, kernel_height, kernel_area);
    }
    else if (morphology_type == MV_OPEN) {
        mvBinaryErode (src, temp, kernel_point_array, kernel_width, kernel_height, kernel_area);
        cvZero (dst);
        mvBinaryDilate (temp, dst, kernel_point_array, kernel_width, kernel_height, kernel_area);
    }
    else if (morphology_type == MV_GRADIENT) {
        mvBinaryGradient (src, temp, kernel_point_array, kernel_width, kernel_height, kernel_area);
    }
    
    cvCopy (temp, dst);
}


//#########################################################################
//#### Helper functions for mvBinaryMorphology
//#########################################################################
void mvBinaryDilate (
    IplImage* src, IplImage* dst,
    int* kernel_point_array,
    unsigned kernel_width,
    unsigned kernel_height,
    unsigned kernel_area
)
{
    assert (src->imageData != dst->imageData);
    unsigned width = src->width;
    unsigned height = src->height;
    unsigned widthStep = src->widthStep;    // width in bytes
    unsigned char* dst_addr_first = (unsigned char*)dst->imageData;
    unsigned char* dst_addr_last = dst_addr_first + widthStep*height;

    unsigned char *srcPtr;
    unsigned char *dstPtr;

    /// go over every pixel in src. If that pixel is High, we set a
    /// kernel around it in dst to be High as well.
    for (unsigned r = 0; r < height; r++) {
        srcPtr = (unsigned char*) (src->imageData + r*widthStep);
        dstPtr = (unsigned char*) (dst->imageData + r*widthStep);
 
        for (unsigned c = 0; c < width; c++) {
            if (*srcPtr != 0) { // only work on non-zero (high) pixels

                unsigned start = (c < (kernel_width+1)/2) ? (kernel_area+1)/2 : 0;
                unsigned stop  = (c > width-(kernel_width+1)/2) ? (kernel_area+1)/2 : kernel_area;
                for (unsigned i = start; i < stop; i++) {
                    unsigned char *ptr = dstPtr + kernel_point_array[i];
                    if (IN_DST(ptr)) {
                        *ptr = *srcPtr;
                    }
                }

            }
            srcPtr++;
            dstPtr++;
        }
    }
}

void mvBinaryErode (
    IplImage* src, IplImage* dst,
    int* kernel_point_array,
    unsigned kernel_width,
    unsigned kernel_height,
    unsigned kernel_area
)
{
    assert (src->imageData != dst->imageData);
    unsigned width = src->width;
    unsigned height = src->height;
    unsigned widthStep = src->widthStep;    // width in bytes
    unsigned char* src_addr_first = (unsigned char*)src->imageData;
    unsigned char* src_addr_last = src_addr_first + widthStep*height;

    unsigned char *srcPtr;
    unsigned char *dstPtr;

    /// go over every pixel in src. If that pixel is High, we check a
    /// kernel around it for 0s. If a 0 is found set dst pixel to 0
    for (unsigned r = 0; r < height; r++) {
        srcPtr = (unsigned char*) (src->imageData + r*widthStep);
        dstPtr = (unsigned char*) (dst->imageData + r*widthStep);
 
        for (unsigned c = 0; c < width; c++) {
            if (*srcPtr != 0) { // only work on non-zero (high) pixels

                bool BREAK = false;
                for (unsigned i = 0; i < kernel_area; i++) {
                    unsigned char *ptr = srcPtr + kernel_point_array[i];
                    if (IN_SRC(ptr) && *ptr == 0) {
                        *dstPtr = 0;
                        BREAK = true;
                        break;
                    }
                }
                if (!BREAK)
                    *dstPtr = *srcPtr;

            }
            srcPtr++;
            dstPtr++;
        }
    }
}

void mvBinaryGradient (
    IplImage* src, IplImage* dst,
    int* kernel_point_array,
    unsigned kernel_width,
    unsigned kernel_height,
    unsigned kernel_area
)
{
    // first do dilate, then use the result + the src to do the gradient
    mvBinaryDilate (src, dst, kernel_point_array, kernel_width, kernel_height, kernel_area);

    assert (src != dst);
    unsigned width = src->width;
    unsigned height = src->height;
    unsigned widthStep = src->widthStep;    // width in bytes
    unsigned char* src_addr_first = (unsigned char*)src->imageData;
    unsigned char* src_addr_last = src_addr_first + widthStep*height;

    unsigned char *srcPtr;
    unsigned char *dstPtr;

    // go over each high pixel in dst (dilated img). If the kernel around the 
    // equivalent src pixel is zero or will be eroded to zero, leave it alone.
    // Else set it to 0, because the src pixel is 1 and Grad = Dst - Src
    for (unsigned r = 0; r < height; r++) {
        srcPtr = (unsigned char*) (src->imageData + r*widthStep);
        dstPtr = (unsigned char*) (dst->imageData + r*widthStep);
 
        for (unsigned c = 0; c < width; c++) {
            if (dstPtr != 0) {
                // this hack makes all pixels around the left/right edges zero.
                // Otherwise artifacts are left on the edges.
                if (2*c < kernel_width || 2*(width-1-c) < kernel_width) {
                    *dstPtr = 0;
                }
                // here we check if *srcPtr will be eroded to 0
                else if (*srcPtr != 0) {
                    bool BREAK = false;
                    for (unsigned i = 0; i < kernel_area; i++) {
                        unsigned char *ptr = srcPtr + kernel_point_array[i];
                        if (IN_SRC(ptr) && *ptr == 0) { // *srcPtr eroded to 0
                            BREAK = true;
                            break;
                        }
                    }
                    if (!BREAK)
                        *dstPtr = 0;
                }
            }
            srcPtr++;
            dstPtr++;
        }
    }
}

//#########################################################################
//#### Functions for BGR2HSV. Not used.
//#########################################################################
typedef unsigned char uchar;
// Takes in 3 numbers B,R,G, and modifies them to instead be in H,S,V format
void tripletBGR2HSV (int Blue, int Green, int Red, int &Hue, int &Sat, int &Val) {
    uchar uH, uS, uV;
    tripletBGR2HSV (static_cast<uchar>(Blue), static_cast<uchar>(Green), static_cast<uchar>(Red), uH,uS,uV);
    Hue = static_cast<int>(uH);
    Sat = static_cast<int>(uS);
    Val = static_cast<int>(uV); 
}
void tripletBGR2HSV (uchar Blue, uchar Green, uchar Red, uchar &Hue, uchar &Sat, uchar &Val) {
    /// find the max and min color component
    uchar M, m, Chroma;
    if (Blue > Red) {
        M = std::max(Blue, Green);
        m = std::min(Red, Green);
    }
    else {
        M = std::max(Red, Green);
        m = std::min(Blue, Green);
    }
    Chroma = M - m;

    /// calculate HSV components
    if (Chroma == 0) {
        Hue = Sat = 0;
        Val = M;
        return;
    }

    if (M == Red) {
        Hue = 30 * (Green - Blue) / Chroma;
        if (Hue > 180) Hue += 180;
    } else if (M == Green) {
        Hue = 60 + 30 * (Blue - Red) / Chroma;
    } else {
        Hue = 120 + 30 * (Red - Green) / Chroma;
    }

    Val = M;
    Sat = 255 *Chroma / Val;
}

void mvBGR2HSV(IplImage* src, IplImage* dst) {
    
    assert (src != NULL);
    assert (dst != NULL);
    assert (src->nChannels == 3);
    assert (dst->nChannels == 3);
    
    int i, j;
    unsigned char* srcPtr, * dstPtr;

    for ( i = 0; i < src->height; i++) {
        srcPtr = (unsigned char *)((src->imageData) + i*(src->widthStep));
        dstPtr = (unsigned char *)((dst->imageData) + i*(dst->widthStep));
        for ( j = 0; j < src->width; j++) {
    	    tripletBGR2HSV(*srcPtr, *(srcPtr+1), *(srcPtr+2), *dstPtr, *(dstPtr+1), *(dstPtr+2));
    	    srcPtr +=3;
    	    dstPtr +=3;  
	    }
    }
  return;
}
