#include "mvColorFilter.h"
#include <math.h>

#define DISPLAY_HIST 1
#define HIST_PIXEL_SCALE 10

#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

//####################################################################################
//####################################################################################
//####################################################################################

mvHistogramFilter::mvHistogramFilter (const char* settings_file) :
    bin_Hist ("Adaptive3 - Logic"),
    bin_CvtColor ("Adaptive3 - CvtColor")
{
    read_mv_setting (settings_file, "HUE_MIN", hue_min);
    read_mv_setting (settings_file, "HUE_MAX", hue_max);
    read_mv_setting (settings_file, "SAT_MIN", sat_min);
    read_mv_setting (settings_file, "SAT_MAX", sat_max);
    read_mv_setting (settings_file, "VAL_MIN", val_min);
    read_mv_setting (settings_file, "VAL_MAX", val_max);

    HSV_img = mvGetScratchImage_Color();
    hue_img = mvCreateImageHeader();
    sat_img = mvCreateImageHeader();

    if (DISPLAY_HIST) {
        hist_img = cvCreateImage(
            cvSize(nbins_hue*HIST_PIXEL_SCALE, nbins_sat*HIST_PIXEL_SCALE),
            IPL_DEPTH_8U,
            1
        );
    }
    else
        hist_img = NULL;

    win = new mvWindow ("Adaptive Filter 3");

    int size[] = {nbins_hue, nbins_sat};
    float hue_range[] = {hue_range_min, hue_range_max};
    float sat_range[] = {sat_range_min, sat_range_max};
    float* ranges[] = {hue_range, sat_range};

    hist = cvCreateHist (
        2,              //dims
        size,           // num of bins per dims
        CV_HIST_ARRAY,  // dense matrix
        ranges,         // upper & lower bound for bins
        1               // uniform
    );
}

mvHistogramFilter::~mvHistogramFilter () {
    delete win;
    cvReleaseHist (&hist);
    mvReleaseScratchImage_Color();
    cvReleaseImageHeader(&hue_img);
    cvReleaseImageHeader(&sat_img);
    if (DISPLAY_HIST)
        cvReleaseImage(&hist_img);
}

void mvHistogramFilter::setQuad (Quad &Q, int h0, int s0, int h1, int s1) {
    Q.h0 = h0; Q.s0 = s0;
    Q.h1 = h1; Q.s1 = s1;
}

int mvHistogramFilter::getQuadAvgCount (Quad Q) {
    if (Q.s0 == -1)
        return -1;

    int count = 0;
    int num_bins = 0;
    for (int h = Q.h0; ; h++) {
        if (h >= nbins_hue) h = 0;  // allows looping of hue from bin16 to bin2, ect
        for (int s = Q.s0; s <= Q.s1; s++) {
            count += cvQueryHistValue_2D (hist, h, s);
            num_bins++;   
        }
        if(h == Q.h1) break;
    }
    return count/num_bins;
}

