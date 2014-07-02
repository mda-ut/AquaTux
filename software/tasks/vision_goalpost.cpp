#include "mda_vision.h"

#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

const char MDA_VISION_MODULE_GOALPOST::MDA_VISION_GOALPOST_SETTINGS[] = "vision_goalpost_settings.csv";

/// #########################################################################
/// MODULE_GOALPOST methods
/// #########################################################################
MDA_VISION_MODULE_GOALPOST:: MDA_VISION_MODULE_GOALPOST () :
    window (mvWindow("Goalpost Vision Module")),
    window2 (mvWindow("Goalpost Vision Module 2"))//,
{
    N_FRAMES_TO_KEEP = 8;
    gray_img = mvGetScratchImage();
    gray_img_2 = mvGetScratchImage2();
}

MDA_VISION_MODULE_GOALPOST:: ~MDA_VISION_MODULE_GOALPOST () {
    mvReleaseScratchImage();
    mvReleaseScratchImage2();
}

void MDA_VISION_MODULE_GOALPOST::primary_filter (IplImage* src) {
    // shift the frames back by 1
    shift_frame_data (m_frame_data_vector, read_index, N_FRAMES_TO_KEEP);


    watershed_filter.watershed(src, gray_img, mvWatershedFilter::WATERSHED_STEP_SMALL);
    window.showImage (src);
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_GOALPOST::calc_vci () {
    MDA_VISION_RETURN_CODE retval = NO_TARGET;
    COLOR_TRIPLE color;
    MvRotatedBox rbox;
    MvRBoxVector rbox_vector, rbox_vector_filtered;
    rbox_vector_filtered.resize (2);

    // In this step, loop over every segment and try to find one that looks like a rectangle
    while ( watershed_filter.get_next_watershed_segment(gray_img_2, color) ) {
        // check that the segment is roughly red
        contour_filter.match_rectangle(gray_img_2, &rbox_vector, color, 10.0, 40.0, 1);
        //window2.showImage (gray_img_2);
    }
    for (unsigned i = 0; i < rbox_vector.size(); i++) {
        //look for vertical red rbox
        if ((rbox_vector[i].m1 < 100 && rbox_vector[i].m2 < 100 && rbox_vector[i].m3 > 150)&&(abs(rbox_vector[i].angle) < 30)) {
            rbox_vector_filtered [0] = rbox_vector[i];
            //DEBUG_PRINT ("Found vertical red segment\n");
        }
        //look for horizontal green box
        if ((rbox_vector[i].m1 < 100 && rbox_vector[i].m2 > 150 && rbox_vector[i].m3 < 100)&&(abs(rbox_vector[i].angle) > 60)) {
            rbox_vector_filtered [1] = rbox_vector[i];
            //DEBUG_PRINT ("Found horizontal green segment\n");
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
        m_pixel_x = rbox_vector_filtered[0].center.x - gray_img->width/2;
        m_pixel_y = rbox_vector_filtered[0].center.y - gray_img->height/2;
        //rbox length is used as height of red box
        m_range = (float)(GOALPOST_REAL_HEIGHT * gray_img->height / rbox_vector_filtered[0].length * TAN_FOV_Y);
        retval = FULL_DETECT;
    }
    else if (seg_red_v_valid && !seg_green_h_valid) {
        m_pixel_x = rbox_vector_filtered[0].center.x - gray_img->width/2;
        m_pixel_y = rbox_vector_filtered[0].center.y - gray_img->height/2;
        //rbox length is used as height of red box
        m_range = (float)(GOALPOST_REAL_HEIGHT * gray_img->height / rbox_vector_filtered[0].length * TAN_FOV_Y);
        retval = ONE_SEGMENT;
    }
    else if (!seg_red_v_valid && seg_green_h_valid) {
        m_pixel_x = rbox_vector_filtered[1].center.x - gray_img->width/2;
        m_pixel_y = rbox_vector_filtered[1].center.y - gray_img->height/2 - rbox_vector_filtered[1].length/3;
        //rbox length is used as length of green box
        m_range = (float)(GOALPOST_REAL_WIDTH * gray_img->width / rbox_vector_filtered[1].length * TAN_FOV_X);
        retval = ONE_SEGMENT;
    }
    else {
        //DEBUG_PRINT ("Goalpost: No Target\n");
        return NO_TARGET;
    }
    cvCircle(gray_img, cvPoint(m_pixel_x + gray_img->width/2, m_pixel_y + gray_img->height/2), 10, CV_RGB(200, 200, 200), -1);
    window2.showImage (gray_img);

    m_angular_x = RAD_TO_DEG * atan(TAN_FOV_X * m_pixel_x / gray_img->width);
    m_angular_y = RAD_TO_DEG * atan(TAN_FOV_Y * m_pixel_y / gray_img->height);
    //DEBUG_PRINT ("Goalpost: (%d,%d) (%5.2f, %5.2f)  range=%d\n", m_pixel_x, m_pixel_y, m_angular_x, m_angular_y, m_range);
    
    return retval;
}
