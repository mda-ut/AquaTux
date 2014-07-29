#include "mda_vision.h"
#include "../common.h"

#ifdef DEBUG_VISION_PATH
    #define DEBUG_PRINT(_n, _format, ...) if(_n<=DEBUG_LEVEL)printf(_format, ##__VA_ARGS__)
	#define DEBUG_SHOWIMAGE(_n, _win, _img) if(_n<=DEBUG_LEVEL)_win.showImage(_img);
    #define DEBUG_WAITKEY(_n,_msecs) if(_n<=DEBUG_LEVEL)WAITKEY(_msecs);
#else
    #define DEBUG_PRINT(n, format, ...)
#endif

float absf(float f){
    if(f < 0) return f*-1;
    else return f;
}

const char MDA_VISION_MODULE_PATH::MDA_VISION_PATH_SETTINGS[] = "vision_path_settings.csv";

static const int N_GOOD_FRAMES_NEEDED = 1;

/// ########################################################################
/// MODULE_PATH methods
/// ########################################################################
MDA_VISION_MODULE_PATH::MDA_VISION_MODULE_PATH () :
	window (mvWindow("Path Vision 1")),
    window2 (mvWindow("Path Vision 2")),
    Morphology (mvBinaryMorphology(19, 19, MV_KERN_RECT)),
    Morphology2 (mvBinaryMorphology(7, 7, MV_KERN_RECT))//,
{
    read_color_settings (MDA_VISION_PATH_SETTINGS);
    read_mv_setting (MDA_VISION_PATH_SETTINGS, "PATH_DEBUG_LEVEL", DEBUG_LEVEL);
    contour_filter.set_debug_level(DEBUG_LEVEL);
    rectangle_params = read_rectangle_settings(MDA_VISION_PATH_SETTINGS);

    //read_mv_setting (MDA_VISION_PATH_SETTINGS, "DIFF_THRESHOLD", DIFF_THRESHOLD_SETTING);

    N_FRAMES_TO_KEEP = 2;
    gray_img = mvGetScratchImage();
    gray_img_2 = mvGetScratchImage2();
}

MDA_VISION_MODULE_PATH::~MDA_VISION_MODULE_PATH () {
    mvReleaseScratchImage();
    mvReleaseScratchImage2();
}

