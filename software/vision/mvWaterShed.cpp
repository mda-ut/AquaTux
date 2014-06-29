#include "mvColorFilter.h"
#include <math.h>

// Contains functions for mvWatershedFilter that pertain to the watershed algorithm

//#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

//#define SEED_IMAGE_RANDOM 
#define USE_AVERAGE_INTER_CLUSTER_DIST

// used in sorting color difference color triples
bool m3_less_than (COLOR_TRIPLE T1, COLOR_TRIPLE T2) {
    return T1.m3 < T2.m3;
}

// ########################################################################################

mvWatershedFilter::mvWatershedFilter() :
    bin_SeedGen ("mvWatershed - SeedGen"),
    bin_SeedPlace ("mvWatershed - SeedPlace"),
    bin_Filter ("mvWatershed - Filter")
{
    scratch_image_3c = mvCreateImage_Color();
    scratch_image = mvCreateImage();
    ds_image_nonedge = cvCreateImage (
        cvSize(scratch_image_3c->width/WATERSHED_DS_FACTOR, scratch_image_3c->height/WATERSHED_DS_FACTOR),
        IPL_DEPTH_8U,
        1
    );
    marker_img_32s = cvCreateImage(
        cvGetSize(scratch_image_3c),
        IPL_DEPTH_32S,
        1
    );

    kernel = cvCreateStructuringElementEx (KERNEL_WIDTH, KERNEL_HEIGHT, (KERNEL_WIDTH+1)/2, (KERNEL_HEIGHT+1)/2, CV_SHAPE_ELLIPSE);


    srand(time(NULL));
}

mvWatershedFilter::~mvWatershedFilter() {
    cvReleaseImage(&scratch_image_3c);
    cvReleaseImage(&scratch_image);
    cvReleaseImage(&ds_image_nonedge);
    cvReleaseImage(&marker_img_32s);
}

void mvWatershedFilter::watershed(IplImage* src, IplImage* dst, int method) {
// attemps to use cvWaterShed to segment the image
    assert (src->nChannels == 3);
    assert (dst->nChannels == 1);
    
    segment_color_hash.clear();
    color_point_vector.clear();

    cvCopy (src, scratch_image_3c);

    bin_SeedGen.start();
    watershed_generate_markers_internal(src, method);
    bin_SeedGen.stop();

    bin_SeedPlace.start();
    //watershed_process_markers_internal();
    watershed_process_markers_internal2();
    watershed_place_markers_internal(src);
    bin_SeedPlace.stop();

    bin_Filter.start();
    watershed_filter_internal(src,dst);
    bin_Filter.stop();

    // return curr_segment_iter to beginning
    curr_segment_iter = segment_color_hash.begin();
    curr_segment_index = 0;
}

