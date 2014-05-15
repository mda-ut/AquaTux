#include "mda_vision.h"

#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

const char MDA_VISION_MODULE_FRAME::MDA_VISION_FRAME_SETTINGS[] = "vision_frame_settings.csv";

/// #########################################################################
/// MODULE_FRAME methods
/// #########################################################################
MDA_VISION_MODULE_FRAME:: MDA_VISION_MODULE_FRAME () :
    window (mvWindow("Frame Vision Module")),
    window2 (mvWindow("Frame Vision Module 2"))//,
{
    N_FRAMES_TO_KEEP = 8;
    gray_img = mvGetScratchImage();
    gray_img_2 = mvGetScratchImage2();
}

MDA_VISION_MODULE_FRAME:: ~MDA_VISION_MODULE_FRAME () {
    mvReleaseScratchImage();
    mvReleaseScratchImage2();
}

void MDA_VISION_MODULE_FRAME::primary_filter (IplImage* src) {
    // shift the frames back by 1
    shift_frame_data (m_frame_data_vector, read_index, N_FRAMES_TO_KEEP);

    // HSV hack!
    /*unsigned char *srcptr;
    int zeros = 0;
    for (int i = 0; i < src->height; i++) {
        srcptr = (unsigned char*)src->imageData + i*src->widthStep;
        for (int j = 0; j < src->width; j++) {
            if (srcptr[1] < 90) {
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
    }*/

    watershed_filter.watershed(src, gray_img, mvWatershedFilter::WATERSHED_STEP_SMALL);
    window.showImage (src);
}

