#ifndef __MDA_VISION_MVCOLORFILTER__
#define __MDA_VISION_MVCOLORFILTER__

#include <cv.h>
#include "mv.h"
#include "mgui.h"

// ##################################################################################################
//  Utility Functions and Filters that are not classes
// ##################################################################################################

inline bool hue_in_range (unsigned char hue, int HMIN, int HMAX) { // helper function for the filter
    if (HMAX >= HMIN) 
        return (hue >= HMIN && hue <= HMAX);
    else
        return ((hue <= HMIN && hue <= HMAX) || (hue >= HMIN && hue >= HMAX)); 
}

void ManhattanDistanceFilter(IplImage* src, IplImage* dst);

// ##################################################################################################
//  HSVFilter - very basic filter that looks at each pixel and sees if it falls within user defined 
//              HSV bounds
// ##################################################################################################

/** HSV color limit filter */
// Currently support for changing the HSV values on the fly are limited
class mvHSVFilter {
    unsigned IMG_WIDTH, IMG_HEIGHT;
    int HMIN,HMAX;
    unsigned SMIN,SMAX, VMIN, VMAX;
    
    IplImage* scratch_3;

    PROFILE_BIN bin_WorkingLoop;
    PROFILE_BIN bin_CvtColor;
    
    void filter_internal (IplImage* HSV_img, IplImage* result);

    public:
    mvHSVFilter (const char* settings_file);
    ~mvHSVFilter ();
    
    void filter (IplImage* img, IplImage* result) {
        bin_CvtColor.start();
        cvCvtColor (img, scratch_3, CV_BGR2HSV); // convert to HSV 
        bin_CvtColor.stop();

        filter_internal (scratch_3, result);
    }

    void filter_non_common_size (IplImage* img, IplImage* result) {
        IplImage * temp_scratch; // scratch_3 is common size. temp_scratch will be same size as img
        temp_scratch = mvCreateImage_Color (img);

        bin_CvtColor.start();
        cvCvtColor (img, temp_scratch, CV_BGR2HSV); // convert to HSV 
        bin_CvtColor.stop();

        filter_internal (temp_scratch, result);

        cvReleaseImage (&temp_scratch);
    }   
};

// ##################################################################################################
//  mvHistogramFilter - uses histogram bins to determine best H,S bounds
// ##################################################################################################
class mvHistogramFilter {
    static const int nbins_hue = 30;
    static const int nbins_sat = 25;
    static const int hue_range_min = 0;
    static const int hue_range_max = 179;
    static const int sat_range_min = 6;
    static const int sat_range_max = 255;
    static const int MAX_BINS_MARKED = 9;
    static const int HISTOGRAM_NORM_FACTOR = 100000;
    static const bool DISPLAY_HIST = true;
    static const int NUM_SIDES_RECTANGLE = 4;


    // a "Quad" is a collection of histogram bins in 2D. So a quad that goes from Hue=[0,20] Sat=[50,80]
    // will contain all histogram bins with those H,S values
    struct Quad{
        int h0, s0, h1, s1;
    };

    int hue_min;
    int hue_max;
    int sat_min;
    int sat_max;
    int val_min;
    int val_max;

    IplImage* HSV_img; // stores the image but converted to HSV
    IplImage* hue_img; 
    IplImage* sat_img;
    IplImage* hist_img; // used to display the histogram if needed

    CvHistogram* hist; 

    PROFILE_BIN bin_Hist;
    PROFILE_BIN bin_CvtColor;
    mvWindow* win;

    private:
    void setQuad (Quad &Q, int h0, int s0, int h1, int s1); // set boundaries on a quad
    int getQuadAvgCount (Quad Q);                              // get average bin count
    void getRectangleNeighbours(Quad rect, Quad sides[]);

    public:
    mvHistogramFilter (const char* settings_file);
    ~mvHistogramFilter ();
    void filter (IplImage* src, IplImage* dst);
    void print_histogram ();
    void show_histogram ();
};