void mvHistogramFilter::filter (IplImage* src, IplImage* dst) {
    assert (src != NULL);
    assert (dst != NULL);
    assert (src->nChannels == 3);
    assert (dst->nChannels == 1);
    assert (src->width == HSV_img->width);
    assert (src->height == HSV_img->height);

    /// convert imageto HSV  
    bin_CvtColor.start();
    cvCvtColor (src, HSV_img, CV_BGR2HSV); // convert to Hue,Saturation,Value 
    bin_CvtColor.stop();

    /// Mark all invalid (Value not within limits) pixels as Hue=255 and Sat = 0 which is outside histogram range
    bin_Hist.start();
    unsigned char *imgPtr;
    for (int i = 0; i < HSV_img->height; i++) {
        imgPtr = (unsigned char*) HSV_img->imageData + i*HSV_img->widthStep;
        for (int j = 0; j < HSV_img->width; j++) {
            if ((*(imgPtr+2) < val_min) || (*(imgPtr+2) > val_max)) {
                *imgPtr = 255;
                *(imgPtr+1) = 0;
            }
            imgPtr+=3;
        }
    }

    /// in place split of the image
    mvSplitImage (HSV_img, &hue_img, &sat_img);

    /// genereate the hue-sat histogram and normalize it 
    IplImage* planes[] = {hue_img, sat_img};
    cvCalcHist (
        planes, hist,
        0,      // accumulate
        NULL    // possible boolean mask image
    ); 

    cvNormalizeHist (hist, HISTOGRAM_NORM_FACTOR);
    
    #ifdef M_DEBUG
        print_histogram();
        show_histogram();
    #endif

    DEBUG_PRINT ("\nBeginning Histogram Filter Algorithm\n");

    /// Attempt to find a local maximum within the user-defined hue-sat bounds
    int bin_min_index_hue = hue_min*nbins_hue / (hue_range_max - hue_range_min);
    int bin_max_index_hue = hue_max*nbins_hue / (hue_range_max - hue_range_min) - 1;
    int bin_min_index_sat = sat_min*nbins_sat / (sat_range_max - sat_range_min);
    int bin_max_index_sat = sat_max*nbins_sat / (sat_range_max - sat_range_min) - 1;
    DEBUG_PRINT ("Bins Searched: Hue (%d-%d)  Sat (%d-%d)\n", bin_min_index_hue, bin_max_index_hue, bin_min_index_sat, bin_max_index_sat);
    
    unsigned local_max_bin_index[2] = {0,0};
    unsigned local_max_bin_value = 0;
    for (int h = bin_min_index_hue; ; h++) {
        if (h >= nbins_hue) h = 0;  // allows looping of hue from bin16 to bin2, ect

        for (int s = bin_min_index_sat; s <= bin_max_index_sat; s++) {
            unsigned bin_value = cvQueryHistValue_2D (hist, h, s);
            if (bin_value > local_max_bin_value) {
                local_max_bin_value = bin_value;
                local_max_bin_index[0] = h;
                local_max_bin_index[1] = s;
            }
        }

        if(h == bin_max_index_hue) break;
    }

    if (local_max_bin_value == 0) { // no non-zero bins in range
        printf ("All bins in search range are zero!\n");
        bin_Hist.stop();
        return;
    }

    DEBUG_PRINT ("Local Max at bin (%d, %d)\n", local_max_bin_index[0], local_max_bin_index[1]);

    /// now define rect, which is the rectangle in BinIndex space representing the accepted H-S window
    Quad rect;
    setQuad (rect, local_max_bin_index[0], local_max_bin_index[1], local_max_bin_index[0], local_max_bin_index[1]);
    Quad sides[NUM_SIDES_RECTANGLE];

    DEBUG_PRINT ("Accumulating Bins\n");
    bool successful = false;

    for(int i = 0; i < 4; i++){
        DEBUG_PRINT ("  Iteration %d\n", i);
        
        /// Obtain the BinIndex space representation of the 4 sides of the rect
        getRectangleNeighbours (rect, sides);
        int rect_val = getQuadAvgCount(rect);

        /// find the avg value of the squares on the sides.
        int side_val[NUM_SIDES_RECTANGLE];
        int side_val_avg = 0;
        int num_valid_sides = 0;
        for (int j = 0; j < NUM_SIDES_RECTANGLE; j++) {
            side_val[j] = getQuadAvgCount(sides[j]);
            if (side_val[j] >= 0) {
                num_valid_sides++;
                side_val_avg += side_val[j];
            }
        }

        side_val_avg /= num_valid_sides;
   
        DEBUG_PRINT ("    rect: (%d,%d,%d,%d) - %d\n", rect.h0, rect.s0, rect.h1, rect.s1, rect_val);
        for (int j = 0; j < NUM_SIDES_RECTANGLE; j++){
            DEBUG_PRINT("    side: (%d,%d,%d,%d) - %d\n", sides[j].h0, sides[j].s0, sides[j].h1, sides[j].s1, side_val[j]);
        }        
        DEBUG_PRINT ("    side_avg: %d\n", side_val_avg);

        /// If the avg value of the sides >> value inside the rect, thenthe algorithm is done
        if (rect_val > 8*side_val_avg) {
            DEBUG_PRINT ("  Break on iteration %d\n", i);
            successful = true;
            break;
        }

        /// Otherwise add the best side to the rectangle
        int best_side = -1;
        int best_side_val = -1;
        for (int j = 0; j < NUM_SIDES_RECTANGLE; j++) {
            if (side_val[j] >= 0 && side_val[j] > best_side_val) {
                best_side = j;
                best_side_val = side_val[j];
            }
        }
        if (best_side == 0 || best_side == 3) {
            rect.h0 = sides[best_side].h0; 
            rect.s0 = sides[best_side].s0;
        }
        else {
            rect.h1 = sides[best_side].h1;
            rect.s1 = sides[best_side].s1;
        }
    }

    /// Calculate the H-S space coordinates of the allowed window using the BinIndex values
    unsigned hue_min_hist = (unsigned)rect.h0 * hue_range_max / nbins_hue;
    unsigned hue_max_hist = (unsigned)(rect.h1+1) * hue_range_max / nbins_hue;
    unsigned sat_min_hist = (unsigned)rect.s0 * sat_range_max / nbins_sat;
    unsigned sat_max_hist = (unsigned)(rect.s1+1) * sat_range_max / nbins_sat;
    DEBUG_PRINT ("Hue Range: %d-%d\n", hue_min_hist, hue_max_hist);
    DEBUG_PRINT ("Sat Range: %d-%d\n", sat_min_hist, sat_max_hist);

    /// if successful, generate dst image
    unsigned count = 0;
    if (successful) {
        unsigned char *dstPtr, *huePtr, *satPtr;
        for (int i = 0; i < dst->height; i++) {
            dstPtr = (unsigned char*) dst->imageData + i*dst->widthStep;
            huePtr = (unsigned char*) hue_img->imageData + i*hue_img->widthStep;
            satPtr = (unsigned char*) sat_img->imageData + i*sat_img->widthStep;

            for (int j = 0; j < dst->width; j++) {
                if ((*huePtr != 255) && hue_in_range (*huePtr, hue_min_hist, hue_max_hist) && 
                    (*satPtr >= sat_min_hist && *satPtr <= sat_max_hist))
                {
                    *dstPtr = 255;
                    count++;
                }
                else {                    
                    *dstPtr = 0;
                }
                
                dstPtr++; 
                huePtr++;
                satPtr++;
            }
        }
    }
    else {
        cvZero (dst);
    }

    DEBUG_PRINT ("Allowed %d pixels\n", count);
    bin_Hist.stop();
}