void mvWatershedFilter::watershed_generate_markers_internal (IplImage* src, int method, std::vector<CvPoint>* seed_vector) {
// This function generates a bunch of markers and puts them into color_point_vector
    // massively downsample - this smoothes the image
    cvCvtColor (src, scratch_image, CV_BGR2GRAY);
    /*cvResize (scratch_image, ds_image_nonedge, CV_INTER_LINEAR);
      
    // generate the "nonedge image" which is 1 if the pixel isnt an edge image in ds_image_3c
    cvSmooth (ds_image_nonedge, ds_image_nonedge, CV_GAUSSIAN, 5);

    // perform gradient
    IplImage *ds_scratch = cvCreateImageHeader (cvGetSize(ds_image_nonedge), IPL_DEPTH_8U, 1);
    ds_scratch->imageData = scratch_image->imageData;
    cvMorphologyEx (ds_image_nonedge, ds_image_nonedge, ds_scratch, kernel, CV_MOP_GRADIENT);
    cvReleaseImageHeader (&ds_scratch);

    CvScalar mean, stdev;
    cvAvgSdv (ds_image_nonedge, &mean, &stdev);
    cvThreshold (ds_image_nonedge, ds_image_nonedge, mean.val[0]+2*stdev.val[0], 255, CV_THRESH_BINARY);
    cvErode (ds_image_nonedge, ds_image_nonedge, kernel);
    cvNot (ds_image_nonedge, ds_image_nonedge);

    // draw the bad pixels on the image so we can see them
    cvResize (ds_image_nonedge, scratch_image, CV_INTER_NN);
    for (int i = 0; i < scratch_image->height; i++) {
        unsigned char* srcPtr = (unsigned char*)(scratch_image->imageData + i*scratch_image->widthStep);
        unsigned char* dstPtr = (unsigned char*)(src->imageData + i*src->widthStep);

        for (int j = 0; j < scratch_image->width; j++) {        
            if (*srcPtr == 0) {
                dstPtr[0] = 0;
                dstPtr[1] = 0;
                dstPtr[2] = 200;
            }
            srcPtr ++;
            dstPtr += 3;
        }
    }
    */

    if (method & WATERSHED_SAMPLE_RANDOM) {
        cvSet (ds_image_nonedge, CV_RGB(1,1,1));
        // sample the image like this
        // 1. randomly generate an x,y coordinate.
        // 2. Check if the coordinate is a non-edge pixel on the nonedge image.
        // 3. If so add it to color_point_vector and
        // 4. If so mark coordinates near it as edge on the nonege image
        for (int i = 0; i < 200; i++) {
            int x = rand() % ds_image_nonedge->width;
            int y = rand() % ds_image_nonedge->height;

            unsigned char nonedge = *((unsigned char*)ds_image_nonedge->imageData + y*ds_image_nonedge->widthStep + x);
            if (nonedge != 0) {
                // calculate corresponding large image coords
                int xl = x * WATERSHED_DS_FACTOR;
                int yl = y * WATERSHED_DS_FACTOR;
                // 3.
                unsigned char* colorPtr = (unsigned char*)src->imageData + yl*src->widthStep + 3*xl;
                COLOR_TRIPLE ct (colorPtr[0], colorPtr[1], colorPtr[2], 0);;
                color_point_vector.push_back(std::make_pair(ct, cvPoint(xl,yl)));
                // 4.
                cvCircle (ds_image_nonedge, cvPoint(x,y), 10, CV_RGB(0,0,0), -1);          
            }
        }
    }
    else {
        int step = 10;
        if (method & WATERSHED_STEP_SMALL) step = 5;

        COLOR_TRIPLE ct_prev;

        for (int y = step/2; y < src->height; y += step) {
            unsigned char* colorPtr = (unsigned char*)src->imageData + y*src->widthStep + 3*step/2;

            for (int x = step/2; x < src->width; x += step) {
                COLOR_TRIPLE ct (colorPtr[0], colorPtr[1], colorPtr[2], 0);
                
                if (ct.diff(ct_prev) >= 20) {
                    color_point_vector.push_back(std::make_pair(ct, cvPoint(x,y)));
                    ct_prev = ct;
                    //x += step;
                    //colorPtr += 3*step;
                }

                colorPtr += 3*step;
            }
        }
    }

    int diff_limit = 30;
    // the color point vector will have too many pixels that are really similar - get rid of some by merging    
    for (unsigned i = 0; i < color_point_vector.size(); i++) {
        for (unsigned j = i+1; j < color_point_vector.size(); j++) {
            int dx = color_point_vector[i].second.x - color_point_vector[j].second.x;
            int dy = color_point_vector[i].second.y - color_point_vector[j].second.y;  
            
            if (color_point_vector[i].first.diff(color_point_vector[j].first) < diff_limit && dx*dx + dy*dy < 10000)
            {
                if (rand() % 2 == 0) {
                    color_point_vector[i].first.merge(color_point_vector[j].first);
                    color_point_vector.erase(color_point_vector.begin()+j);
                    j--;
                }
                else {
                    color_point_vector[j].first.merge(color_point_vector[i].first);
                    color_point_vector.erase(color_point_vector.begin()+i);
                    i--;
                    break;    
                }
            }
        }
    }
}


