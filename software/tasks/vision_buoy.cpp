#include "mda_vision.h"
#include "../common.h"

#ifdef DEBUG_VISION_GOALPOST
    #define DEBUG_PRINT(_n, _format, ...) if(_n<=DEBUG_LEVEL)printf(_format, ##__VA_ARGS__)
    #define DEBUG_SHOWIMAGE(_n, _win, _img) if(_n<=DEBUG_LEVEL)_win.showImage(_img);
    #define DEBUG_WAITKEY(_n,_msecs) if(_n<=DEBUG_LEVEL)WAITKEY(_msecs);
#else
    #define DEBUG_PRINT(n, format, ...)
#endif

const int ANGLE_LIMIT = 45;
const float LEN_TO_WIDTH_MAX = 3.0;
const float LEN_TO_WIDTH_MIN = 1.0;
const int FRAMES_TO_KEEP = 60;
const int FRAMES_THRESHOLD_FRACTION = 6;

const char MDA_VISION_MODULE_BUOY::MDA_VISION_BUOY_SETTINGS[] = "vision_buoy_settings.csv";

/// #########################################################################
/// MODULE_BUOY methods
/// #########################################################################
MDA_VISION_MODULE_BUOY::MDA_VISION_MODULE_BUOY () :
    window (mvWindow("Buoy Vision Module")),
    window2 (mvWindow("Buoy Vision Module 2")),
    Morphology5 (mvBinaryMorphology(5, 5, MV_KERN_RECT)),
    Morphology3 (mvBinaryMorphology(3, 3, MV_KERN_RECT))//,
{
    read_color_settings (MDA_VISION_BUOY_SETTINGS);
    read_mv_setting (MDA_VISION_BUOY_SETTINGS, "BUOY_DEBUG_LEVEL", DEBUG_LEVEL);
    read_mv_setting (MDA_VISION_BUOY_SETTINGS, "DIFF_THRESHOLD", DIFF_THRESHOLD_SETTING);
    rectangle_params = read_rectangle_settings(MDA_VISION_BUOY_SETTINGS);
    circle_params = read_circle_settings(MDA_VISION_BUOY_SETTINGS);

    gray_img = mvGetScratchImage();
    gray_img_2 = mvGetScratchImage2();
    //filtered_img = mvGetScratchImage (); // common size
    N_FRAMES_TO_KEEP = FRAMES_TO_KEEP;
}

MDA_VISION_MODULE_BUOY::~MDA_VISION_MODULE_BUOY () {
    mvReleaseScratchImage();
    mvReleaseScratchImage2();
}