void mvHistogramFilter::print_histogram () {
    int hist_height = hist->mat.dim[0].size;
    int hist_width = hist->mat.dim[1].size;
    printf ("\nprint_histogram():\n");

    for (int i = 0; i < hist_height; i++) {
        for (int j = 0; j < hist_width; j++) {
            int binval = cvQueryHistValue_2D(hist, i,j);
            unsigned hue_min = (unsigned)i * hue_range_max / nbins_hue;
            unsigned hue_max = (unsigned)(i+1) * hue_range_max / nbins_hue;
            unsigned sat_min = (unsigned)j * sat_range_max / nbins_sat;
            unsigned sat_max = (unsigned)(j+1) * sat_range_max / nbins_sat;
        
            if (binval > 0)
                printf ("Bin(%2d,%2d) HS(%d-%d,%d-%d) count=%d\n",i,j, hue_min,hue_max,sat_min,sat_max, binval);
        }
    }
}

void mvHistogramFilter::show_histogram () {
    cvZero (hist_img);
    
    float max;
    cvGetMinMaxHistValue (hist, 0, &max, 0, 0);

    int hist_height = hist->mat.dim[0].size;
    int hist_width = hist->mat.dim[1].size;

    for (int i = 0; i < hist_height; i++) {
        for (int j = 0; j < hist_width; j++) {
            float binval = cvQueryHistValue_2D(hist, i,j);
            int intensity = cvRound (sqrt(binval/max) * 255);

            cvRectangle (
                hist_img,
                cvPoint (j*HIST_PIXEL_SCALE, i*HIST_PIXEL_SCALE), // x coord first
                cvPoint ((j+1)*HIST_PIXEL_SCALE, (i+1)*HIST_PIXEL_SCALE),
                CV_RGB(intensity,intensity,intensity),
                CV_FILLED
            );
        }
    }

    win->showImage(hist_img);
}

void mvHistogramFilter::getRectangleNeighbours(Quad rect, Quad sides[]){
    int h0 = rect.h0;
    int s0 = rect.s0;
    int h1 = rect.h1;
    int s1 = rect.s1;

    setQuad(sides[0], (h0>0) ? h0-1: nbins_hue-1, s0, (h0>0) ? h0-1: nbins_hue-1, s1);
    setQuad(sides[1], h0, (s1<nbins_sat-1) ? s1+1 : -1, h1, (s1<nbins_sat-1) ? s1+1: -1);
    setQuad(sides[2], (h1<nbins_hue-1) ? h1+1: 0, s0, (h1<nbins_hue-1) ? h1+1: 0, s1);
    setQuad(sides[3], h0, (s0>0)? s0-1: -1, h1, (s0>0)? s0-1: -1);
}
