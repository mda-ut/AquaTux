/** mvShapes.cpp  */

#include "mvShapes.h"
#include "mgui.h"
#include "mv.h"
#include "math.h"
#include <cv.h>

//#define FLAG_DEBUG
#ifdef FLAG_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

// ######################################################################################
//  mvShape base class
// ######################################################################################

mvShape::mvShape() {
    DOWNSAMPLING_FACTOR = 1; // default constructor - use no ds
    ds_image = mvGetScratchImage(); // can use scratch image since we are not doing ds
    
    if (DEBUG_SHAPE)
        window = new mvWindow("mvShape Debug");
    else
        window = NULL;
}

mvShape::mvShape(const char* settings_file) {
    int width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);

    read_mv_setting (settings_file, "DOWNSAMPLING_FACTOR", DOWNSAMPLING_FACTOR);
    assert (DOWNSAMPLING_FACTOR >= 1);

    IplImage* temp = mvGetScratchImage();
    ds_image = cvCreateImageHeader(cvSize(width/DOWNSAMPLING_FACTOR, height/DOWNSAMPLING_FACTOR), IPL_DEPTH_8U, 1);
    ds_image->imageData = temp->imageData;

    if (DEBUG_SHAPE)
        window = new mvWindow("mvShape Debug");
    else
        window = NULL;
}

mvShape::~mvShape() {
    cvReleaseImageHeader(&ds_image);
    mvReleaseScratchImage();
    if (DEBUG_SHAPE) delete window;
}

void mvShape::downsample_from_image(IplImage* src, int target_brightness) {
    assert (src->nChannels = 1);

    // downsample image if needed
    if (src->width == ds_image->width && src->height == ds_image->height)
        cvCopy (src, ds_image);
    else
        cvResize (src, ds_image);

    // remove non-target_brightness pixels if needed
    if (target_brightness >= 0) {
        unsigned char* ptr;
        for (int i = 0; i < ds_image->height; i++) {
            ptr = (unsigned char*) (ds_image->imageData + i*ds_image->widthStep);
            for (int j = 0; j < ds_image->width; j++)
                if (*ptr != target_brightness)
                    *(ptr++) = 0;
        }    
    }
}

void mvShape::upsample_to_image(IplImage *dst) {
    if (dst->width == ds_image->width && dst->height == ds_image->height)
        cvCopy (ds_image, dst);
    else
        cvResize (ds_image, dst);
}

int mvShape::collect_pixels (IplImage* src, POINT_VECTOR &p_vector, int target_brightness) {
// put all pixels with value == target_brightness into the array p_vector
    unsigned char* ptr;

    if (target_brightness >= 0) {   // target_brightness pixels only
        for (int i = 0; i < src->height; i++) {
            ptr = (unsigned char*) (src->imageData + i*src->widthStep);
            for (int j = 0; j < src->width; j++)
                if (*(ptr++) == target_brightness)
                    p_vector.push_back(cvPoint(j,i));
        }
    }
    else {  // all nonzero pixels
        for (int i = 0; i < src->height; i++) {
            ptr = (unsigned char*) (src->imageData + i*src->widthStep);
            for (int j = 0; j < src->width; j++)
                if (*(ptr++) != 0)
                    p_vector.push_back(cvPoint(j,i));
        }   
    }

    return p_vector.size();
}

// ######################################################################################
//  mvRect class
// ######################################################################################

mvRect::mvRect(const char* settings_file) : 
    mvShape(settings_file),
    bin_rect("mvRect")
{
    read_mv_setting (settings_file, "MIN_STACKED_ROWS", MIN_STACKED_ROWS);
    read_mv_setting (settings_file, "MIN_ROW_LENGTH", MIN_ROW_LENGTH);
    read_mv_setting (settings_file, "RECT_HEIGHT_TO_WIDTH_RATIO", RECT_HEIGHT_TO_WIDTH_RATIO);   
}

mvRect::~mvRect() {
}

int mvRect::find(IplImage* img, int target_brightness) {
    int result;
    
    if (img->width == ds_image->width && img->height == ds_image->height) { // no downsampling required
        result = find_internal (img, target_brightness);
    }
    else {
        downsample_from_image (img, target_brightness);
        result = find_internal (ds_image, target_brightness);
        upsample_to_image (img);
    }
    
    return result;
}