void MDA_VISION_MODULE_PATH::add_frame (IplImage* src) {
    // shift the frames back by 1
    shift_frame_data (m_frame_data_vector, read_index, N_FRAMES_TO_KEEP);
/*
    // BGR hack!
    unsigned char *srcptr;
    int zeros = 0;
    for (int i = 0; i < src->height; i++) {
        srcptr = (unsigned char*)src->imageData + i*src->widthStep;
        for (int j = 0; j < src->width; j++) {
            if (srcptr[2] < 100) {
                srcptr[0] = 0;
                srcptr[1] = 0;
                srcptr[2] = 0;
                zeros++;
            }
            srcptr += 3;
        }
    }

    if (zeros > 0.999 * 400*300) {
        printf ("Path: add_frame: not enough pixels\n");
        return;
    }
*/
//    window.showImage (src);
    watershed_filter.watershed(src, gray_img, mvWatershedFilter::WATERSHED_STEP_SMALL);

    COLOR_TRIPLE color;
    MvRotatedBox rbox;
    MvRBoxVector rbox_vector;

    DEBUG_PRINT(1,"VISION_PATH: Number of Segments from watershed: %d\n", watershed_filter.num_watershed_segments());

    while ( watershed_filter.get_next_watershed_segment(gray_img_2, color) ) {
        // check that the segment is roughly red
        DEBUG_PRINT(2,"VISION_PATH: segment color: BGR=(%3d,%3d,%3d)\n", color.m1, color.m2, color.m3);
        DEBUG_PRINT(2, "VISION_PATH: comparing against: %s\n", color_limit_string().c_str());
        if (!check_color_triple(color)) {
            DEBUG_PRINT(2,"VISION_PATH: \e[0;31mrejected rectangle due to color\e[0m\n\n");
            continue;
        }

        bool found = contour_filter.match_rectangle(gray_img_2, &rbox_vector, color, rectangle_params);
        if (DEBUG_LEVEL >= 2) {
            cvZero(gray_img_2);
            contour_filter.drawOntoImage(gray_img_2);
            DEBUG_SHOWIMAGE(2,window2, gray_img_2);
            DEBUG_WAITKEY(2,0);
        }
        DEBUG_PRINT(1,"VISION_PATH: Segment was %s\n", found ? "\e[0;32maccepted\e[0m" : "\e[0;31mrejected\e[0m");
        DEBUG_WAITKEY(2,0);
    }

    DEBUG_PRINT(1, "VISION_PATH: Num rectangles identified: %d\n", (int)rbox_vector.size());    

    // debug only
    cvCopy (gray_img, gray_img_2);

    if (rbox_vector.size() > 0) {
        MvRBoxVector::iterator iter = rbox_vector.begin();
        MvRBoxVector::iterator iter_end = rbox_vector.end();
     
        // for now, frame will store rect with best validity
        for (; iter != iter_end; ++iter) {
            if (iter->length * iter->width < 40*20 || iter->length * iter->width > 300*10)
                continue;
            /*if (iter->center.x - iter->width/2 == 1 || iter->center.x + iter->width/2 == 398)
                continue;
            if (iter->center.y - iter->width/2 == 1 || iter->center.y + iter->width/2 == 298)
                continue;*/
            m_frame_data_vector[read_index].assign_rbox_by_validity(*iter);
        }
    }

    if (m_frame_data_vector[read_index].is_valid()) {
        m_frame_data_vector[read_index].drawOntoImage(gray_img_2);
        window2.showImage (gray_img_2);
    }

    //print_frames();
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_PATH::frame_calc () {
    MDA_VISION_RETURN_CODE retval = FATAL_ERROR;
   
    // go thru each frame and pull all individual segments into a vector
    // set i to point to the element 1 past read_index
    int i = read_index + 1;
    if (i >= N_FRAMES_TO_KEEP) i = 0;
    MvRBoxVector segment_vector;
    do {
        if (m_frame_data_vector[i].has_data() && m_frame_data_vector[i].rboxes_valid[0]) {
            segment_vector.push_back(m_frame_data_vector[i].m_frame_boxes[0]);
        }
        if (++i >= N_FRAMES_TO_KEEP) i = 0;
    } while (i != read_index);

    // bin the frames
    for (unsigned i = 0; i < segment_vector.size(); i++) {
        for (unsigned j = i+1; j < segment_vector.size(); j++) {
            CvPoint center1 = segment_vector[i].center;
            CvPoint center2 = segment_vector[j].center;
            int angle_delta = static_cast<int>(segment_vector[i].angle-segment_vector[j].angle);
            
            if (abs(center1.x-center2.x)+abs(center1.y-center2.y) < 50 && 
                abs(segment_vector[i].length-segment_vector[j].length) < 35 &&
                (abs(angle_delta) < 25 || abs(180-angle_delta) < 25)
            )
            {
                segment_vector[i].shape_merge(segment_vector[j]);
                segment_vector.erase(segment_vector.begin()+j);
                j--;
            }
        }
    }

    // sort by count
    std::sort (segment_vector.begin(), segment_vector.end(), shape_count_greater_than);

    // debug
    for (unsigned i = 0; i<=1 && i<segment_vector.size(); i++) {
        DEBUG_PRINT(1,"\tSegment %d (%3d,%3d) height=%3.0f, width=%3.0f   count=%d\n", i, segment_vector[i].center.x, segment_vector[i].center.y,
            segment_vector[i].length, segment_vector[i].width, segment_vector[i].count);
    }

    if (segment_vector.size() == 0 || segment_vector[0].count < N_GOOD_FRAMES_NEEDED) { // not enough good segments, return no target
        DEBUG_PRINT (1,"Path: No Target\n");
        return NO_TARGET;
    }
    else { // first segment is good enough, use that only
        DEBUG_PRINT (1,"Path: Segment found\n");
        
        // check path length to width
        float length_to_width = static_cast<float>(segment_vector[0].length) / segment_vector[0].width;
        if (length_to_width < rectangle_params.lw_ratio_min || length_to_width > rectangle_params.lw_ratio_min) {
            DEBUG_PRINT(2,"Path: length to width check failed\n");
            return NO_TARGET;
        }

        m_pixel_x = segment_vector[0].center.x;
        m_pixel_y = segment_vector[0].center.x;
        m_range = (PATH_REAL_LENGTH * gray_img->height) / (segment_vector[0].length * TAN_FOV_Y);
        m_angle = segment_vector[0].angle;

        retval = FULL_DETECT;
    }

#ifdef DEBUG_VISION_PATH
    segment_vector[0].drawOntoImage(gray_img);
    window2.showImage(gray_img);
#endif

    m_pixel_x -= gray_img->width/2;
    m_pixel_y -= gray_img->height/2;
    m_angular_x = RAD_TO_DEG * atan(TAN_FOV_X * m_pixel_x / gray_img->width);
    m_angular_y = RAD_TO_DEG * atan(TAN_FOV_Y * m_pixel_y / gray_img->height);
    printf ("Path (%3d,%3d) (%5.2f,%5.2f) angle=%3.0f  range=%d\n", m_pixel_x, m_pixel_y, m_angular_x, m_angular_y, m_angle, m_range);
    return retval;
}