void mvWatershedFilter::watershed_place_markers_internal (IplImage* src) {
    // clear segment_color_hash, then populate the hash with the needed triples
    int num_pixels = color_point_vector.size();

    for (int i = 0; i < num_pixels; i++) {
        int index_number = color_point_vector[i].first.index_number;
        
        if (index_number != 0) {
            unsigned char index_char = static_cast<unsigned char>(index_number);
            COLOR_TRIPLE T (0,0,0,index_number);
            segment_color_hash.insert(std::make_pair(index_char,T));
        }
    }

    // zero marker image and draw markers onto it
    // also draw marker positions onto src so we can see where the markers are
    cvZero (marker_img_32s);
    //DEBUG_PRINT ("Markers:\n");
    for (int i = 0; i < num_pixels; i++) {
        COLOR_TRIPLE ct = color_point_vector[i].first;
        CvPoint C = color_point_vector[i].second;
        int x = C.x;
        int y = C.y;

        if (ct.index_number != 0) {
            int *ptr = &CV_IMAGE_ELEM(marker_img_32s, int, y, x);
            *ptr = static_cast<int>(ct.index_number);

            cvCircle(src, cvPoint(x,y), 2, CV_RGB(200,0,200), -1);
            //DEBUG_PRINT ("\tmarker: location <%3d,%3d>: color (%3d,%3d,%3d) - %2d\n", x, y, ct.m1, ct.m2, ct.m3, ct.index_number);
        }
    }
}

void mvWatershedFilter::watershed_process_markers_internal () {
    int num_pixels = color_point_vector.size();
    assert (num_pixels > 0);
    DEBUG_PRINT ("Candidate Number of Pixels for Markers = %d\n", num_pixels);

    // go thru each pair of pixels and calculate their color difference and add it to a vector
    // the pixels are represented by their indices in the color_point_vector
    // we'll use a vector of triples - m1 = index1, m2 = index2, m3 = diff
    COLOR_TRIPLE_VECTOR pair_difference_vector;
    for (int i = 0; i < num_pixels; i++) {
        for (int j = i+1; j < num_pixels; j++) {
            int dx = color_point_vector[i].second.x - color_point_vector[j].second.x;
            int dy = color_point_vector[i].second.y - color_point_vector[j].second.y;  

            if (dx*dx + dy+dy < scratch_image->width*scratch_image->width/16) {
                int diff = color_point_vector[i].first.diff(color_point_vector[j].first);
                COLOR_TRIPLE ct (i, j, diff, 0);

                pair_difference_vector.push_back(ct);
            }
        }
    }

    // now sort the pair_difference_vector
    std::sort(pair_difference_vector.begin(), pair_difference_vector.end(), m3_less_than);

    // assign index numbers
    DEBUG_PRINT ("Diffs:\n");
    final_index_number = 4;
    int limit = pair_difference_vector.size();
    for (int i = 0; i < limit; i++) {
        const int index1 = pair_difference_vector[i].m1;
        const int index2 = pair_difference_vector[i].m2;
        const int diff = pair_difference_vector[i].m3;
        // Print a histogram like graphic for the sorted diffs
        /*printf ("\tDiff (%3d,%3d) = %5d\t", index1, index2, diff);
        for (int j = 0; j < diff; j+=200)
            printf ("#");
        printf ("\n");
        */
        if (diff > 60 || final_index_number > MAX_INDEX_NUMBER)
            break;

        const bool pixel1_unassigned = (color_point_vector[index1].first.index_number == 0);
        const bool pixel2_unassigned = (color_point_vector[index2].first.index_number == 0);

        // if neither pixel has been assigned a number
        if (pixel1_unassigned && pixel2_unassigned) {
            // assign both pixels the next number
            color_point_vector[index1].first.index_number = final_index_number;
            color_point_vector[index2].first.index_number = final_index_number;
            final_index_number += 5;
        }
        // if first has been assigned a number
        else if (!pixel1_unassigned && pixel2_unassigned) {
            // assign second the same number
            color_point_vector[index2].first.index_number = color_point_vector[index1].first.index_number;
        }
        else if (pixel1_unassigned && !pixel2_unassigned) {
            color_point_vector[index1].first.index_number = color_point_vector[index2].first.index_number;
        }
        // both have numbers
        else {
            // go thru entire color_point_vector. Every pixel with the second index gets the first index instead
            const unsigned index_to_find = color_point_vector[index2].first.index_number;
            const unsigned index_to_set = color_point_vector[index1].first.index_number;
            for (int j = 0; j < num_pixels; j++) 
                if (color_point_vector[j].first.index_number == index_to_find)
                    color_point_vector[j].first.index_number = index_to_set;
        }
    }
}