int mvRect::find_internal(IplImage* img, int target_brightness) {
    // algorithm description

    bin_rect.start();
    m_rect_v.clear();

    POINT_VECTOR point_vector;
    std::vector<ROW> row_vector;
    std::vector<ROW> stacked_rows;

    /// put appropriate pixels into point_vector
    int n_points = collect_pixels (img, point_vector, target_brightness);
    if (n_points < MIN_POINTS_IN_RESAMPLED_IMAGE) {
        DEBUG_PRINT ("mvRect: Not enough points in resampled image\n");
        return -1;
    }

    /// go through the point_vector, identify long rows and put them in row_vector
    /// this is done simply by keeping track of continuous pixels on a row and putting the row into the
    /// vector if number of continuous pixels is greater than MIN_ROW_LENGTH 
    int contiguous_length = 0;
    POINT_VECTOR::iterator prev_iter = point_vector.begin();
    POINT_VECTOR::iterator iter = prev_iter + 1;
    POINT_VECTOR::iterator first_iter; 
    for (; iter != point_vector.end(); ++iter,++prev_iter) {
        if (iter->y == prev_iter->y  &&  iter->x - prev_iter->x <= 2) { // contiguous point
            if (contiguous_length == 0) // start of new sequence
                first_iter = prev_iter;

            contiguous_length++;
        }
        else {
            // end of current sequence
            if (contiguous_length >= MIN_ROW_LENGTH) {
                row_vector.push_back(make_row(first_iter->y, first_iter->x, prev_iter->x));
            }

            contiguous_length = 0;
        }   
    }

    for (std::vector<ROW>::iterator it = row_vector.begin(); it != row_vector.end(); ++it)
        DEBUG_PRINT ("\tRow %d, %d-%d\n", it->y, it->x1, it->x2);
    
    /// iterate over the rows and cluster them
    for (std::vector<ROW>::iterator row_iter = row_vector.begin(); row_iter != row_vector.end(); ++row_iter) {
        /// compare the row object with all existing m_rect_v objects, clustering to closest one or adding a new
        /// m_rect_v if no good clustering option
        bool cluster_success = false;
        for (std::vector<MV_SHAPE_RECT>::iterator rect_iter = m_rect_v.begin(); rect_iter != m_rect_v.end(); ++rect_iter) {
            if (row_iter->y >= rect_iter->y1-2 && row_iter->y <= rect_iter->y2+2 &&
                abs(row_iter->x1 - rect_iter->x1) <= 2 && abs(row_iter->x2 - rect_iter->x2) <= 2)
            {
                rect_iter->x1 = (rect_iter->num*rect_iter->x1 + row_iter->x1) / (rect_iter->num+1);
                rect_iter->x2 = (rect_iter->num*rect_iter->x2 + row_iter->x2) / (rect_iter->num+1);
                rect_iter->y1 = std::min(row_iter->y, rect_iter->y1);
                rect_iter->y2 = std::max(row_iter->y, rect_iter->y2);
                rect_iter->num++;

                cluster_success = true;
            }
        }

        if (!cluster_success)
            m_rect_v.push_back(make_rect(row_iter->x1,row_iter->y, row_iter->x2,row_iter->y));
    }

    for (std::vector<MV_SHAPE_RECT>::iterator it = m_rect_v.begin(); it != m_rect_v.end();) {
        float hw_ratio = (float)(it->y2 - it->y1) / (float)(it->x2 - it->x1);

        if (it->num < 6 || hw_ratio > 1.2*RECT_HEIGHT_TO_WIDTH_RATIO || hw_ratio < 0.8*RECT_HEIGHT_TO_WIDTH_RATIO)
            m_rect_v.erase(it);
        else
            it++;
    }

    get_rect_color (img);
    
    bin_rect.stop();

    if (window != NULL) window->showImage(img); // window may not exist if mvShape::DEBUG_SHAPE is not set
    return 0;
}