// ##################################################################################################
//  Hue_Box - helper class for mvAdvancedColorFilter
// ##################################################################################################
class Hue_Box {
public:
    unsigned char HUE_MIN;
    unsigned char HUE_MAX;
    unsigned char SAT_MIN;
    unsigned char SAT_MAX;
    unsigned char VAL_MIN;
    unsigned char VAL_MAX;
    int BOX_NUMBER;
    int BOX_COLOR;
    bool BOX_ENABLED; // whether the box is being used or not

    static const int HUE_GUTTER_LEN = 6;
    static const int HUE_ADP_LEN = 2;
    static const float RESET_THRESHOLD_FRACTION = 0.0005;
    unsigned char HUE_MIN_OUT, HUE_MAX_OUT;     // outer hue limits for expanding the box
    unsigned char HUE_MIN_IN, HUE_MAX_IN;       // inner hue limits for contracting the box
    unsigned char HUE_MIN_ADP, HUE_MAX_ADP;     // normal hue limits
    int inner_count;                            // num of pixels within the inner box
    int min_inside_count, max_inside_count;     // num of pixels within the normal box
    int min_outside_count, max_outside_count;   // num of pixels within the outside box

    Hue_Box (const char* settings_file, int box_number);

    bool check_hsv (unsigned char hue, unsigned char sat, unsigned char val) {
        // shifting logic goes here
        if (sat >= SAT_MIN && sat <= SAT_MAX && val >= VAL_MIN && val <= VAL_MAX) {
            if (HUE_MAX >= HUE_MIN) 
                return (hue >= HUE_MIN && hue <= HUE_MAX);
            else
                return ((hue <= HUE_MIN && hue <= HUE_MAX) || (hue >= HUE_MIN && hue >= HUE_MAX)); 
        }
        return false;
    }

    bool check_hsv_adaptive_hue (unsigned char hue, unsigned char sat, unsigned char val);
    void update_hue ();

    bool is_enabled () {
        return BOX_ENABLED;
    }
};

// ##################################################################################################
//  COLOR_TRIPLE - helper class for mvAdvancedColorFilter
// ##################################################################################################
class COLOR_TRIPLE {
public:
    int m1, m2, m3;
    unsigned n_pixels;
    unsigned index_number;

    static const int NUM_HISTOGRAM_BINS = 5;
    
    COLOR_TRIPLE(){
        m1 = m2 = m3 = n_pixels = index_number = 0;
    }
    COLOR_TRIPLE(unsigned M1, unsigned M2, unsigned M3, unsigned Index_Number){
        m1 = M1; 
        m2 = M2;
        m3 = M3;
        n_pixels = 1;
        index_number = Index_Number;
    }
    void calc_average () {
        if (n_pixels > 0) {
            m1 /= n_pixels;
            m2 /= n_pixels;
            m3 /= n_pixels;
        }
        else {
            m1 = m2 = m3 = 0;
        }
    }
    void BGR_to_HSV () {
        unsigned char H,S,V;
        tripletBGR2HSV (static_cast<unsigned char>(m1),static_cast<unsigned char>(m2),static_cast<unsigned char>(m3)
                        ,H,S,V); // m1,m2,m3 are actually in BGR
        m1 = static_cast<int>(H);
        m2 = static_cast<int>(S);
        m3 = static_cast<int>(V);
    }
    void merge (COLOR_TRIPLE B) {
        unsigned total = n_pixels + B.n_pixels;
        m1 = (m1*n_pixels + B.m1*B.n_pixels) / total;
        m2 = (m2*n_pixels + B.m2*B.n_pixels) / total;
        m3 = (m3*n_pixels + B.m3*B.n_pixels) / total;
        n_pixels = total;
    }
    void add_pixel (int p1, int p2, int p3) {
        m1 += p1;
        m2 += p2;
        m3 += p3;
        n_pixels++;
    }
    int diff (COLOR_TRIPLE T) {
        return (abs(m1-T.m1) + abs(m2-T.m2) + abs(m3-T.m3));
    }
    int sqr_diff (COLOR_TRIPLE T) {
        int d1 = m1 - T.m1;
        int d2 = m2 - T.m2;
        int d3 = m3 - T.m3;
        return (d1*d1 + d2*d2 + d3*d3);
    }
    void print () {
        printf ("color_triplet #%2d (%d pixels): %d %d %d\n", index_number, n_pixels, m1, m2, m3);
    }
};