void mvWatershedFilter::watershed_process_markers_internal2 () {
    // Use cvKMeans2 to cluster the colors
    int num_pixels = color_point_vector.size();
    assert (num_pixels > 0);

    cv::Mat color_point_mat (num_pixels, 1, CV_32FC3);       // rows=num_pixels, cols=1, type = 8bit Unsigned Channels2
    //CvMat* cluster_index_mat = cvCreateMat(num_pixels, 1, CV_32SC1);
    cv::Mat cluster_index_mat (num_pixels, 1, CV_32SC1);

    for (int i = 0; i < num_pixels; i++) {
        float* matPtr = (float*)(color_point_mat.data + i*color_point_mat.step);
        matPtr[0] = color_point_vector[i].first.m1;
        matPtr[1] = color_point_vector[i].first.m2;
        matPtr[2] = color_point_vector[i].first.m3;
    }
    /*for (int i = 0; i < num_pixels; i++) {
        float* data = (float*)(color_point_mat.data + i*color_point_mat.step);
        printf ("Pixel stored in matrix: (%5.1f, %5.1f, %5.1f)\n", data[0], data[1], data[2]);
    }*/

    double best_validity = -100;    // will keep track of lowest validity number
    int bad_cluster_counter = 0;    // keeps track of how many non-valid clustering configs we've had
    int max_n_clusters = std::min(6, num_pixels);

    for (int n_clusters = 1; n_clusters <= max_n_clusters; n_clusters++) {
        cv::Mat centers (n_clusters, 1, CV_32FC3);

        double compactness = cv::kmeans (
                color_point_mat,    
                n_clusters,
                cluster_index_mat,
                cvTermCriteria (CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 10, 1.0),
                1,      // attempts
                cv::KMEANS_PP_CENTERS,      // flags
#if CV_MINOR_VERSION > 2
                centers
#else
                &centers   // CvArr* centers
#endif
                //&compactness
        );

        // find the inter cluster diff
        double min_cluster_dist = 0;
        int num_cluster_dists = 0;
        if (n_clusters == 1) {
            min_cluster_dist = 3 * 30*30;
        }
        else {
            for (int c1 = 0; c1 < n_clusters; c1++) {
                for (int c2 = c1+1; c2 < n_clusters; c2++) {
                    float* center1 = (float*)(centers.data + c1*centers.step);
                    float* center2 = (float*)(centers.data + c2*centers.step);
                    assert (center1 != center2);

                    float d0 = center1[0] - center2[0];
                    float d1 = center1[1] - center2[1];
                    float d2 = center1[2] - center2[2];

                    double cluster_dist = d0*d0 + d1*d1 + d2*d2;
                    //assert (cluster_dist != 0);
                    if (cluster_dist == 0) // two clusters just happen to coincide?
                        continue;

                    #ifdef USE_AVERAGE_INTER_CLUSTER_DIST
                        // find the average inter cluster dist
                        min_cluster_dist += cluster_dist;
                        num_cluster_dists++;
                    #else
                        // find the min cluster dist
                        if (min_cluster_dist == 0 || cluster_dist < min_cluster_dist)
                            min_cluster_dist = cluster_dist;
                    #endif
                }
            }

            #ifdef USE_AVERAGE_INTER_CLUSTER_DIST
                min_cluster_dist /= num_cluster_dists;
            #endif
        }

        double validity = compactness / min_cluster_dist;
        DEBUG_PRINT ("n_clusters=%d, compactness=%2.0lf, min_cluster_dist=%2.0lf, validity=%3.1lf\n", 
                n_clusters, compactness, min_cluster_dist, validity);
        
        if (best_validity < 0 || (best_validity/validity) > 1.1) {
            best_validity = validity;

            DEBUG_PRINT ("New Cluster Configuration Accepted\n");
            for (int i = 0; i < num_pixels; i++) {
                color_point_vector[i].first.index_number = 1+static_cast<int>(*(cluster_index_mat.data + i*cluster_index_mat.step));
            }

            final_index_number = n_clusters+1;
        }
        else {
            if (++bad_cluster_counter >= 2) {
                DEBUG_PRINT ("Broke from clustering. Last attempt used %d clusters\n", n_clusters);
                break;
            }
        }
        /*
        for (int i = 0; i < num_pixels; i++) {
            float* data = (float*)(color_point_mat.data + i*color_point_mat.step);
            int index_number = static_cast<int>(*(cluster_index_mat.data + i*cluster_index_mat.step));
            printf ("\t(%5.1lf, %5.1lf, %5.1lf)  index_number=%d\n", data[0], data[1], data[2], index_number);
        }*/
        /*for (int i = 0; i < n_clusters; i++) {
            float* data = (float*)(centers.data + i*centers.step);
            printf ("\tCenter (%5.1f, %5.1f, %5.1f)\n", data[0], data[1], data[2]);
        }*/
    }
}