void MDA_VISION_MODULE_BUOY::add_frame (IplImage* src) {
    // shift the frames back by 1
    shift_frame_data (m_frame_data_vector, read_index, N_FRAMES_TO_KEEP);

    COLOR_TRIPLE color;
    MvCircle circle;
    MvCircleVector circle_vector;
    MvRotatedBox rbox;
    MvRBoxVector rbox_vector;

    // HSV hack!
    /*unsigned char *srcptr;
    int zeros = 0;
    for (int i = 0; i < src->height; i++) {
        srcptr = (unsigned char*)src->imageData + i*src->widthStep;
        for (int j = 0; j < src->width; j++) {
            if (srcptr[2] < 15) {
                srcptr[0] = 0;
                srcptr[1] = 0;
                srcptr[2] = 0;
                zeros++;
            }
            srcptr += 3;
        }
    }*/
    /*if (zeros > 0.999 * 400*300) {
        printf ("Path: add_frame: not enough pixels\n");
        return;
    }*/

    watershed_filter.watershed(src, gray_img, mvWatershedFilter::WATERSHED_STEP_SMALL);
    DEBUG_SHOWIMAGE (2, window, gray_img);

    while ( watershed_filter.get_next_watershed_segment(gray_img_2, color) ) {
        cvCopy (gray_img_2, gray_img);
        /*int H,S,V;
        tripletBGR2HSV (color.m1,color.m2,color.m3, H,S,V);
        if (S < 10 || V < 60 || H > 70) {
            printf ("VISION_BUOY: rejected rectangle due to color: HSV=(%3d,%3d,%3d)\n", H,S,V);
            continue;
        }*/

        bool found = contour_filter.match_circle(gray_img_2, &circle_vector, color, circle_params);
        (void) found;
        //contour_filter.match_rectangle(gray_img, &rbox_vector, color, LEN_TO_WIDTH_MIN, LEN_TO_WIDTH_MAX);        
        //window2.showImage (gray_img_2);
        //cvWaitKey(0);
    }
/*
    if (circle_vector.size() > 0) {
        MvCircleVector::iterator iter = circle_vector.begin();
        MvCircleVector::iterator iter_end = circle_vector.end();
        
        // for now, frame will store circle with best validity
        for (; iter != iter_end; ++iter) {
            m_frame_data_vector[read_index].assign_circle_by_validity(*iter);
        }
    }
*/
    if (rbox_vector.size() > 0) {
        MvRBoxVector::iterator iter = rbox_vector.begin();
        MvRBoxVector::iterator iter_end = rbox_vector.end();
        
        // for now, frame will store rect with best validity
        for (; iter != iter_end; ++iter) {
            if (abs(iter->angle) <= ANGLE_LIMIT) { // dont add if angle > 30 degree
                m_frame_data_vector[read_index].assign_rbox_by_validity(*iter);
            }
        }
    }

    /*if (m_frame_data_vector[read_index].is_valid()) {
        m_frame_data_vector[read_index].drawOntoImage(gray_img_2);
        window2.showImage (gray_img_2);
    }*/

    //print_frames();
}

void MDA_VISION_MODULE_BUOY::rbox_calc(MvRBoxVector* rboxes_returned, int nframes) {
    assert (rboxes_returned != NULL);
    MvRotatedBox input_rbox;
    m_rbox_segment_vector.clear();
    
    // go thru each frame and pull all individual segments into a vector
    // set i to point to the element 1 past read_index
    int i = read_index + 1;
    if (i >= N_FRAMES_TO_KEEP) i = 0;
    do {
        if (m_frame_data_vector[i].has_data()) {
            if (m_frame_data_vector[i].rboxes_valid[0])
                m_rbox_segment_vector.push_back(m_frame_data_vector[i].m_frame_boxes[0]);
        }
        if (++i >= N_FRAMES_TO_KEEP) i = 0;
    } while (i != read_index);


    // bin the frames
    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        for (unsigned j = i+1; j < m_rbox_segment_vector.size(); j++) {
            CvPoint center1 = m_rbox_segment_vector[i].center;
            CvPoint center2 = m_rbox_segment_vector[j].center;  
            
            if (abs(center1.x-center2.x) < 25 &&
                abs(center1.y-center2.y) < 25 && 
                abs(m_rbox_segment_vector[i].length-m_rbox_segment_vector[j].length) < 40 &&
                abs(m_rbox_segment_vector[i].width-m_rbox_segment_vector[j].width) < 30 &&
                m_rbox_segment_vector[i].color_check(m_rbox_segment_vector[j], 30)
                //m_rbox_segment_vector[i].color_int == m_rbox_segment_vector[j].color_int
            )
            {
                m_rbox_segment_vector[i].shape_merge(m_rbox_segment_vector[j]);
                m_rbox_segment_vector.erase(m_rbox_segment_vector.begin()+j);
                j--;
            }
        }
    }

    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        if (m_rbox_segment_vector[i].count < nframes/FRAMES_THRESHOLD_FRACTION) {
            m_rbox_segment_vector.erase(m_rbox_segment_vector.begin()+i);
            i--;
        }
    }

    // sort by count
    std::sort (m_rbox_segment_vector.begin(), m_rbox_segment_vector.end(), shape_count_greater_than);

    // debug
    printf ("Buoy: rbox_calc segments:\n");
    for (unsigned i = 0; i < m_rbox_segment_vector.size() && i < 2; i++) {
        printf ("\tSegment %d (%3d,%3d) h=%1.0f,w=%1.0f color=%s  count=%d\n", i, 
            m_rbox_segment_vector[i].center.x, m_rbox_segment_vector[i].center.y,
            m_rbox_segment_vector[i].length, m_rbox_segment_vector[i].width, 
            color_int_to_string(m_rbox_segment_vector[i].color_int).c_str(), m_rbox_segment_vector[i].count);
    }

    // put the best rbox into the result for now
    if (0) {
    }
    else {
        rboxes_returned->clear();
        if (m_rbox_segment_vector.size() > 0 && m_rbox_segment_vector[0].validity >= nframes/FRAMES_THRESHOLD_FRACTION) {
            m_rbox_segment_vector[0].center.x -= gray_img->width/2;
            m_rbox_segment_vector[0].center.y -= gray_img->height/2;
            rboxes_returned->push_back(m_rbox_segment_vector[0]);
            // debug
            m_rbox_segment_vector[0].drawOntoImage(gray_img);
            window2.showImage(gray_img);
        }
    }
}