void mvRect::get_rect_color(IplImage* img) {
// takes the internal array of rectangles, and checks what color each of them are on the input image
// the check is done by looking at 5 different points in a cruciform pattern inside the rectangle
    for (std::vector<MV_SHAPE_RECT>::iterator it = m_rect_v.begin(); it != m_rect_v.end(); ++it) {
        int color_count[5] = {0,0,0,0,0};
        int Px[5], Py[5];
        it->color = MV_UNCOLORED;

        Px[0] = Px[1] = Px[4] = (it->x1 + it->x2) / 2; // P1 and P2 are the vertical points
        Py[0] = (3*it->y1 + it->y2) / 4;
        Py[1] = (3*it->y2 + it->y1) / 4;

        Py[2] = Py[3] = Py[4] = (it->y1 + it->y2) / 2; // P1 and P2 are the horiz points
        Px[2] = (3*it->x1 + it->x2) / 4;
        Px[3] = (3*it->x2 + it->x1) / 4;

        for (int i = 0; i < 5; i++) {
            unsigned char pixel_color = (unsigned char)(*(img->imageData + Py[i]*img->widthStep + Px[i]));
            switch (pixel_color) { 
                case MV_RED: color_count[0]++; break;
                case MV_YELLOW: color_count[1]++; break;
                case MV_GREEN: color_count[2]++; break;
                case MV_BLUE: color_count[3]++; break;
                default:;
            }            
        }

        // find which color has the highest count, then compare it to the the num of pixels checked
        int max_count=-1, max_count_index=-1;
        for (int i = 0; i < 4; i++) {
            if (color_count[i] > max_count) {
                max_count = color_count[i];
                max_count_index = i;
            }
        }
        DEBUG_PRINT ("get_rect_color: %d/5 pixels checked are colored %d, %s\n", max_count, MV_COLOR_VECTOR[max_count_index], color_int_to_string(MV_COLOR_VECTOR[max_count_index]).c_str());
        if (max_count >= 3) {
            it->color = MV_COLOR_VECTOR[max_count_index];
        }
    }
}

bool m_circle_has_greater_count (MV_SHAPE_CIRCLE c1, MV_SHAPE_CIRCLE c2) { return (c1.num > c2.num); }

mvAdvancedCircles::mvAdvancedCircles (const char* settings_file) :
    mvShape(settings_file),
    bin_findcircles ("mvAdvancedCircles")
{
    srand(time(NULL)); // seed for the partly random circle finding algorithm

    int width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
    read_mv_setting (settings_file, "_MIN_RADIUS_", _MIN_RADIUS_);
    read_mv_setting (settings_file, "_THRESHOLD_", _THRESHOLD_);
    read_mv_setting (settings_file, "N_CIRCLES_GENERATED", N_CIRCLES_GENERATED);
    read_mv_setting (settings_file, "N_CIRCLES_CUTOFF", N_CIRCLES_CUTOFF);

    float angular_division = 2*CV_PI/N_POINTS_TO_CHECK;
    float angle = 0;
    for (int i = 0; i < N_POINTS_TO_CHECK; i++) {
        cos_sin_vector.push_back(std::make_pair(cos(angle),sin(angle)));
        angle += angular_division;
    }
}

mvAdvancedCircles::~mvAdvancedCircles () {
}

int mvAdvancedCircles::find (IplImage* img) {
    int result;

    if (img->width == ds_image->width && img->height == ds_image->height) { // no downsampling required
        result = find_internal (img);
    }
    else {
        downsample_from_image (img);
        result = find_internal (ds_image);
        upsample_to_image (img);
    }
    return result;
}