void mvWatershedFilter::watershed_filter_internal (IplImage* src, IplImage* dst) {
    cvWatershed(scratch_image_3c, marker_img_32s);

    // go thru each pixel in the marker img and do two things
    // 1. fill in dst so we can show it and stuff later
    // 2. add each pixel from a segment to its entry in segment_color_hash
    for (int i = 0; i < marker_img_32s->height; i++) {
        unsigned char* dstPtr = (unsigned char*)dst->imageData + i*dst->widthStep;
        unsigned char* colorPtr = (unsigned char*)scratch_image_3c->imageData + i*scratch_image_3c->widthStep;

        for (int j = 0; j < marker_img_32s->width; j++) {
            int index_number = CV_IMAGE_ELEM(marker_img_32s, int, i, j);
            if (index_number == -1) {
                *dstPtr = 0;
            }
            else {
                unsigned char index_char = static_cast<unsigned char>(index_number);
                // 1.
                *dstPtr = index_char * MAX_INDEX_NUMBER / final_index_number;
                // 2.
                segment_color_hash[index_char].add_pixel(colorPtr[0],colorPtr[1],colorPtr[2]);
            }

            dstPtr ++;
            colorPtr += 3;
        }
    }

    DEBUG_PRINT ("Watershed Segments:\n");
    // calculate the mean color profile of each segment
    std::map<unsigned char,COLOR_TRIPLE>::iterator seg_iter = segment_color_hash.begin();
    std::map<unsigned char,COLOR_TRIPLE>::iterator seg_iter_end = segment_color_hash.end();
    for (; seg_iter != seg_iter_end; ++seg_iter) {
        COLOR_TRIPLE* ct_ptr = &(seg_iter->second);
        if (ct_ptr->n_pixels > 0) {
            ct_ptr->calc_average();
            DEBUG_PRINT ("\tSegment: index %d (%d pixels): (%3d,%3d,%3d)\n", ct_ptr->index_number, ct_ptr->n_pixels, ct_ptr->m1, ct_ptr->m2, ct_ptr->m3);
        }
    }
}

bool mvWatershedFilter::get_next_watershed_segment(IplImage* binary_img, COLOR_TRIPLE &T) {
    assert (binary_img->width == marker_img_32s->width);
    assert (binary_img->height == marker_img_32s->height);
    
    // check if we are out of segments
    //if (curr_segment_iter == segment_color_hash.end()) {
    if (curr_segment_index >= segment_color_hash.size()) {
        return false;   
    }

    // get the index number of the current segment, then obtain a binary image which is 1 for each pixel
    // on the watershed result that matches the index number, and 0 otherwise
    int index_number = static_cast<int>(curr_segment_iter->second.index_number);
    T = curr_segment_iter->second;
    cvCmpS (marker_img_32s, index_number, binary_img, CV_CMP_EQ);

    ++curr_segment_iter;
    ++curr_segment_index;
    return true;
}
