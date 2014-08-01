#include "mda_vision.h"
#include "../common.h"

#ifdef DEBUG_VISION_GOALPOST
    #define DEBUG_PRINT(_n, _format, ...) if(_n<=DEBUG_LEVEL)printf(_format, ##__VA_ARGS__)
    #define DEBUG_SHOWIMAGE(_n, _win, _img) if(_n<=DEBUG_LEVEL)_win.showImage(_img);
    #define DEBUG_WAITKEY(_n,_msecs) if(_n<=DEBUG_LEVEL)WAITKEY(_msecs);
#else
    #define DEBUG_PRINT(n, format, ...)
#endif

const char MDA_VISION_MODULE_GOALPOST::MDA_VISION_GOALPOST_SETTINGS[] = "vision_goalpost_settings.csv";

/// #########################################################################
/// MODULE_GOALPOST methods
/// #########################################################################
MDA_VISION_MODULE_GOALPOST::MDA_VISION_MODULE_GOALPOST () :
    window (mvWindow("Goalpost Vision Module")),
    window2 (mvWindow("Goalpost Vision Module 2"))//,
{
    read_color_settings (MDA_VISION_GOALPOST_SETTINGS);
    read_mv_setting (MDA_VISION_GOALPOST_SETTINGS, "GOALPOST_DEBUG_LEVEL", DEBUG_LEVEL);
    contour_filter.set_debug_level(DEBUG_LEVEL);
    rectangle_params = read_rectangle_settings(MDA_VISION_GOALPOST_SETTINGS);

    N_FRAMES_TO_KEEP = 8;
    gray_img = mvGetScratchImage();
    gray_img_2 = mvGetScratchImage2();
}

MDA_VISION_MODULE_GOALPOST::~MDA_VISION_MODULE_GOALPOST () {
    mvReleaseScratchImage();
    mvReleaseScratchImage2();
}