int mvAdvancedCircles::find_internal (IplImage* img) {
    assert (img != NULL);
    assert (img->nChannels == 1);
    
    bin_findcircles.start();
    accepted_circles.clear();

    /// extract all the nonzero pixels and put them in an array
    POINT_VECTOR point_vector;
    /// put appropriate pixels into point_vector
    int n_points = collect_pixels (img, point_vector);
    if (n_points < MIN_POINTS_IN_RESAMPLED_IMAGE) {
        DEBUG_PRINT ("mvCircles: Not enough points in resampled image\n");
        return -1 ;
    }

    /// main working loop
    int n_valid_circles = 0;
    for (int N = 0; N < 60*N_CIRCLES_GENERATED; N++) {
        // choose a point at random then choose a point with same Y, then a point with same X
        int c1, c2, c3;
        c1 = rand() % n_points;
        do {c2 = rand() % n_points;} while (c2 == c1);
        do {c3 = rand() % n_points;} while (c3 == c1 || c3 == c2);
        /*
        // now choose a point with same Y coord as c1, by looking near c1 first
        int i=c1, j=c1;
        while (1) {
            // search forward with index i until hit end of vector or hit a point that is not within 1 row of c1
            if (i < n_points-1 && point_vector[++i].y-point_vector[c1].y <= 1) { 
                if (std::abs(point_vector[i].x - point_vector[c1].x) > 3) {
                    c3 = i;
                    break;
                }
            }
            // search backward with index j until hit end of vector or hit a point that is not within 1 row of c1
            else if (j > 0 && point_vector[c1].y-point_vector[--j].y <= 1) {
                if (std::abs(point_vector[j].x - point_vector[c1].x) > 3) {
                    c3 = j;
                    break;
                }
            }
            // break if both iterators cannot advance 
            else {
                c3 = rand() % n_points;
                break;
            }
        }*/
            
        // get the circle center and radius
        MV_SHAPE_CIRCLE Circle;
        Circle.num = 1;
        Circle.color = 0;
        if ( get_circle_from_3_points (point_vector[c1],point_vector[c2],point_vector[c3], Circle) )
            continue;

        // scale back to normal-sized image
        Circle.x *= DOWNSAMPLING_FACTOR;
        Circle.y *= DOWNSAMPLING_FACTOR;
        Circle.rad *= DOWNSAMPLING_FACTOR;

        // sanity roll
        if (Circle.x < 0 || Circle.x > img->width || Circle.y < 0 || Circle.y > img->height)
            continue;
        if (Circle.rad < _MIN_RADIUS_*img->width)
            continue;

        int count = check_circle_validity(img, Circle);
        //DEBUG_PRINT("count: %d\n", count);
        if(count < _THRESHOLD_*N_POINTS_TO_CHECK)
            continue;

        // check each existing circle. avg with an existing circle or push new circle
        bool similar_circle_exists = false;
        for (unsigned i = 0; i < accepted_circles.size(); i++) {
            int x = accepted_circles[i].x;
            int y = accepted_circles[i].y;
            int r = accepted_circles[i].rad;
            int c = accepted_circles[i].num;

            int dx = Circle.x - x;
            int dy = Circle.y - y;
            int dr = Circle.rad - r;
            if (abs(dr) < RADIUS_SIMILARITY_CONSTANT && dx*dx + dy*dy < CENTER_SIMILARITY_CONSTANT) {
                accepted_circles[i].x = (x*c + Circle.x)/(c + 1);
                accepted_circles[i].y = (y*c + Circle.y)/(c + 1);
                accepted_circles[i].rad = (r*c + Circle.rad)/(c + 1);
                accepted_circles[i].num++;

                // erase the circle from resampled image if cutoff is reached
                // this gets rid of a circle that is already found
                // this is done by removing points from point_vector
                if (accepted_circles[i].num == N_CIRCLES_CUTOFF) {
                    for (unsigned j = 0; j < point_vector.size(); j++) {
                        MV_SHAPE_CIRCLE temp_circle;
                        temp_circle.x = accepted_circles[i].x / DOWNSAMPLING_FACTOR;
                        temp_circle.y = accepted_circles[i].y / DOWNSAMPLING_FACTOR;
                        temp_circle.rad = accepted_circles[i].rad / DOWNSAMPLING_FACTOR;

                        int dx = point_vector[j].x - temp_circle.x;
                        int dy = point_vector[j].y - temp_circle.y;
                        float ratio = sqrt(dx*dx + dy*dy) / temp_circle.rad;

                        if (ratio < 1.2 && ratio > 0.83) {    // remove point if its approximately on temp_circle
                            point_vector.erase(point_vector.begin()+j);
                        }
                    }

                    get_circle_color (img, accepted_circles[i]);
                }

                similar_circle_exists = true;
                break;
            }
        }
        if (!similar_circle_exists) {
            accepted_circles.push_back( Circle );
        }

        n_valid_circles++;
        if (n_valid_circles >= N_CIRCLES_GENERATED) {
            break;
        }
    }

    /// remove each circle with less than N_CIRCLES_CUTOFF support
    CIRCLE_VECTOR::iterator it = accepted_circles.begin();
    /// Note: The loop is strange because it is erasing while iterating.
    ///       You will risk strange segfaults if you change this!
    while (it != accepted_circles.end()) {
        if (it->num < N_CIRCLES_CUTOFF) {
            accepted_circles.erase(it);
        } else {
            ++it;
        }
    }

    /// sort the circles by their counts
    std::sort (accepted_circles.begin(), accepted_circles.end(), m_circle_has_greater_count);

    /*for (unsigned i = 0; i < accepted_circles.size(); i++) {
        MV_SHAPE_CIRCLE circle = accepted_circles[i];
        DEBUG_PRINT ("(x=%d, y=%d, R=%f)  c=%d\n", circle.x,circle.y,circle.rad,accepted_circles[i].num);
    }*/

    bin_findcircles.stop();
    return 0;
}