class COLOR_TRIPLE_FLOAT {
public:
    double mf1, mf2, mf3;
    unsigned n_pixels;

    COLOR_TRIPLE_FLOAT () {
        mf1 = mf2 = mf3 = n_pixels = 0;
    }
    void add_pixel (int p1, int p2, int p3) {
        mf1 += static_cast<double>(p1);
        mf2 += static_cast<double>(p2);
        mf3 += static_cast<double>(p3);
        n_pixels++;
    }
    void calc_average () {
        if (n_pixels == 0) return;
        mf1 /= static_cast<double>(n_pixels);
        mf2 /= static_cast<double>(n_pixels);
        mf3 /= static_cast<double>(n_pixels);
    }
    void print () {
        printf ("color_triplet_float (%d pixels): %6.2lf %6.2lf %6.2lf\n", n_pixels, mf1, mf2, mf3);    
    }
    void print (const char* name) {
        printf ("%s (%d pixels): %6.2lf %6.2lf %6.2lf\n", name, n_pixels, mf1, mf2, mf3);        
    }
};

typedef std::vector<COLOR_TRIPLE> COLOR_TRIPLE_VECTOR;  

void mvGetBoundsFromGaussian (
    COLOR_TRIPLE_FLOAT mean, COLOR_TRIPLE_FLOAT variance, COLOR_TRIPLE_FLOAT skew, 
    COLOR_TRIPLE &upper, COLOR_TRIPLE &lower
);

// ##################################################################################################
//  mvAdvancedColorFilter - mean_shift, flood_fill, and other more complicated algorithms
// ##################################################################################################
class mvAdvancedColorFilter {
    //declare constants here
    static const int DS_FACTOR = 1; // downsampling
    static const int GOOD_PIXELS_FACTOR = 6;
    static const int KERNEL_SIZE = 7;
    static const int S_MIN = 60;
    static const int V_MIN = 30;
    static const unsigned char BAD_PIXEL = 253;
    static const unsigned char TEMP_PIXEL = 251;

    static const bool KERNEL_SHAPE_IS_RECT = false;

public:
    static const int NUM_BOXES = 3;

private:
    // parameters read from settings file
    int COLOR_DIST;
    int H_DIST;
    int S_DIST;
    int V_DIST;

    // calculated parameters
    int KERNEL_AREA;
    int KERNEL_RAD;

    // internal data
    Hue_Box* hue_box[NUM_BOXES]; // array of pointers to boxes
    COLOR_TRIPLE_VECTOR color_triple_vector;
    int* kernel_point_array;
    IplImage* ds_scratch_3;   // downsampled scratch image 3 channel
    IplImage* ds_scratch;   // 1 channel

    // variable and functions for flood_image algorithm and flood_image_interactive_callback
    std::vector<COLOR_TRIPLE_VECTOR> Training_Matrix;
    int Current_Interactive_Color;
    static const int NUM_INTERACTIVE_COLORS = 2;
    bool FLAG_DO_COLOR_ADJUSTMENT;

    void meanshift_internal(IplImage* scratch);
    void flood_image_internal ();
    bool flood_from_pixel(int r, int c, unsigned index_number);
    void perform_color_adjustment_internal ();

    // profile bins
    PROFILE_BIN bin_Resize;
    PROFILE_BIN bin_Filter;
    