void MDA_VISION_MODULE_GOALPOST::primary_filter (IplImage* src) {
    // shift the frames back by 1
    shift_frame_data (m_frame_data_vector, read_index, N_FRAMES_TO_KEEP);

    watershed_filter.watershed(src, gray_img, mvWatershedFilter::WATERSHED_STEP_SMALL);
    DEBUG_PRINT(1,"VISION_GOALPOST: Number of Segments from watershed: %d\n", watershed_filter.num_watershed_segments());

    DEBUG_SHOWIMAGE (1, window, gray_img);
    DEBUG_PRINT(1,"VISION_GOALPOST: showing source img from watershed\n");
    DEBUG_WAITKEY(1,0);
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_GOALPOST::calc_vci () {
    MDA_VISION_RETURN_CODE retval = NO_TARGET;
    COLOR_TRIPLE color;
    MvRotatedBox rbox;
    MvRBoxVector rbox_vector, rbox_vector_filtered;
    rbox_vector_filtered.resize (2);

    // In this step, loop over every segment and try to find one that looks like a rectangle
    while ( watershed_filter.get_next_watershed_segment(gray_img_2, color) ) {

        DEBUG_PRINT(1,"VISION_GOALPOST: segment color: BGR=(%3d,%3d,%3d)\n", color.m1, color.m2, color.m3);
        DEBUG_PRINT(2, "VISION_GOALPOST: comparing against: %s\n", color_limit_string().c_str());

        if (!check_color_triple(color)) {
            DEBUG_PRINT(2,"VISION_GOALPOST: rejected segment due to color\n");
            DEBUG_WAITKEY(2,0);
            continue;
        }

        bool found = contour_filter.match_rectangle(gray_img_2, &rbox_vector, color, rectangle_params);
        if (DEBUG_LEVEL >= 2) {
            cvZero(gray_img_2);
            contour_filter.drawOntoImage(gray_img_2);
            DEBUG_SHOWIMAGE(2,window2, gray_img_2);
            DEBUG_PRINT(2, "Rose: just printed all contours from last segment\n");
            DEBUG_WAITKEY(2,0);
        }

        DEBUG_PRINT(1,"VISION_GOALPOST: Segment was %s\n", found ? "\e[0;32maccepted\e[0m" : "\e[0;31mrejected\e[0m");
        DEBUG_WAITKEY(2,0);
    }
    DEBUG_PRINT(1, "VISION_GOALPOST: Num rectangles identified: %d\n", (int)rbox_vector.size());    


    for (unsigned i = 0; i < rbox_vector.size(); i++) {
        //look for vertical red rbox
        if ((rbox_vector[i].m1 < 100 && rbox_vector[i].m2 < 100 && rbox_vector[i].m3 > 150)&&(abs(rbox_vector[i].angle) < 30)) {
            rbox_vector_filtered [0] = rbox_vector[i];
            DEBUG_PRINT (2, "GOALPOST: Found vertical red segment\n");
        }
        //look for horizontal green box
        if ((rbox_vector[i].m1 < 100 && rbox_vector[i].m2 > 150 && rbox_vector[i].m3 < 100)&&(abs(rbox_vector[i].angle) > 60)) {
            rbox_vector_filtered [1] = rbox_vector[i];
            DEBUG_PRINT (2, "GOALPOST: Found horizontal green segment\n");
        }
    }

    // debug
    cvZero(gray_img);
    for (unsigned i = 0; i < rbox_vector_filtered.size(); i++) {
        if (rbox_vector_filtered[i].validity < 0)
            continue;
        printf ("Box %d: (%d,%d), angle=%f,   color=(%d,%d,%d)\n", i+1, 
            (int)rbox_vector_filtered[i].length, (int)rbox_vector_filtered[i].width, rbox_vector_filtered[i].angle,
            rbox_vector_filtered[i].m1, rbox_vector_filtered[i].m2, rbox_vector_filtered[i].m3);
        rbox_vector_filtered[i].drawOntoImage(gray_img);
    }

    const bool seg_red_v_valid = rbox_vector_filtered[0].validity > 0;
    const bool seg_green_h_valid = rbox_vector_filtered[1].validity > 0;

    // 2 valid segments
    if (seg_red_v_valid && seg_green_h_valid) {
        m_pixel_x = rbox_vector_filtered[0].center.x + rbox_vector_filtered[1].length/8*3 - gray_img->width/2;
        m_pixel_y = rbox_vector_filtered[0].center.y - gray_img->height/2;
        //rbox length is used as height of red box
        m_range = (float)(GOALPOST_REAL_HEIGHT * gray_img->height / rbox_vector_filtered[0].length * TAN_FOV_Y);
        retval = FULL_DETECT;
        DEBUG_PRINT (2, "Goalpost: Full detect\n");
    }
    else if (seg_red_v_valid && !seg_green_h_valid) {
        m_pixel_x = rbox_vector_filtered[0].center.x - gray_img->width/2 + rbox_vector_filtered[0].length/8*3;
        m_pixel_y = rbox_vector_filtered[0].center.y - gray_img->height/2;
        //rbox length is used as height of red box
        m_range = (float)(GOALPOST_REAL_HEIGHT * gray_img->height / rbox_vector_filtered[0].length * TAN_FOV_Y);
        retval = ONE_SEGMENT;
        DEBUG_PRINT (2, "Goalpost: One segment: red\n");
    }
    else if (!seg_red_v_valid && seg_green_h_valid) {
        m_pixel_x = rbox_vector_filtered[1].center.x - gray_img->width/2 + rbox_vector_filtered[1].length/8*3;
        m_pixel_y = rbox_vector_filtered[1].center.y - gray_img->height/2 - rbox_vector_filtered[1].length/3;
        //rbox length is used as length of green box
        m_range = (float)(GOALPOST_REAL_WIDTH * gray_img->width / rbox_vector_filtered[1].length * TAN_FOV_X);
        retval = ONE_SEGMENT;
        DEBUG_PRINT (2, "Goalpost: One segment: green\n");
    }
    else {
        DEBUG_PRINT (2, "Goalpost: No Target\n");
        return NO_TARGET;
    }
    cvCircle(gray_img, cvPoint(m_pixel_x + gray_img->width/2, m_pixel_y + gray_img->height/2), 5, CV_RGB(200, 200, 200), -1);
    DEBUG_SHOWIMAGE (2, window2, gray_img);

    m_angular_x = RAD_TO_DEG * atan(TAN_FOV_X * m_pixel_x / gray_img->width);
    m_angular_y = RAD_TO_DEG * atan(TAN_FOV_Y * m_pixel_y / gray_img->height);
    DEBUG_PRINT (2, "Goalpost: m_pixel_y, m_pixel_y (%d,%d)  m_angular_x, m_angular_y (%5.2f, %5.2f)  range=%d\n", m_pixel_x, m_pixel_y, m_angular_x, m_angular_y, m_range);
    
    return retval;
}