void mvAdvancedCircles::drawOntoImage (IplImage* img) {
    CIRCLE_VECTOR::iterator iter = accepted_circles.begin();
    CIRCLE_VECTOR::iterator end_iter = accepted_circles.end();
    for (; iter != end_iter; ++iter) {
        cvCircle (img, cvPoint(iter->x,iter->y), iter->rad, CV_RGB(100,100,100), CIRCLE_THICKNESS);        
    }
}

int mvAdvancedCircles::ncircles() {
    return (int)(accepted_circles.size());
}

int mvAdvancedCircles::get_circle_from_3_points (CvPoint p1, CvPoint p2, CvPoint p3, MV_SHAPE_CIRCLE &Circle) {
    /** Basically, if you have 3 points, you can solve for the point which is equidistant to all 3.
     *  You get a matrix equation Ax = B, where A is a 2x2 matrix, x is trans([X, Y]), and B is trans([B1 B2])
     *  Then you can solve for x = inv(A) * B. Which is what we do below.
    */
    int x1=p1.x,  y1=p1.y;
    int x2=p2.x,  y2=p2.y;
    int x3=p3.x,  y3=p3.y;

    // calcate the determinant of A
    int DET = (x1-x2)*(y1-y3)-(y1-y2)*(x1-x3);
    if (DET == 0) { // this means the points are colinear
        return 1;
    }

    // caculate the 2 components of B
    int B1 = x1*x1 + y1*y1 - x2*x2 - y2*y2;
    int B2 = x1*x1 + y1*y1 - x3*x3 - y3*y3;

    // solve for X and Y
    int X = ( (y1-y3)*B1 + (y2-y1)*B2 ) / 2 / DET;
    int Y = ( (x3-x1)*B1 + (x1-x2)*B2 ) / 2 / DET;

    Circle.x = X;
    Circle.y = Y;
    Circle.rad = sqrt((float)((x1-X)*(x1-X) + (y1-Y)*(y1-Y)));
    return 0;
}

int mvAdvancedCircles::check_circle_validity (IplImage* img, MV_SHAPE_CIRCLE Circle) {
    int x, y, count = 0;
    std::vector<FLOAT_PAIR>::iterator it = cos_sin_vector.begin();
    std::vector<FLOAT_PAIR>::iterator end_it = cos_sin_vector.end();

    for(; it != end_it; ++it) {
        float cos_val = it->first;
        float sin_val = it->second;
        x = Circle.x + Circle.rad*cos_val;
        y = Circle.y + Circle.rad*sin_val;

        if (x < 0 || x > img->width || y < 0 || y > img->height)
            continue;

        unsigned char* ptr = (unsigned char*) (img->imageData + y*img->widthStep + x);
        if (*(ptr) != 0) {
            count++;
        }
    }

    return count;
}

int mvAdvancedCircles::get_circle_color (IplImage* img, MV_SHAPE_CIRCLE &Circle) {
    Circle.color = MV_UNCOLORED;
    int x, y;
    int color_count[4] = {0,0,0,0};
    
    std::vector<FLOAT_PAIR>::iterator it = cos_sin_vector.begin();
    std::vector<FLOAT_PAIR>::iterator end_it = cos_sin_vector.end();
    for(; it != end_it; ++it) {
        float cos_val = it->first;
        float sin_val = it->second;
        x = Circle.x + Circle.rad*cos_val;
        y = Circle.y + Circle.rad*sin_val;

        if (x < 0 || x > img->width || y < 0 || y > img->height)
            continue;

        unsigned char* ptr = (unsigned char*) (img->imageData + y*img->widthStep + x);
        switch (*(ptr)) { 
            case MV_RED: color_count[0]++; break;
            case MV_YELLOW: color_count[1]++; break;
            case MV_GREEN: color_count[2]++; break;
            case MV_BLUE: color_count[3]++; break;
            default:;
        }
    }

    // find which color has the highest count, then compare it to the the num of pixels
    int max_count=-1, max_count_index=-1;
    for (int i = 0; i < 4; i++) {
        if (color_count[i] > max_count) {
            max_count = color_count[i];
            max_count_index = i;
        }
    }
    DEBUG_PRINT ("get_circle_color: max_count = %d, n_pixels = %d\n", max_count, (int)cos_sin_vector.size());
    if (max_count > (int)cos_sin_vector.size()/4) {
        Circle.color = MV_COLOR_VECTOR[max_count_index];
        return 1;
    }

    return 0;
}
