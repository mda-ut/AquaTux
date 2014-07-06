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
    static const int WATERSHED_STEP_SMALL = 1;
    static const int WATERSHED_SAMPLE_RANDOM = 2;

    mvWatershedFilter ();
    mvWatershedFilter (const char* settings_file); //constructor
    ~mvWatershedFilter(); // destructor
    void watershed(IplImage* src, IplImage* dst, int method=0);
    int num_watershed_segments () { return segment_color_hash.size(); }
    bool get_next_watershed_segment (IplImage* binary_img, COLOR_TRIPLE &T);    
};

#endif
