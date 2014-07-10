#ifndef __MDA_VISION_MVCONTOURS__
#define __MDA_VISION_MVCONTOURS__

#include <cv.h>
#include "mv.h"
#include "mgui.h"
#include "mvColorFilter.h"

typedef std::vector<double> HU_MOMENTS;
static const int CONTOURS_MATCH_NORMAL = 0;
static const int CONTOURS_MATCH_RECIP = 1;
static const int CONTOURS_MATCH_FRACTION = 2;

// ##########################################################################
// Parameters and Utility Functions
// ##########################################################################
typedef struct _RECTANGLE_PARAMS {
    int contour_points_min;
    int contour_area_min;
    int contour_area_max;
    float lw_ratio_min;
    float lw_ratio_max;
    float area_ratio_min;
    float peri_ratio_min;
    float peri_ratio_max;
} RECTANGLE_PARAMS;

RECTANGLE_PARAMS read_rectangle_settings(const char filename[]);

typedef struct _CIRCLE_PARAMS {
    int contour_points_min;
    int contour_area_min;
    int contour_area_max;
    float ratio03_min;
    float ratio02_min;
    float ratio12_min;
    float ratio11_max;
    float ratioR_max;
    float radius_min;
    float radius_max;
    float area_ratio_min;
} CIRCLE_PARAMS;

CIRCLE_PARAMS read_circle_settings(const char filename[]);

  // ##########################################################################
// mvContours
// ##########################################################################
class mvContours {
    int DEBUG_LEVEL;

    // list of images that contain objects to match against
    static const int NUM_CONTOUR_RECT_IMAGES = 5;
    static const char* contour_rect_images[];
    std::vector<HU_MOMENTS> hu_moments_rect_vector;

    static const int NUM_CONTOUR_CIRC_IMAGES = 2;
    static const char* contour_circ_images[];
    std::vector<HU_MOMENTS> hu_moments_circ_vector;

    CvMemStorage* m_storage;
    CvSeq* m_contours;

    PROFILE_BIN bin_contours;
    PROFILE_BIN bin_match;
    PROFILE_BIN bin_calc;

private:
    // approximates a rounded contour with a polygon
    CvSeq* approx_poly (CvSeq* contour_to_approx, int accuracy) { 
        return cvApproxPoly(
                contour_to_approx,
                sizeof(CvContour),
                m_storage,
                CV_POLY_APPROX_DP,
                accuracy,               // accuracy in pixels
                0                       // contour level. 0 = first only, 1 = all contours level with and below the first
            ); 
    }

    int find_contour_and_check_errors (IplImage* img);
    void get_rect_parameters (IplImage* img, CvSeq* contour1, CvPoint &centroid, float &length, float &angle);
    void get_circle_parameters (IplImage* img, CvSeq* contour1, CvPoint &centroid, float &radius);

    void draw_contours (CvSeq* contours_to_draw, IplImage* img, int level=0) {
        cvDrawContours (
                    img,
                    contours_to_draw,
                    cvScalar(200,200,200),  // contour color
                    cvScalar(200,200,200),  // background color
                    level                   // max contours level
            );
    }

public:
    mvContours (const int _debug_level=0);
    ~mvContours ();

    void set_debug_level(int _debug_level) { DEBUG_LEVEL = _debug_level; }
    int match_rectangle (IplImage* img, MvRBoxVector* rbox_vector, COLOR_TRIPLE color, const RECTANGLE_PARAMS &params);
    int match_circle (IplImage* img, MvCircleVector* circle_vector, COLOR_TRIPLE color, const CIRCLE_PARAMS &params);
    int match_ellipse (IplImage* img, MvRBoxVector* circle_vector, COLOR_TRIPLE color, float min_lw_ratio=1, float max_lw_ratio=100, int method=0);

    void drawOntoImage (IplImage* img) { draw_contours (m_contours, img, 1); }        

};

int assign_color_to_shape (int m1, int m2, int m3, MvShape* shape);
int assign_color_to_shape (COLOR_TRIPLE t, MvShape* shape);

#endif