MDA_VISION_RETURN_CODE MDA_VISION_MODULE_FRAME::calc_vci () {
    MDA_VISION_RETURN_CODE retval = NO_TARGET;
    COLOR_TRIPLE color;
    int H,S,V;
    MvRotatedBox rbox;
    MvRBoxVector rbox_vector;

    while ( watershed_filter.get_next_watershed_segment(gray_img_2, color) ) {
        // check that the segment is roughly red
        tripletBGR2HSV (color.m1,color.m2,color.m3, H,S,V);
        if (S < 10 || V < 30 || color.m2 < 40 || color.m1 > 80/*|| !(H >= 160 || H < 130)*/) {
            DEBUG_PRINT ("VISION_FRAME: rejected rectangle due to color: HSV=(%3d,%3d,%3d)\n", H,S,V);
            continue;
        }

        contour_filter.match_rectangle(gray_img_2, &rbox_vector, color, 5.0, 40.0, 1);
        //window2.showImage (gray_img_2);
    }

    // debug
    for (unsigned i = 0; i < rbox_vector.size(); i++) {
        rbox_vector[i].drawOntoImage(gray_img);
    }
    window2.showImage (gray_img);

    if (rbox_vector.size() < 2) { // not enough good segments, return no target
        printf ("Frame: No Target\n");
        return NO_TARGET;
    }
    else if (rbox_vector.size() == 2) {
        MvRotatedBox rbox0 = rbox_vector[0];
        MvRotatedBox rbox1 = rbox_vector[1];
        float height = -1;
        float width  = -1;

        if(abs(rbox0.angle) > 60 && abs(rbox1.angle) > 60){
            printf("Frame Sanity Failure: 2 Horizontal Lines?????\n");
            return NO_TARGET;
        }
        else if(abs(rbox0.angle) > 30 && abs(rbox1.angle) < 30){
            printf ("Frame: 2 segments, rbox0 horiz\n");
            // 0 is horiz
            m_pixel_x = rbox0.center.x - gray_img->width/2;
            m_pixel_y = rbox1.center.y - gray_img->height/2;
            width  = rbox0.length;
            height = rbox1.length;

            retval = FULL_DETECT;
        }
        else if(abs(rbox0.angle) < 30 && abs(rbox1.angle) > 60){
            printf ("Frame: 2 segments, rbox1 horiz\n");
            // 1 is horiz
            m_pixel_x = rbox1.center.x - gray_img->width/2;
            m_pixel_y = rbox0.center.y - gray_img->height/2;
            width  = rbox1.length;
            height = rbox0.length;

            retval = FULL_DETECT;
        }
        else if(abs(rbox0.angle) < 30 && abs(rbox1.angle) < 30){
            printf ("Frame: 2 segments, both vertical\n");
            // 2 vertical
            m_pixel_x = (rbox0.center.x + rbox1.center.x - gray_img->width) / 2;
            m_pixel_y = (rbox0.center.y + rbox1.center.y - gray_img->height) / 2;
            width  = abs(rbox0.center.x - rbox1.center.x);
            height = (rbox0.length + rbox1.length) / 2;

            retval = FULL_DETECT;
        }
        else {
            return NO_TARGET;
        }

        m_range = (float)(FRAME_REAL_WIDTH * gray_img->width / width * TAN_FOV_X);
    }
    else if (rbox_vector.size() == 3) {
        MvRotatedBox rbox0 = rbox_vector[0];
        MvRotatedBox rbox1 = rbox_vector[1];
        MvRotatedBox rbox2 = rbox_vector[2];
        float height = -1;
        float width  = -1;

        if(abs(rbox0.angle) > 60 && abs(rbox1.angle) < 30 && abs(rbox2.angle) < 30) {
            printf ("Frame: 3 segments, rbox0 horiz\n");
            m_pixel_x = rbox0.center.x - gray_img->width/2;
            m_pixel_y = (rbox1.center.y + rbox2.center.y - gray_img->height) / 2;
            width  = abs(rbox1.center.x - rbox2.center.x);
            height = (rbox1.length + rbox2.length) / 2;

            retval = FULL_DETECT;
        }
        else if(abs(rbox0.angle) < 30 && abs(rbox1.angle) > 60 && abs(rbox2.angle) < 30) {
            printf ("Frame: 3 segments, rbox1 horiz\n");
            m_pixel_x = rbox1.center.x - gray_img->width/2;
            m_pixel_y = (rbox0.center.y + rbox2.center.y - gray_img->height) / 2;
            width  = abs(rbox0.center.x - rbox2.center.x);
            height = (rbox0.length + rbox2.length) / 2;

            retval = FULL_DETECT;
        }
        else if(abs(rbox0.angle) < 30 && abs(rbox1.angle) < 30 && abs(rbox2.angle) > 60) {
            printf ("Frame: 3 segments, rbox2 horiz\n");
            m_pixel_x = rbox2.center.x - gray_img->width/2;
            m_pixel_y = (rbox0.center.y + rbox1.center.y - gray_img->height) / 2;
            width  = abs(rbox0.center.x - rbox1.center.x);
            height = (rbox0.length + rbox1.length) / 2;

            retval = FULL_DETECT;
        }
        else{
            return NO_TARGET;
        }

        m_range = (float)(FRAME_REAL_WIDTH * gray_img->width / width * TAN_FOV_X);
    }
    else {
        return NO_TARGET;
    }

    m_angular_x = RAD_TO_DEG * atan(TAN_FOV_X * m_pixel_x / gray_img->width);
    m_angular_y = RAD_TO_DEG * atan(TAN_FOV_Y * m_pixel_y / gray_img->height);
    DEBUG_PRINT ("Frame: (%d,%d) (%5.2f, %5.2f)  range=%d\n", m_pixel_x, m_pixel_y, 
        m_angular_x, m_angular_y, m_range);
    return retval;
}
/*
MDA_VISION_RETURN_CODE MDA_VISION_MODULE_FRAME:: calc_vci () {
    MDA_VISION_RETURN_CODE retval = FATAL_ERROR;
    unsigned nClusters = KMeans.nClusters();
    int imWidth  =  (int)filtered_img->width*0.5;
    int imHeight =  (int)filtered_img->height*0.5;

    // for IsRed, we check any vertical segments we find to see if they are red
    // We look near the centroid of the vert. segment, and check that the pixels are set 
    // to value MV_RED, which is what the filter will set red pixels to in greyscale images
    IsRed = -1;

    if(nClusters == 0) {
        printf ("Frame: No clusters =(\n");
        return NO_TARGET;
    }
    else if(nClusters == 1){
        DEBUG_PRINT("Frame: 1 Cluster...\n");
        int x1 = KMeans[0][0].x, x2 = KMeans[0][1].x;
        int y1 = KMeans[0][0].y, y2 = KMeans[0][1].y;    

        float denom = (x1==x2) ? 1 : abs(x2-x1);
        double slope = ((double)y1-y2)/denom;

        if (abs(slope) < 0.2) { // single horizontal line
            m_pixel_x = (int)(x1+x2)*0.5 - imWidth;
            m_pixel_y = (int)(y1+y2)*0.5 - imHeight;
            m_range = ((float)(FRAME_REAL_WIDTH) * filtered_img->width) / ((x2-x1) * TAN_FOV_X);

            // displace returned y coordinate upwards by half the frame height 
            m_pixel_y -= (int) (abs(x2-x1) / FRAME_REAL_WIDTH * FRAME_REAL_HEIGHT * 0.5); 
            
            retval = ONE_SEGMENT;
            goto RETURN_CENTROID;
        }
        else if (abs(slope) > 6) { // single vertical line
            int centroid_x = (int)(x1+x2)*0.5;
            int centroid_y = (int)(y1+y2)*0.5;  

            m_pixel_x = centroid_x - imWidth;
            m_pixel_y = centroid_y - imHeight;
            
            // displace returned y coordinate upwards by half the frame height 
            m_pixel_y -= (int) (abs(x2-x1) / FRAME_REAL_WIDTH * FRAME_REAL_HEIGHT * 0.5);

            retval = ONE_SEGMENT;
            goto RETURN_CENTROID;
        }
        else { // unknown - raise error and return centroid
            retval = UNKNOWN_TARGET;
            goto RETURN_CENTROID;
        }
    }
    else if(nClusters == 2){
        DEBUG_PRINT ("Frame: 2 clusters...\n");
        int x00 = KMeans[0][0].x,   y00 = KMeans[0][0].y;
        int x01 = KMeans[0][1].x,   y01 = KMeans[0][1].y;
        int x10 = KMeans[1][0].x,   y10 = KMeans[1][0].y;
        int x11 = KMeans[1][1].x,   y11 = KMeans[1][1].y;

        //float height0 = abs(y01 - y00); // height of first 1ine
        //float height1 = abs(y11 - y10); // height of second line
        float height = -1;
        float width  = -1;

        float denom0 = (x00==x01) ? 1 : abs(x00-x01);
        double slope0 = ((double)y00-y01)/denom0;

        float denom1 = (x10==x11) ? 1 : abs(x10-x11);
        double slope1 = ((double)y10-y11)/denom1;

        if(abs(slope0) < 0.2 && abs(slope1) < 0.2){
            printf("Frame Sanity Failure: 2 Horizontal Lines?????\n");
            m_pixel_x = (int)(x00+x01+x10+x11)*0.25 - imWidth;
            m_pixel_y = (int)(y00+y01+y10+y11)*0.25 - imHeight;
            width = (int)(abs(x00-x01) + abs(x10-x11))*0.25;
            retval = UNKNOWN_TARGET;
        }
        // RZ: for some reason here Y coordinates go from 0(top) to 300(bottom), so we actually want MIN for the vertical segments
        else if(abs(slope0) < 0.2 && abs(slope1) > 6){
            //printf("1 of Each\n");
            m_pixel_x = (int)(x00 + x01)*0.5 - imWidth;
            m_pixel_y = (int)(2*MIN(y10,y11)+y00+y01)/4 - imHeight;
            width  = (int)abs(x00-x01);
            height = (int)abs(y10-y11);

            // check for red segment
            unsigned char* centroid_pixel = (unsigned char*)(filtered_img->imageData + (y10+y11)/2*filtered_img->widthStep + (x10+x11)/2);
            IsRed = check_pixel_is_color (centroid_pixel, MV_RED);

            retval = ONE_SEGMENT;  //Need other retval here
        }
        else if(abs(slope0) > 6 && abs(slope1) < 0.2){
            //printf("1 of Each\n");
            m_pixel_x = (int)(x10 + x11)*0.5 - imWidth;
            m_pixel_y = (int)(2*MIN(y00,y01)+y10+y11)/4 - imHeight;
            width  = (int)abs(x10-x11);
            height = (int)abs(y00-y01);
            
            // check for red segment
            unsigned char* centroid_pixel = (unsigned char*)(filtered_img->imageData + (y00+y01)/2*filtered_img->widthStep + (x00+x01)/2);
            IsRed = check_pixel_is_color (centroid_pixel, MV_RED);

            retval = ONE_SEGMENT;
        }
        else if(abs(slope0) > 6 && abs(slope1) > 6){
            //printf("2 Vertical Lines\n");
            m_pixel_x = (int)(x00+x01+x10+x11)*0.25 - imWidth;
            m_pixel_y = (int)(y00+y01+y10+y11)*0.25 - imHeight;
            width  = (int)(abs(x00-x11) + abs(x10-x11))*0.5;
            height = (int)(abs(y00-y11) + abs(y10-y11))*0.5;

            // check both segments for red segment
            unsigned char* centroid_pixel = (unsigned char*)(filtered_img->imageData + (y00+y01)/2*filtered_img->widthStep + (x00+x01)/2);
            unsigned char* centroid_pixel2 = (unsigned char*)(filtered_img->imageData + (y10+y11)/2*filtered_img->widthStep + (x10+x11)/2); 
            IsRed = check_pixel_is_color (centroid_pixel, MV_RED) | check_pixel_is_color (centroid_pixel2, MV_RED);

            retval = ONE_SEGMENT;
        }
        else{
            DEBUG_PRINT("Frame Sanity Failure: Lines Incorrectly Oriented\n");  
            retval = UNKNOWN_TARGET;   
            goto RETURN_CENTROID;   
        }

        m_range = (float)(FRAME_REAL_WIDTH * filtered_img->width / width * TAN_FOV_X);
    }

    else if(nClusters == 3){
        DEBUG_PRINT ("Frame: 3 clusters\n");
        int x00 = KMeans[0][0].x,   y00 = KMeans[0][0].y;
        int x01 = KMeans[0][1].x,   y01 = KMeans[0][1].y;
        int x10 = KMeans[1][0].x,   y10 = KMeans[1][0].y;
        int x11 = KMeans[1][1].x,   y11 = KMeans[1][1].y;
        int x20 = KMeans[2][0].x,   y20 = KMeans[2][0].y;
        int x21 = KMeans[2][1].x,   y21 = KMeans[2][1].y;

        float height = -1;
        float width  = -1;

        float denom0 = (x00==x01) ? 1 : abs(x00-x01);
        double slope0 = ((double)y00-y01)/denom0;

        float denom1 = (x10==x11) ? 1 : abs(x10-x11);
        double slope1 = ((double)y10-y11)/denom1;

        float denom2 = (x20==x21) ? 1 : abs(x20-x21);
        double slope2 = ((double)y20-y21)/denom2;

        if(abs(slope0) < 0.2 && abs(slope1) > 6 && abs(slope2) > 6){
            m_pixel_x = (int)(x00+x01+x10+x11+x20+x21)*0.167 - imWidth;
            m_pixel_y = (int)(MIN(y10,y11)+MIN(y20,y21)+y00+y01)*0.25 - imHeight;
            width  = (int)(abs(x00-x01)+abs((x10+x11)-(x20+x21)))*0.333;
            height = (int)(-1*y00-y01+MIN(y10,y11)+MIN(y20,y21))*0.5;

            // check both vert segments for red segment
            unsigned char* centroid_pixel = (unsigned char*)(filtered_img->imageData + (y10+y11)/2*filtered_img->widthStep + (x10+x11)/2); 
            unsigned char* centroid_pixel2 = (unsigned char*)(filtered_img->imageData + (y20+y21)/2*filtered_img->widthStep + (x20+x21)/2);
            IsRed = check_pixel_is_color (centroid_pixel, MV_RED) | check_pixel_is_color (centroid_pixel2, MV_RED);

            retval = FULL_DETECT;
        }
        else if(abs(slope0) > 6 && abs(slope1) < 0.2 && abs(slope2) > 6){
            m_pixel_x = (int)(x00+x01+x10+x11+x20+x21)*0.167 - imWidth;
            m_pixel_y = (int)(MIN(y00,y01)+MIN(y20,y21)+y10+y11)*0.25 - imHeight;
            width  = (int)(abs(x10-x11)+abs((x00+x01)-(x20+x21)))*0.333;
            height = (int)(-1*y10-y11+MIN(y00,y01)+MIN(y20,y21))*0.5;

            // check both vert segments for red segment
            unsigned char* centroid_pixel = (unsigned char*)(filtered_img->imageData + (y00+y01)/2*filtered_img->widthStep + (x00+x01)/2); 
            unsigned char* centroid_pixel2 = (unsigned char*)(filtered_img->imageData + (y20+y21)/2*filtered_img->widthStep + (x20+x21)/2);
            IsRed = check_pixel_is_color (centroid_pixel, MV_RED) | check_pixel_is_color (centroid_pixel2, MV_RED);

            retval = FULL_DETECT;
        }
        else if(abs(slope0) > 6 && abs(slope1) > 6 && abs(slope2) < 0.2){
            m_pixel_x = (int)(x00+x01+x10+x11+x20+x21)*0.167 - imWidth;
            m_pixel_y = (int)(MIN(y10,y11)+MIN(y00,y01)+y20+y21)*0.25 - imHeight;
            width  = (int)(abs(x20-x21)+abs((x10+x11)-(x00+x01)))*0.333;
            height = (int)(-1*y20-y21+MIN(y10,y11)+MIN(y00,y01))*0.5;
        
            // check both vert segments for red segment
            unsigned char* centroid_pixel = (unsigned char*)(filtered_img->imageData + (y00+y01)/2*filtered_img->widthStep + (x00+x01)/2); 
            unsigned char* centroid_pixel2 = (unsigned char*)(filtered_img->imageData + (y10+y11)/2*filtered_img->widthStep + (x10+x11)/2);
            IsRed = check_pixel_is_color (centroid_pixel, MV_RED) | check_pixel_is_color (centroid_pixel2, MV_RED);

            retval = FULL_DETECT;
        }
        else{
            DEBUG_PRINT("Frame Sanity Failure: Incorrect line arrangement\n");
            m_pixel_x = (int)(x00+x01+x10+x11+x20+x21)*0.167 -imWidth;
            m_pixel_y = (int)(y00+y01+y10+y11+y20+y21)*0.167 - imHeight;
            retval = UNKNOWN_TARGET;
            goto RETURN_CENTROID;
        }

        m_range = (float)(FRAME_REAL_WIDTH * filtered_img->width / width * TAN_FOV_X);
    }

    else{
        printf("nClusters = %d unhandled\n", nClusters);
        return NO_TARGET;
    }

    // this code has to be here since we check filter_image values inside this function
    lines.drawOntoImage (filtered_img);
    KMeans.drawOntoImage (filtered_img);
    window.showImage (filtered_img);

    /// if we encounter any sort of sanity error, we will return only the centroid
    RETURN_CENTROID:
        m_angular_x = RAD_TO_DEG * atan(TAN_FOV_X * m_pixel_x / filtered_img->width);
        m_angular_y = RAD_TO_DEG * atan(TAN_FOV_Y * m_pixel_y / filtered_img->height);
        DEBUG_PRINT ("Frame: (%d,%d) (%5.2f, %5.2f)\n", m_pixel_x, m_pixel_y, 
            m_angular_x, m_angular_y); 

        switch (IsRed) {
            case -1: DEBUG_PRINT ("Frame red signal: Unsure\n"); break;
            case 0: DEBUG_PRINT ("Frame red signal = No\n"); break;
            case 1: DEBUG_PRINT ("Frame red signal = Yes\n"); break;
            default: printf ("Unhandled value of IsRed in vision_frame.\n"); exit(1);
        }
        return retval;
}
*/