    void downsample_from(IplImage* src) {    // downsamples src to internal scratch image
        assert (src->nChannels == 3);
        bin_Resize.start();
          cvResize (src, ds_scratch_3, CV_INTER_NN);
        bin_Resize.stop();
    }
    void upsample_to_3 (IplImage* dst) {     // upsamples internal scrach to dst
        assert (dst->nChannels == 3);
        bin_Resize.start();
          cvResize (ds_scratch_3, dst, CV_INTER_NN);
        bin_Resize.stop();
    }
    void upsample_to (IplImage* dst) {     // upsamples internal scrach to dst
        assert (dst->nChannels == 1);
        bin_Resize.start();
          cvResize (ds_scratch, dst, CV_INTER_NN);
        bin_Resize.stop();
    }

    bool check_and_accumulate_pixel (unsigned char* pixel, unsigned char* ref_pixel,
                                    COLOR_TRIPLE &triple);
    bool check_and_accumulate_pixel_BGR (unsigned char* pixel, unsigned char* ref_pixel,
                                    COLOR_TRIPLE &triple);
    bool check_and_accumulate_pixel_HSV (unsigned char* pixel, unsigned char* ref_pixel,
                                    COLOR_TRIPLE &triple);
    
    void colorfilter_internal();
    void colorfilter_internal_adaptive_hue();

public:
    mvAdvancedColorFilter (const char* settings_file); //constructor
    ~mvAdvancedColorFilter(); // destructor
    void mean_shift(IplImage* src, IplImage* dst);
    void flood_image(IplImage* src, IplImage* dst, bool interactive=false);
    void filter(IplImage *src, IplImage* dst);
    void combined_filter(IplImage *src, IplImage* dst);

    friend void flood_image_interactive_callback(int event, int x, int y, int flags, void* param);
};

void flood_image_interactive_callback(int event, int x, int y, int flags, void* param);

// ##################################################################################################
//  mvWatershedFilter
// ##################################################################################################
class mvWatershedFilter {
    static const int WATERSHED_DS_FACTOR = 3;
    static const unsigned MAX_INDEX_NUMBER = 250;
    static const int MAX_MARKERS_TO_GENERATE = 100;
    static const int KERNEL_WIDTH = 3;
    static const int KERNEL_HEIGHT = 3;
    
    IplImage* scratch_image_3c;
    IplImage* scratch_image;
    IplImage* ds_image_3c;
    IplImage* ds_image_nonedge;
    IplImage* marker_img_32s;
    IplConvKernel* kernel;

    typedef std::pair<COLOR_TRIPLE, CvPoint> COLOR_POINT;
    typedef std::vector<COLOR_POINT> COLOR_POINT_VECTOR;
    COLOR_POINT_VECTOR color_point_vector;

    std::map<unsigned char,COLOR_TRIPLE> segment_color_hash;
    std::map<unsigned char,COLOR_TRIPLE>::iterator curr_segment_iter;
    
    unsigned curr_segment_index;
    unsigned final_index_number;

    PROFILE_BIN bin_SeedGen;
    PROFILE_BIN bin_SeedPlace;
    PROFILE_BIN bin_Filter;

    // generate markers from image, place them into color_point_vector
    void watershed_generate_markers_internal (IplImage* src, int method=0, std::vector<CvPoint>* seed_vector=NULL);
    // assign index number to markers - similar colored markers get similar indices
    void watershed_process_markers_internal ();
    void watershed_process_markers_internal2 ();
    // draw markers onto marker_img_32s and ready the segment_color_hash
    void watershed_place_markers_internal (IplImage* src);
    // calls cvWatershed
    void watershed_filter_internal (IplImage* src, IplImage* dst); // run watershed

public:
    mvWatershedFilter ();
    mvWatershedFilter (const char* settings_file); //constructor
    ~mvWatershedFilter(); // destructor
    void watershed(IplImage* src, IplImage* dst, int method=0);
    bool get_next_watershed_segment (IplImage* binary_img, COLOR_TRIPLE &T);    
};

#endif