/*
void MDA_VISION_MODULE_BUOY::rbox_calc(MvRBoxVector* rboxes_returned, int nframes) {
    assert (rboxes_returned != NULL);
    MvRotatedBox input_rbox;
    m_rbox_segment_vector.clear();
    
    // go thru each frame and pull all individual segments into a vector
    // set i to point to the element 1 past read_index
    int i = read_index + 1;
    if (i >= N_FRAMES_TO_KEEP) i = 0;
    do {
        if (m_frame_data_vector[i].has_data()) {
            if (m_frame_data_vector[i].rboxes_valid[0])
                m_rbox_segment_vector.push_back(m_frame_data_vector[i].m_frame_boxes[0]);
            if (m_frame_data_vector[i].rboxes_valid[1])
                m_rbox_segment_vector.push_back(m_frame_data_vector[i].m_frame_boxes[1]);
        }
        if (++i >= N_FRAMES_TO_KEEP) i = 0;
    } while (i != read_index);


    // bin the frames
    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        for (unsigned j = i+1; j < m_rbox_segment_vector.size(); j++) {
            CvPoint center1 = m_rbox_segment_vector[i].center;
            CvPoint center2 = m_rbox_segment_vector[j].center;  
            
            if (abs(center1.x-center2.x) < 25 &&
                abs(center1.y-center2.y) < 25 && 
                abs(m_rbox_segment_vector[i].length-m_rbox_segment_vector[j].length) < 40 &&
                abs(m_rbox_segment_vector[i].width-m_rbox_segment_vector[j].width) < 30 &&
                m_rbox_segment_vector[i].color_check(m_rbox_segment_vector[j], 20)
                //m_rbox_segment_vector[i].color_int == m_rbox_segment_vector[j].color_int
            )
            {
                m_rbox_segment_vector[i].shape_merge(m_rbox_segment_vector[j]);
                m_rbox_segment_vector.erase(m_rbox_segment_vector.begin()+j);
                j--;
            }
        }
    }

    // figure out what if there is a color cycling buoy - this should be a buoy of the same position but different colors
    // for each buoy, look at all other buoys, if there are 2 more buoys with diff colors by similar position merge them
    // and flag as MV_UNCOLORED
    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        int num_other_colors = 0;
        for (unsigned j = i+1; j < m_rbox_segment_vector.size(); j++) {
            CvPoint center1 = m_rbox_segment_vector[i].center;
            CvPoint center2 = m_rbox_segment_vector[j].center;  
            
            if (abs(center1.x-center2.x) < 20 &&
                abs(center1.y-center2.y) < 20 &&
                !m_rbox_segment_vector[i].color_check(m_rbox_segment_vector[j], 20)
                )
            {
                m_rbox_segment_vector[i].shape_merge(m_rbox_segment_vector[j]);
                m_rbox_segment_vector.erase(m_rbox_segment_vector.begin()+j);
                j--;
                num_other_colors++;
            }
        }
        if (num_other_colors >= 2) {
            m_rbox_segment_vector[i].color_int = MV_UNCOLORED;
        }
    }

    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        if (m_rbox_segment_vector[i].count < nframes/FRAMES_THRESHOLD_FRACTION) {
            m_rbox_segment_vector.erase(m_rbox_segment_vector.begin()+i);
            i--;
        }
    }

    // sort by count
    std::sort (m_rbox_segment_vector.begin(), m_rbox_segment_vector.end(), shape_count_greater_than);

    // debug
    printf ("Buoy: rbox_calc segments:\n");
    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        printf ("\tSegment %d (%3d,%3d) h=%1.0f,w=%1.0f color=%s  count=%d\n", i, 
            m_rbox_segment_vector[i].center.x, m_rbox_segment_vector[i].center.y,
            m_rbox_segment_vector[i].length, m_rbox_segment_vector[i].width, 
            color_int_to_string(m_rbox_segment_vector[i].color_int).c_str(), m_rbox_segment_vector[i].count);
    }

    // put the best 2 rbox into the result for now
    if (0) {

    }
    else {
        rboxes_returned->clear();
        if (m_rbox_segment_vector.size() > 1 && m_rbox_segment_vector[1].validity >= nframes/FRAMES_THRESHOLD_FRACTION)
            rboxes_returned->push_back(m_rbox_segment_vector[1]);
        if (m_rbox_segment_vector.size() > 0 && m_rbox_segment_vector[0].validity >= nframes/FRAMES_THRESHOLD_FRACTION)
            rboxes_returned->push_back(m_rbox_segment_vector[0]);
    }
}
*/
void MDA_VISION_MODULE_BUOY::circle_calc (MvCircleVector* circles_returned, int nframes) {
    assert (circles_returned != NULL);
    MvCircle input_circle;
    m_circle_segment_vector.clear();
    
    // go thru each frame and pull all individual segments into a vector
    // set i to point to the element 1 past read_index
    int i = read_index + 1;
    if (i >= N_FRAMES_TO_KEEP) i = 0;
    do {
        if (m_frame_data_vector[i].has_data()) {
            if (m_frame_data_vector[i].circle_valid)
                m_circle_segment_vector.push_back(m_frame_data_vector[i].m_frame_circle);
        }
        if (++i >= N_FRAMES_TO_KEEP) i = 0;
    } while (i != read_index);


    // bin the frames
    for (unsigned i = 0; i < m_circle_segment_vector.size(); i++) {
        for (unsigned j = i+1; j < m_circle_segment_vector.size(); j++) {
            CvPoint center1 = m_circle_segment_vector[i].center;
            CvPoint center2 = m_circle_segment_vector[j].center;  
            
            if (abs(center1.x-center2.x) < 25 &&
                abs(center1.y-center2.y) < 25 && 
                abs(m_circle_segment_vector[i].radius-m_circle_segment_vector[j].radius) < 40 &&
                m_circle_segment_vector[i].color_check(m_circle_segment_vector[j], 20)
                //m_circle_segment_vector[i].color_int == m_circle_segment_vector[j].color_int
            )
            {
                m_circle_segment_vector[i].shape_merge(m_circle_segment_vector[j]);
                m_circle_segment_vector.erase(m_circle_segment_vector.begin()+j);
                j--;
            }
        }
    }

    for (unsigned i = 0; i < m_circle_segment_vector.size(); i++) {
        if (m_circle_segment_vector[i].count < nframes/FRAMES_THRESHOLD_FRACTION) {
            m_circle_segment_vector.erase(m_circle_segment_vector.begin()+i);
            i--;
        }
    }

    // sort by count
    std::sort (m_circle_segment_vector.begin(), m_circle_segment_vector.end(), shape_count_greater_than);

    // debug
    printf ("Buoy: circle_calc segments:\n");
    for (unsigned i = 0; i < m_circle_segment_vector.size(); i++) {
        printf ("\tSegment %d (%3d,%3d) R=%1.0f, color=%s  count=%d\n", i, 
            m_circle_segment_vector[i].center.x, m_circle_segment_vector[i].center.y, m_circle_segment_vector[i].radius, 
            color_int_to_string(m_circle_segment_vector[i].color_int).c_str(), m_circle_segment_vector[i].count);
    }

    circles_returned->clear();
    if (m_circle_segment_vector.size() > 0 && m_circle_segment_vector[0].validity >= nframes/FRAMES_THRESHOLD_FRACTION)
        circles_returned->push_back(m_circle_segment_vector[0]);
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_BUOY::frame_calc () {
    /*
    MDA_VISION_RETURN_CODE retval = FATAL_ERROR;
    const int ANGLE_LIMIT = 35;

    // go thru each frame and pull all individual segments into a vector
    // set i to point to the element 1 past read_index
    m_rbox_segment_vector.clear();
    int i = read_index + 1;
    if (i >= N_FRAMES_TO_KEEP) i = 0;
    do {
        if (m_frame_data_vector[i].has_data() && m_frame_data_vector[i].rboxes_valid[0]) {
            m_rbox_segment_vector.push_back(m_frame_data_vector[i].m_frame_boxes[0]);
        }
        if (++i >= N_FRAMES_TO_KEEP) i = 0;
    } while (i != read_index);


    // bin the frames
    for (unsigned i = 0; i < m_rbox_segment_vector.size(); i++) {
        for (unsigned j = i+1; j < m_rbox_segment_vector.size(); j++) {
            CvPoint center1 = m_rbox_segment_vector[i].center;
            CvPoint center2 = m_rbox_segment_vector[j].center;  
            
            if (abs(center1.x-center2.x)+abs(center1.y-center2.y) < 50 && 
                abs(m_rbox_segment_vector[i].length-m_rbox_segment_vector[j].length) < 30 &&
                abs(m_rbox_segment_vector[i].width-m_rbox_segment_vector[j].width) < 20
            )
            {
                m_rbox_segment_vector[i].shape_merge(m_rbox_segment_vector[j]);
                m_rbox_segment_vector.erase(m_rbox_segment_vector.begin()+j);
            }
        }
    }

    // sort by count
    std::sort (m_rbox_segment_vector.begin(), m_rbox_segment_vector.end(), shape_count_greater_than);

    return FULL_DETECT;
    */
    exit (1);
}

bool MDA_VISION_MODULE_BUOY::rbox_stable (int rbox_index, float threshold) {
    assert (rbox_index >= 0 && rbox_index <= 1);
    return true;
}

bool MDA_VISION_MODULE_BUOY::circle_stable (float threshold) {
    return true;
}

void MDA_VISION_MODULE_BASE::print_frames () {
    printf ("\nSAVED FRAMES\n");
    int i = read_index;
    int i2 = 0;
    do {
        printf ("Frame[%-2d]:\t", i2);        
        if (m_frame_data_vector[i].is_valid()) {
            int n_circles = m_frame_data_vector[i].circle_valid?1:0;
            int n_boxes = (m_frame_data_vector[i].rboxes_valid[0]?1:0) + (m_frame_data_vector[i].rboxes_valid[1]?1:0);
            std::string color_str, color_str_2;

            color_str = color_int_to_string(m_frame_data_vector[i].m_frame_circle.color_int);
            printf ("%d Circles (%s)\t", n_circles, (n_circles > 0)?color_str.c_str():"---");

            color_str = color_int_to_string(m_frame_data_vector[i].m_frame_boxes[0].color_int);
            color_str_2 = color_int_to_string(m_frame_data_vector[i].m_frame_boxes[1].color_int);
            printf ("%d Boxes (%s,%s)\n", n_boxes,
                (n_boxes > 0)?color_str.c_str():"---", (n_boxes > 1)?color_str_2.c_str():"---"
                );
        }
        else
            printf ("Invalid\n");

        if (++i >= N_FRAMES_TO_KEEP) i = 0;
        i2++;
    } while (i != read_index && i2 < N_FRAMES_TO_KEEP);
}
