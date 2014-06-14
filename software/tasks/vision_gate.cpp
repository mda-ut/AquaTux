#include "mda_vision.h"

//#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

const char MDA_VISION_MODULE_GATE::MDA_VISION_GATE_SETTINGS[] = "vision_gate_settings.csv";

/// ########################################################################
/// MODULE_GATE methods
/// ########################################################################
MDA_VISION_MODULE_GATE:: MDA_VISION_MODULE_GATE () :
	window (mvWindow("Gate Vision Module")),
    window2 (mvWindow("Gate Vision Module 2"))//,
	//HoughLines (mvHoughLines(MDA_VISION_GATE_SETTINGS)),
	//lines (mvLines())
{
    N_FRAMES_TO_KEEP = 8;
    gray_img = mvGetScratchImage();
    gray_img_2 = mvGetScratchImage2();
}

MDA_VISION_MODULE_GATE:: ~MDA_VISION_MODULE_GATE () {
    mvReleaseScratchImage();
    mvReleaseScratchImage2();
}

void MDA_VISION_MODULE_GATE::add_frame (IplImage* src) {
    // shift the frames back by 1
    shift_frame_data (m_frame_data_vector, read_index, N_FRAMES_TO_KEEP);

    watershed_filter.watershed(src, gray_img, mvWatershedFilter::WATERSHED_STEP_SMALL);
    window.showImage (src);

    COLOR_TRIPLE color;
    int H,S,V;
    MvRotatedBox rbox;
    MvRBoxVector rbox_vector;

    //float length_to_width = 1.d/GATE_REAL_SLENDERNESS;

    while ( watershed_filter.get_next_watershed_segment(gray_img_2, color) ) {
        // check that the segment is roughly red
        tripletBGR2HSV (color.m1,color.m2,color.m3, H,S,V);
        if (S < 20 || V < 30 || !(H >= 160 || H < 130)) {
            DEBUG_PRINT ("VISION_BUOY: rejected rectangle due to color: HSV=(%3d,%3d,%3d)\n", H,S,V);
            continue;
        }

        contour_filter.match_rectangle(gray_img_2, &rbox_vector, color, 5.0, 15.0, 1);
        //window2.showImage (gray_img_2);
    }

    // debug only
    cvCopy (gray_img, gray_img_2);

    if (rbox_vector.size() > 0) {
        MvRBoxVector::iterator iter = rbox_vector.begin();
        MvRBoxVector::iterator iter_end = rbox_vector.end();
        
        // this stores the rects with 2 best validity
        for (; iter != iter_end; ++iter) {
            if (abs(iter->angle) < 20) {
                m_frame_data_vector[read_index].assign_rbox_by_validity(*iter);
            }
        }
    }

    //m_frame_data_vector[read_index].sort_rbox_by_x();

    if (m_frame_data_vector[read_index].is_valid()) {
        m_frame_data_vector[read_index].drawOntoImage(gray_img_2);
        window2.showImage (gray_img_2);
    }

    //print_frames();
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_GATE::frame_calc () {

    MDA_VISION_RETURN_CODE retval = FATAL_ERROR;
    const int ANGLE_LIMIT = 25;

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
            
            if (abs(center1.x-center2.x)+abs(center1.y-center2.y) < 50)
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
        printf ("\tSegment %d (%3d,%3d) height=%3.0f, width=%3.0f   count=%d\n", i, segment_vector[i].center.x, segment_vector[i].center.y,
            segment_vector[i].length, segment_vector[i].width, segment_vector[i].count);
    }

    if (segment_vector.size() == 0 || segment_vector[0].count < 3) { // not enough good segments, return no target
        printf ("Gate: No Target\n");
        return NO_TARGET;
    }
    else if (segment_vector.size() >= 2 && segment_vector[1].count >= 3) { // full detect, return both segments
        int gate_pixel_height = (segment_vector[0].length + segment_vector[1].length) / 2;
        int gate_pixel_width = abs(segment_vector[0].center.x - segment_vector[1].center.x);
        float gate_width_to_height_ratio = abs(static_cast<float>(gate_pixel_width)/gate_pixel_height);

        // check segment is vertical
        if (abs(segment_vector[0].angle) > ANGLE_LIMIT || abs(segment_vector[1].angle) > ANGLE_LIMIT) {
            DEBUG_PRINT("Full Detect: angle outside limit\n");
            return NO_TARGET;            
        }
        // check length similarity
        if (segment_vector[0].length > 1.3*segment_vector[1].length || 1.3*segment_vector[0].length < segment_vector[1].length) {
            DEBUG_PRINT("Full Detect: similarity check failed\n");
            return NO_TARGET;            
        }
        if (gate_width_to_height_ratio > 1.3*GATE_WIDTH_TO_HEIGHT_RATIO || 1.3*gate_width_to_height_ratio < GATE_WIDTH_TO_HEIGHT_RATIO) {
            DEBUG_PRINT("Full Detect: gate width to height check failed\n");
            return NO_TARGET  ; 
        }

        m_pixel_x = (segment_vector[0].center.x + segment_vector[1].center.x) / 2;
        m_pixel_y = (segment_vector[0].center.y + segment_vector[1].center.y) / 2;
        m_range = (GATE_REAL_HEIGHT * gray_img->height) / (gate_pixel_height * TAN_FOV_Y);

        printf ("Gate: FULL_DETECT\n");
        retval = FULL_DETECT;
    }
    else if (segment_vector.size() >= 1 && segment_vector[0].count >= 3) { // first segment is good enough, use that only
        printf ("Gate: ONE_SEGMENT\n");
        int gate_pixel_height = segment_vector[0].length;
        
        // check segment is vertical
        if (abs(segment_vector[0].angle) > ANGLE_LIMIT) {
            DEBUG_PRINT("One Segment: angle outside limit\n");
            return NO_TARGET;            
        }

        m_pixel_x = segment_vector[0].center.x;
        m_pixel_y = segment_vector[0].center.x;
        m_range = (GATE_REAL_HEIGHT * gray_img->height) / (gate_pixel_height * TAN_FOV_Y);

        retval = ONE_SEGMENT;
    }
    else {
	return NO_TARGET;
    }

#ifdef M_DEBUG
    for (unsigned i = 0; i<=1 && i<segment_vector.size(); i++)
        segment_vector[i].drawOntoImage(gray_img);
      window2.showImage(gray_img);
#endif

    m_pixel_x -= gray_img->width/2;
    m_pixel_y -= gray_img->height/2;
    m_angular_x = RAD_TO_DEG * atan(TAN_FOV_X * m_pixel_x / gray_img->width);
    m_angular_y = RAD_TO_DEG * atan(TAN_FOV_Y * m_pixel_y / gray_img->height);
    printf ("Gate (%3d,%3d) (%5.2f,%5.2f) range=%d\n", m_pixel_x, m_pixel_y, m_angular_x, m_angular_y, m_range);
    return retval;
}
