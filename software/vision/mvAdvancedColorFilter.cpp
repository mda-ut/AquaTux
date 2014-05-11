#include "mvColorFilter.h"
#include <math.h>

#define USE_BGR_COLOR_SPACE
#define FLOOD_IMAGE_COMPARE_WITH_ORIG

#define M_DEBUG
#ifdef M_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

bool triple_has_more_pixels (COLOR_TRIPLE t1, COLOR_TRIPLE t2) {
    return (t1.n_pixels > t2.n_pixels);
}
void cvt_img_to_HSV (IplImage* src, IplImage* dst) {
    #ifndef USE_BGR_COLOR_SPACE
        cvCvtColor(src, dst, CV_BGR2HSV);
    #endif
}
void cvt_img_to_BGR (IplImage* src, IplImage* dst) {
    #ifndef USE_BGR_COLOR_SPACE
        cvCvtColor(src, dst, CV_HSV2BGR);
    #endif
}

void mvGetBoundsFromGaussian (
    COLOR_TRIPLE_FLOAT mean, COLOR_TRIPLE_FLOAT variance, COLOR_TRIPLE_FLOAT skew, 
    COLOR_TRIPLE &upper, COLOR_TRIPLE &lower
)
{
    COLOR_TRIPLE_FLOAT stdev;
    stdev.mf1 = sqrt(variance.mf1);
    stdev.mf2 = sqrt(variance.mf2);
    stdev.mf3 = sqrt(variance.mf3);

    int m1 = static_cast<int>(mean.mf1 + 2*stdev.mf1);
    int m2 = static_cast<int>(mean.mf2 + 2*stdev.mf2);
    int m3 = static_cast<int>(mean.mf3 + 2*stdev.mf3);
    upper.m1 = (m1 > 255) ? 255 : m1;
    upper.m2 = (m2 > 255) ? 255 : m2;
    upper.m3 = (m3 > 255) ? 255 : m3;

    m1 = static_cast<int>(mean.mf1 - 2*stdev.mf1);
    m2 = static_cast<int>(mean.mf2 - 2*stdev.mf2);
    m3 = static_cast<int>(mean.mf3 - 2*stdev.mf3);
    lower.m1 = (m1 < 0) ? 0 : m1;
    lower.m2 = (m2 < 0) ? 0 : m2;
    lower.m3 = (m3 < 0) ? 0 : m3;
}

mvAdvancedColorFilter::mvAdvancedColorFilter (const char* settings_file) : 
    bin_Resize ("mvAdvanced - Resize"),
    bin_Filter ("mvAdvanced - Filter")
{
    assert (KERNEL_SIZE % 2 == 1);
    
    // read constants from file
    read_mv_setting (settings_file, "COLOR_DIST", COLOR_DIST);
    read_mv_setting (settings_file, "HUE_DIST", H_DIST);
    read_mv_setting (settings_file, "SAT_DIST", S_DIST);
    read_mv_setting (settings_file, "VAL_DIST", V_DIST);
    
    int width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);

    // create Hue_Box
    for (int i = 0; i < NUM_BOXES; i++) {
        hue_box[i] = new Hue_Box(settings_file, i+1);
    }

    // create downsampled scratch images. The 1 channel image shares data with the 3 channel
    ds_scratch_3 = cvCreateImage(cvSize(width/DS_FACTOR, height/DS_FACTOR), IPL_DEPTH_8U, 3);
    ds_scratch = cvCreateImage(cvSize(width/DS_FACTOR, height/DS_FACTOR), IPL_DEPTH_8U, 1);

    // generate kernel point array
    KERNEL_AREA = KERNEL_SIZE * KERNEL_SIZE;
    KERNEL_RAD = (KERNEL_SIZE - 1)/2;

    if (KERNEL_SHAPE_IS_RECT) {
        kernel_point_array = new int[KERNEL_AREA];
        
        unsigned array_index = 0;
        for (int j = -KERNEL_RAD; j <= KERNEL_RAD; j++)
            for (int i = -KERNEL_RAD; i <= KERNEL_RAD; i++)
                kernel_point_array[array_index++] = i*ds_scratch->widthStep + j;       
    } 
    else {
        int* temp_array = new int[KERNEL_AREA];
        unsigned array_index = 0;
        unsigned valid_count = 0;
        int R2 = KERNEL_RAD*KERNEL_RAD;//KERNEL_SIZE*KERNEL_SIZE/4;

        for (int j = -KERNEL_RAD; j <= KERNEL_RAD; j++) {
            for (int i = -KERNEL_RAD; i <= KERNEL_RAD; i++) {
                if (i*i + j*j <= R2) { // check if i,j are in the ellipse
                    temp_array[array_index++] = i*ds_scratch->widthStep + j;
                    valid_count++;   
                }
                else {
                    temp_array[array_index++] = MV_UNDEFINED_VALUE;
                }                
            }
        }

        /// copy the valid points to kernel_point_array
        kernel_point_array = new int[valid_count];
        array_index = 0;
        for (int i = 0; i < KERNEL_AREA; i++) {
            if (temp_array[i] != MV_UNDEFINED_VALUE) {
                kernel_point_array[array_index++] = temp_array[i];
            }
        }
        delete[] temp_array;
        KERNEL_AREA = valid_count;
    }

    FLAG_DO_COLOR_ADJUSTMENT = false;
    Training_Matrix.resize(NUM_INTERACTIVE_COLORS);
    
#ifdef USE_BGR_COLOR_SPACE
    printf ("mvAdvancedColorFilter is using BGR color space\n");
#else
    printf ("mvAdvancedColorFilter is using HSV color space\n");
#endif
}

mvAdvancedColorFilter::~mvAdvancedColorFilter () {
    delete[] kernel_point_array;

    for (int i = 0; i < NUM_BOXES; i++)
        delete hue_box[i];

    cvReleaseImage (&ds_scratch_3);
    cvReleaseImage (&ds_scratch);
}

void mvAdvancedColorFilter::mean_shift(IplImage* src, IplImage* dst) {
      assert (src->nChannels == 3);
      assert (dst->nChannels == 3);
      downsample_from (src);
      cvt_img_to_HSV(ds_scratch_3, ds_scratch_3);

      meanshift_internal(src);
    
      cvt_img_to_BGR(ds_scratch_3, ds_scratch_3);
      upsample_to_3 (dst);
}

void mvAdvancedColorFilter::filter(IplImage* src, IplImage* dst) {
      assert (src->nChannels == 3);
      assert (dst->nChannels == 1);
      downsample_from (src);
      cvt_img_to_HSV(ds_scratch_3, ds_scratch_3);

      colorfilter_internal_adaptive_hue();

      upsample_to (dst);
}

void mvAdvancedColorFilter::combined_filter(IplImage* src, IplImage* dst) {
      assert (src->nChannels == 3);
      assert (dst->nChannels == 1);
      downsample_from (src);
      cvt_img_to_HSV(ds_scratch_3, ds_scratch_3);

      meanshift_internal(src);
      colorfilter_internal();

      upsample_to (dst);
}

void mvAdvancedColorFilter::flood_image(IplImage* src, IplImage* dst, bool interactive) {
    assert (src->nChannels == 3);
    assert (dst->nChannels == 1);
    downsample_from (src);

    bin_Filter.start();
    cvt_img_to_HSV(ds_scratch_3, ds_scratch_3);

    meanshift_internal(src);

    // this does most of the work
    flood_image_internal();

    // if interactive mode, we want to display the image and register the interactive callback 
    if (interactive) {
        Current_Interactive_Color = 0;
        for (int i = 0; i < NUM_INTERACTIVE_COLORS; i++)
            Training_Matrix[i].clear();
        cvNamedWindow ("flood_image_callback");
        cvSetMouseCallback("flood_image_callback", flood_image_interactive_callback, static_cast<void*>(this));
        cvShowImage("flood_image_callback", ds_scratch);
        printf ("Interactive mode is active.\n");
        printf ("Current Color is %d\n", Current_Interactive_Color);
        cvWaitKey(0);

        if (FLAG_DO_COLOR_ADJUSTMENT) {
            perform_color_adjustment_internal ();
            FLAG_DO_COLOR_ADJUSTMENT = false;
        }
    }

    // go thru each active hue box and check if any of the models fit within the hue box
    // if so paint the pixels marked as those models to be the BOX number of that hue box
    cvZero(dst);
    for (int i = 0; i < NUM_BOXES; i++) {
        if (!hue_box[i]->BOX_ENABLED)
            continue;

        COLOR_TRIPLE_VECTOR::iterator iter_end = color_triple_vector.end();
        for (COLOR_TRIPLE_VECTOR::iterator iter = color_triple_vector.begin(); iter != iter_end; ++iter) {
            if (hue_box[i]->check_hsv(iter->m1, iter->m2,iter->m3)) {
                
                for (int k = 0; k < dst->height; k++) {
                    unsigned char* scrPtr = (unsigned char*)(ds_scratch->imageData + k*ds_scratch->widthStep);
                    unsigned char* dstPtr = (unsigned char*)(dst->imageData + k*dst->widthStep);
                    for (int l = 0; l < dst->width; l++) {
                        if (*scrPtr == iter->index_number)
                            *dstPtr = 255;//hue_box[i]->BOX_NUMBER*50;
                        scrPtr++;
                        dstPtr++;
                    }
                }

            }
        }
    }

    bin_Filter.stop();
}

bool mvAdvancedColorFilter::check_and_accumulate_pixel (unsigned char* pixel, unsigned char* ref_pixel, COLOR_TRIPLE &triple)
{
    #ifdef USE_BGR_COLOR_SPACE
        return check_and_accumulate_pixel_BGR (pixel, ref_pixel, triple);
    #else
        return check_and_accumulate_pixel_HSV (pixel, ref_pixel, triple);
    #endif
}

bool mvAdvancedColorFilter::check_and_accumulate_pixel_BGR (unsigned char* pixel, unsigned char* ref_pixel, COLOR_TRIPLE &triple)
{
    // although this uses BGR, the variable names are still HSV, lol...
    int B = pixel[0];
    int G = pixel[1];
    int R = pixel[2];

    //if (abs(B-ref_pixel[0]) + abs(G-ref_pixel[1]) + abs(R-ref_pixel[2]) < COLOR_DIST)
    if (abs(B-ref_pixel[0]) < H_DIST && abs(G-ref_pixel[1]) < S_DIST && abs(R-ref_pixel[2]) < V_DIST)
    {
        triple.m1 += (unsigned)B;           
        triple.m2 += (unsigned)G;
        triple.m3 += (unsigned)R;
        triple.n_pixels++;

        return true;
    }
    return false;
}

bool mvAdvancedColorFilter::check_and_accumulate_pixel_HSV (unsigned char* pixel, unsigned char* ref_pixel, COLOR_TRIPLE &triple)
{
    int H = pixel[0];
    int S = pixel[1];
    int V = pixel[2];
    int Href = ref_pixel[0];
    int Sref = ref_pixel[1];
    int Vref = ref_pixel[2];

    int Hdelta = abs(H - Href);
    int Sdelta = abs(S - Sref);
    int Vdelta = abs(V - Vref);

    if (std::min(Hdelta,180-Hdelta) + Sdelta + Vdelta < COLOR_DIST)
    {
        // Circular red case is hard, let's just use 0 if we're looking for red and see > 90,
        // and use 179 if we're looking for red >= 0
        if (Hdelta > 90) {
            if (H <= 90)
                triple.m1 += 0;
            else
                triple.m1 += 179;
        }
        else
            triple.m1 += (unsigned)H;
                
        triple.m2 += (unsigned)S;
        triple.m3 += (unsigned)V;
        triple.n_pixels++;

        return true;
    }
        
    return false;
}

void mvAdvancedColorFilter::meanshift_internal(IplImage* src_scratch) {
// src_scratch is the src image passed in by the user, which will now be used as a scratch
#ifdef MEANSHIFT_DEBUG
    cvNamedWindow("mvAdvancedColorFilter debug", CV_WINDOW_AUTOSIZE);
#endif   
    cvZero (src_scratch);

    // we will be filtering ds_scratch_3 to src_scratch, then copying the data back to ds_scratch_3
    unsigned char* imgPtr, *resPtr;
    unsigned char* imgLast = (unsigned char*) (ds_scratch_3->imageData+ds_scratch_3->height*ds_scratch_3->widthStep);

    for (int r = KERNEL_RAD; r < ds_scratch_3->height-KERNEL_RAD; r++) {                         
        imgPtr = (unsigned char*) (ds_scratch_3->imageData + r*ds_scratch_3->widthStep); // imgPtr = first pixel of rth's row
        resPtr = (unsigned char*) (src_scratch->imageData + r*src_scratch->widthStep);
        
        if (imgPtr >= imgLast)
            break;

        for (int c = 3*KERNEL_RAD; c < ds_scratch_3->width-3*KERNEL_RAD; c++) {
            // skip pixel if already visited
            if (resPtr[1] != 0) {
                imgPtr += 3;
                resPtr += 3;
                continue;
            }

            unsigned char* tempPtr;
            COLOR_TRIPLE triple(imgPtr[0], imgPtr[1], imgPtr[2], 1);
            unsigned total_pixels = 1;

            // go thru each pixel in the kernel
            for (int i = 0; i < KERNEL_AREA; i++) {
                tempPtr = imgPtr + 3*kernel_point_array[i];

                check_and_accumulate_pixel (tempPtr, imgPtr, triple);

                total_pixels++;
            }

            // if good enough, visit every pixel in the kernel and set value equal to average
            if (GOOD_PIXELS_FACTOR*triple.n_pixels >= total_pixels) {
                triple.calc_average();

                for (int i = 0; i < KERNEL_AREA; i++) {
                    tempPtr = resPtr + 3*kernel_point_array[i];
                    tempPtr[0] = triple.m1;
                    tempPtr[1] = triple.m2;
                    tempPtr[2] = triple.m3;
                }
            }
        
            imgPtr += 3;
            resPtr += 3;

#ifdef MEANSHIFT_DEBUG
            cvShowImage("mvAdvancedColorFilter debug", src_scratch);
            cvWaitKey(2);
#endif
        }
    }

    // copy src_scratch's useful data to ds_scratch_3
    for (int r = 0; r < ds_scratch_3->height; r++) {            
        imgPtr = (unsigned char*) (src_scratch->imageData + r*src_scratch->widthStep);             
        resPtr = (unsigned char*) (ds_scratch_3->imageData + r*ds_scratch_3->widthStep);
        memcpy (resPtr, imgPtr, ds_scratch_3->widthStep);
    }
}

void mvAdvancedColorFilter::colorfilter_internal() {
// this function goes over the Hue_Box array and if a pixel falls inside box X, it marks that pixel
// with value X
    unsigned char *imgPtr, *resPtr;
    for (int r = 0; r < ds_scratch->height; r++) {                        
        imgPtr = (unsigned char*) (ds_scratch_3->imageData + r*ds_scratch_3->widthStep);
        resPtr = (unsigned char*) (ds_scratch->imageData + r*ds_scratch->widthStep); 
   
        for (int c = 0; c < ds_scratch->width; c++) {
            *resPtr = 0;
 
            for (int i = 0; i < NUM_BOXES; i++) {
                if (!(hue_box[i])->is_enabled())
                    continue;

                if (imgPtr[1] != 0 && (hue_box[i])->check_hsv(imgPtr[0], imgPtr[1], imgPtr[2])) {
                    *resPtr = (unsigned char) hue_box[i]->BOX_COLOR;
                }
            }

            imgPtr += 3;
            resPtr++;
        }
    }
}

void mvAdvancedColorFilter::colorfilter_internal_adaptive_hue() {
// this function goes over the Hue_Box array and if a pixel falls inside box X, it marks that pixel
// with value X. Uses adaptive hue box so the range of hue can change
    unsigned char *imgPtr, *resPtr;
    for (int r = 0; r < ds_scratch->height; r++) {                        
        imgPtr = (unsigned char*) (ds_scratch_3->imageData + r*ds_scratch_3->widthStep);
        resPtr = (unsigned char*) (ds_scratch->imageData + r*ds_scratch->widthStep); 
   
        for (int c = 0; c < ds_scratch->width; c++) {
            *resPtr = 0;

            for (int i = 0; i < NUM_BOXES; i++) {
                if (!(hue_box[i])->is_enabled())
                    continue;

                if (imgPtr[1] != 0 && (hue_box[i])->check_hsv_adaptive_hue(imgPtr[0], imgPtr[1], imgPtr[2])) {
                    *resPtr = (unsigned char) hue_box[i]->BOX_COLOR;
                }
            }

            imgPtr += 3;
            resPtr++;
        }
    }

    for (int i = 0; i < NUM_BOXES; i++) {
        hue_box[i]->update_hue();
    }
}

void mvAdvancedColorFilter::flood_image_internal() {
// This algorithm marks each different region of the image with a different index. Each
// region has an average color triple that is stored in COLOR_TRIPLE_Vector[index]
    color_triple_vector.clear();

    // we use ds_scratch as a mask image. If something is marked nonzero in ds_scratch the algorithm
    // will assume the pixel is already processed and skip it.
    cvZero (ds_scratch);

    // this loop does most of the work. It calls flood from pixel in a grid pattern along the image
    unsigned index_number = 10;
    for (int r = 3; r < ds_scratch->height - 3; r+=20) {                         
        for (int c = 3; c < ds_scratch->width - 3; c+=20) {
            if (flood_from_pixel (r,c, index_number))
                index_number += 5;
        }

        if (index_number > 255) {
            printf ("Critical Warning: Bin Index exceeds 255 in flood_image. Ending algorithm prematurely.\n");
            break;
        }
    }

    std::sort (color_triple_vector.begin(), color_triple_vector.end(), triple_has_more_pixels);

    COLOR_TRIPLE_VECTOR::iterator iter_end = color_triple_vector.end();
    for (COLOR_TRIPLE_VECTOR::iterator iter = color_triple_vector.begin(); iter != iter_end; ++iter) {
        iter->print();
    }
    DEBUG_PRINT ("\n");
}

bool mvAdvancedColorFilter::flood_from_pixel(int R, int C, unsigned index_number) {
// assumes ds_scratch is zeroed as needed and does not use profile bin
//#define FLOOD_DEBUG
#ifdef FLOOD_DEBUG
     cvNamedWindow("mvAdvancedColorFilter debug");
     cvSetMouseCallback("mvAdvancedColorFilter debug", show_HSV_call_back, static_cast<void *>(ds_scratch));
#endif

    int widthStep = ds_scratch->widthStep;
    int widthStep3 = ds_scratch_3->widthStep;
    unsigned char* imgPtr = (unsigned char*) (ds_scratch_3->imageData + R*widthStep3+C*3);
    unsigned char* resPtr = (unsigned char*) (ds_scratch->imageData + R*widthStep+C);
    
    if (*imgPtr == 0 || *resPtr != 0)
        return false;

    // create a color model
    COLOR_TRIPLE color_triple(imgPtr[0],imgPtr[1],imgPtr[2],index_number); 

    // use a queue, each time we visit a new pixel, check if its a "good" pixel and queue
    // visits to neighbours. Good pixels are marked with TEMP_PIXEL
    // loop until queue empty
    std::vector < std::pair<int,int> > Point_Array;
    Point_Array.push_back(std::make_pair(R,C));

#ifdef FLOOD_IMAGE_COMPARE_WITH_ORIG
    unsigned char* imgPtrOrig = imgPtr;
    do {
        // dequeue the front pixel
        int r = Point_Array.back().first;
        int c = Point_Array.back().second;
        Point_Array.pop_back();
        resPtr = (unsigned char*) (ds_scratch->imageData + r*widthStep+c);
        // check if we've visited this pixel before
        if (*resPtr != 0)
            continue;

        imgPtr = (unsigned char*) (ds_scratch_3->imageData + r*widthStep3+c*3);
        bool pixel_good = check_and_accumulate_pixel (imgPtr, imgPtrOrig, color_triple);
        if (pixel_good) {
            // mark pixel as visited
            *resPtr = TEMP_PIXEL;

            // queue neighbours
            if (c > 0 && resPtr[-1] == 0)
                Point_Array.push_back(std::make_pair(r,c-1));
            if (c < ds_scratch->width-1 && resPtr[1] == 0)
                Point_Array.push_back(std::make_pair(r,c+1));
            if (r > 0 && resPtr[-widthStep] == 0)
                Point_Array.push_back(std::make_pair(r-1,c));
            if (r < ds_scratch->height-1 && resPtr[widthStep] == 0)
                Point_Array.push_back(std::make_pair(r+1,c));
        }            
        else {
            //DEBUG_PRINT ("failed %d,%d,%d vs %d,%d,%d\n",imgPtr[0],imgPtr[1],imgPtr[2],imgPtrOrig[0],imgPtrOrig[1],imgPtrOrig[2]);
        }
    } while (!Point_Array.empty());
#else 
    do {
        // dequeue the front pixel
        int r = Point_Array.back().first;
        int c = Point_Array.back().second;
        Point_Array.pop_back();
        resPtr = (unsigned char*) (ds_scratch->imageData + r*widthStep + c);
        imgPtr = (unsigned char*) (ds_scratch_3->imageData + r*widthStep3 + c*3);

        // check each pixel around it
        if (c > 0 && resPtr[-1] == 0) { // left
            if (check_and_accumulate_pixel (imgPtr-3, imgPtr, color_triple)) {
                resPtr[-1] = TEMP_PIXEL;
                Point_Array.push_back(std::make_pair(r,c-1));
            }
        }
        if (c < ds_scratch->width-1 && resPtr[1] == 0) { // right
            if (check_and_accumulate_pixel (imgPtr+3, imgPtr, color_triple)) {
                resPtr[1] = TEMP_PIXEL;
                Point_Array.push_back(std::make_pair(r,c+1));
            }
        }
        if (r > 0 && resPtr[-widthStep] == 0) { // above
            if (check_and_accumulate_pixel (imgPtr-widthStep3, imgPtr, color_triple)) {
                resPtr[-widthStep] = TEMP_PIXEL;
                Point_Array.push_back(std::make_pair(r-1,c));
            }
        }
        if (r < ds_scratch->height-1 && resPtr[widthStep] == 0) { // below
            if (check_and_accumulate_pixel (imgPtr+widthStep3, imgPtr, color_triple)) {
                resPtr[widthStep] = TEMP_PIXEL;
                Point_Array.push_back(std::make_pair(r+1,c));
            }
        }
    } while (!Point_Array.empty());    
#endif

    int max_index_number;
    
    // if the box doesnt contain enough pixels, throw it out
    if (color_triple.n_pixels < 50) {//(unsigned)ds_scratch->width*ds_scratch->height/300) {
        max_index_number = 0;
    }
    else {
        color_triple.calc_average();
        
        /*// attempt to merge the box with an existing box. This is if the boxes are very similar
        COLOR_TRIPLE_VECTOR::iterator min_diff_iter;
        int min_diff = 9000;
        
        COLOR_TRIPLE_VECTOR::iterator iter_end = color_triple_vector.end();
        for (COLOR_TRIPLE_VECTOR::iterator iter = color_triple_vector.begin(); iter != iter_end; ++iter) {
            int diff =  abs((int)iter->m1-(int)color_triple.m1) + 
                        abs((int)iter->m2-(int)color_triple.m2) + 
                        abs((int)iter->m3-(int)color_triple.m3);
            if (diff < min_diff) {
                min_diff = diff;
                min_diff_iter = iter;
            }
        }
        // if could not merge, add the new box to the vector
        if (min_diff < 0.8*COLOR_DIST) {
            printf ("merging BGR triplet #%d (%d %d %d) with #%d (%d %d %d)\n", 
                    min_diff_iter->index_number, min_diff_iter->m1, min_diff_iter->m2, min_diff_iter->m3, 
                    color_triple.index_number, color_triple.m1, color_triple.m2, color_triple.m3
                    );
            min_diff_iter->merge(color_triple);
            max_index_number = min_diff_iter->index_number;
        }
        else {
            color_triple_vector.push_back(color_triple);
            max_index_number = color_triple.index_number;
        }*/
#ifdef USE_BGR_COLOR_SPACE
#else
        color_triple.BGR_to_HSV();
#endif
        bool merged = false;
        COLOR_TRIPLE_VECTOR::iterator iter_end = color_triple_vector.end();
        for (COLOR_TRIPLE_VECTOR::iterator iter = color_triple_vector.begin(); iter != iter_end; ++iter) {
            if (abs((int)iter->m1 - (int)color_triple.m1) < H_DIST/2 &&
                abs((int)iter->m2 - (int)color_triple.m2) < S_DIST/2 &&
                abs((int)iter->m3 - (int)color_triple.m3) < V_DIST/2
                )
            {
                printf ("merging HSV triplet #%d (%d %d %d) with #%d (%d %d %d)\n", 
                    iter->index_number, iter->m1, iter->m2, iter->m3, 
                    color_triple.index_number, color_triple.m1, color_triple.m2, color_triple.m3
                    );

                iter->merge(color_triple);
                max_index_number = iter->index_number;
                merged = true;
                break;
            }
        }
        if (!merged) {
            color_triple_vector.push_back(color_triple);
            max_index_number = color_triple.index_number;
        }
    }

    // paint the pixels with the appropriate index number
    for (int r = 0; r < ds_scratch->height; r++) {                         
        resPtr = (unsigned char*) (ds_scratch->imageData + r*widthStep);
        for (int c = 0; c < ds_scratch->width; c++) {
            if (*resPtr == TEMP_PIXEL)
                *resPtr = max_index_number;
            resPtr++;
        }
    }
  
#ifdef FLOOD_DEBUG
    cvShowImage("mvAdvancedColorFilter debug", ds_scratch);
    cvWaitKey(10);
#endif

    return (max_index_number != 0);
}

void flood_image_interactive_callback(int event, int x, int y, int flags, void* param) {
    mvAdvancedColorFilter* instance = (mvAdvancedColorFilter*) param;
    IplImage* img_1 = instance->ds_scratch;
    IplImage* img_3 = instance->ds_scratch_3;
    unsigned char* imgPtr1 = (unsigned char*)(img_1->imageData + y*img_1->widthStep + x);

    if (event == CV_EVENT_LBUTTONDOWN) { // paint the bin with pixel (x,y) as current color
        // Set the Current_Interactive_Color index of the Training_Matrix to color_triple_vector
        unsigned index_number = *imgPtr1;
        unsigned num_pixels_added = 0;

        // go through img_1, for each pixel that matches index number, add the corresponding pixel in img_3 to the Training_Matrix
        for (int r = 0; r < img_1->height; r++) {                        
            unsigned char* imgPtr3 = (unsigned char*) (img_3->imageData + r*img_3->widthStep);
            unsigned char* imgPtr1 = (unsigned char*) (img_1->imageData + r*img_1->widthStep);
                 
            for (int c = 0; c < img_1->width; c++) {
                if (*imgPtr1 == index_number) {
                    instance->Training_Matrix[instance->Current_Interactive_Color].push_back(COLOR_TRIPLE(imgPtr3[0],imgPtr3[1],imgPtr3[2],1));                    
                    num_pixels_added++;
                }

                imgPtr3 += 3;
                imgPtr1++;
            }
        }

        printf ("Added %d pixels from bin %d to vector interactive color %d\n", num_pixels_added, index_number, instance->Current_Interactive_Color);
    }
    else if (event == CV_EVENT_RBUTTONDOWN) { // move to the next color
        instance->Current_Interactive_Color += 1;
        if (instance->Current_Interactive_Color == mvAdvancedColorFilter::NUM_INTERACTIVE_COLORS) {
            instance->Current_Interactive_Color = 0;
        }
        printf ("Current Color is %d\n", instance->Current_Interactive_Color);
    }
    else if (event == CV_EVENT_MBUTTONDOWN) {
        if (instance->FLAG_DO_COLOR_ADJUSTMENT)
            instance->FLAG_DO_COLOR_ADJUSTMENT = false;
        else
            instance->FLAG_DO_COLOR_ADJUSTMENT = true;
        
        printf ("Re-Adjust Color Filters on Exit = %s\n", instance->FLAG_DO_COLOR_ADJUSTMENT ? "TRUE" : "FALSE");
    }
}

void mvAdvancedColorFilter::perform_color_adjustment_internal() {
    // go thru each of the training matrix's COLOR_TRIPLE_VECTOR's and calculate statistics
    for (int i = 0; i < mvAdvancedColorFilter::NUM_INTERACTIVE_COLORS; i++) {
        printf ("INTERACTIVE_COLOR %d\n", i);
        COLOR_TRIPLE_VECTOR::iterator iter_begin = Training_Matrix[i].begin();
        COLOR_TRIPLE_VECTOR::iterator iter_end = Training_Matrix[i].end();
        COLOR_TRIPLE_FLOAT mean, variance, skewness;

        // mean
        for (COLOR_TRIPLE_VECTOR::iterator iter = iter_begin; iter != iter_end; ++iter) {
            mean.add_pixel (
                static_cast<int>(iter->m1),
                static_cast<int>(iter->m2),
                static_cast<int>(iter->m3)
            );
        }
        mean.calc_average();

        // variance
        for (COLOR_TRIPLE_VECTOR::iterator iter = iter_begin; iter != iter_end; ++iter) {
            variance.add_pixel(
                pow(static_cast<double>(iter->m1)-mean.mf1, 2),
                pow(static_cast<double>(iter->m2)-mean.mf2, 2),
                pow(static_cast<double>(iter->m3)-mean.mf3, 2) 
            );
        }
        variance.calc_average();

        // skewness
        for (COLOR_TRIPLE_VECTOR::iterator iter = iter_begin; iter != iter_end; ++iter) {
            skewness.add_pixel(
                pow(static_cast<double>(iter->m1)-mean.mf1, 3),
                pow(static_cast<double>(iter->m2)-mean.mf2, 3),
                pow(static_cast<double>(iter->m3)-mean.mf3, 3) 
            );
        }
        skewness.calc_average();
        skewness.mf1 /= pow(variance.mf1, 1.5);
        skewness.mf2 /= pow(variance.mf2, 1.5);
        skewness.mf3 /= pow(variance.mf3, 1.5);

        //mean.print("\tmean    ");
        //variance.print("\tvariance");
        //skewness.print("\tskewness");

        COLOR_TRIPLE upper, lower;
        mvGetBoundsFromGaussian (mean, variance, skewness, upper, lower); 

        FILE *fp = fopen ("ADJUSTED_COLOR.csv", (i == 0) ? "w" : "a");
        fprintf (fp, "ENABLE_BOX_%d, 1\nCOLOR_BOX_%d, UNKNOWN\n", i+1,i+1);
        fprintf (fp,
            "HUE_MIN_1, %d\nHUE_MAX_1, %d\nSAT_MIN_1, %d\nSAT_MAX_1, %d\nVAL_MIN_1, %d\nVAL_MAX_1, %d\n", 
            lower.m1, upper.m1, lower.m2, upper.m2, lower.m3, upper.m3
            );
        fclose(fp);

        hue_box[i]->HUE_MIN = lower.m1;
        hue_box[i]->HUE_MAX = upper.m1;
        hue_box[i]->SAT_MIN = lower.m2;
        hue_box[i]->SAT_MAX = upper.m2;
        hue_box[i]->VAL_MIN = lower.m3;
        hue_box[i]->VAL_MAX = upper.m3;

        // hack - for now lets base our color distance on the first color
        if (i == 0) {
            H_DIST = upper.m1 - lower.m1;
            S_DIST = upper.m2 - lower.m2;
            V_DIST = upper.m3 - lower.m3;
            printf ("H_DIST = %d\nS_DIST = %d\nV_DIST = %d\n", H_DIST, S_DIST, V_DIST);
        }
    }
}

//#########################################################################
//#### Functions for Hue_Box.
//#########################################################################
Hue_Box::Hue_Box (const char* settings_file, int box_number) {
// read the HUE_MIN and HUE_MAX based on box number. So if box_number is 2, it reads
// HUE_MIN_2 and HUE_MAX_2
    BOX_NUMBER = box_number;
    std::string box_number_str;
    if (box_number == 1)
        box_number_str = "_1";
    else if (box_number == 2)
        box_number_str = "_2";
    else if (box_number == 3)
        box_number_str = "_3";
    else {
        printf ("Invalid box_number %d when constructing Hue_Box!\n", box_number);
        exit (1);
    }

    std::string enabled_str = std::string("ENABLE_BOX") + box_number_str;
    read_mv_setting (settings_file, enabled_str.c_str(), BOX_ENABLED);

    if (!BOX_ENABLED)
        return;

    // read the box color
    std::string box_color_str = std::string("COLOR_BOX") + box_number_str;
    std::string box_color;
    read_mv_setting (settings_file, box_color_str.c_str(), box_color);
    BOX_COLOR = color_str_to_int (box_color);

    std::string hue_min_str = std::string("HUE_MIN") + box_number_str;        
    std::string hue_max_str = std::string("HUE_MAX") + box_number_str;
    std::string sat_min_str = std::string("SAT_MIN") + box_number_str;
    std::string sat_max_str = std::string("SAT_MAX") + box_number_str;        
    std::string val_min_str = std::string("VAL_MIN") + box_number_str;        
    std::string val_max_str = std::string("VAL_MAX") + box_number_str;

    read_mv_setting (settings_file, hue_min_str.c_str(), HUE_MIN);
    read_mv_setting (settings_file, hue_max_str.c_str(), HUE_MAX);
    read_mv_setting (settings_file, sat_min_str.c_str(), SAT_MIN);
    read_mv_setting (settings_file, sat_max_str.c_str(), SAT_MAX);
    read_mv_setting (settings_file, val_min_str.c_str(), VAL_MIN);
    read_mv_setting (settings_file, val_max_str.c_str(), VAL_MAX);

    HUE_MIN_OUT = (HUE_MIN < HUE_GUTTER_LEN) ? 180+HUE_MIN-HUE_GUTTER_LEN : HUE_MIN-HUE_GUTTER_LEN;
    HUE_MAX_OUT = (HUE_MAX + HUE_GUTTER_LEN >= 180) ? HUE_MAX+HUE_GUTTER_LEN-180 : HUE_MAX+HUE_GUTTER_LEN;
    HUE_MIN_ADP = HUE_MIN;
    HUE_MAX_ADP = HUE_MAX;
    inner_count = min_inside_count = max_inside_count = min_outside_count = max_outside_count = 0;

    DEBUG_PRINT ("Hue_Box Number %d Constructed\n", box_number);
    DEBUG_PRINT ("\tBox Color %s. Greyscale Value = %d\n", box_color.c_str(), BOX_COLOR);
    DEBUG_PRINT ("\tHue MinMax = [%d,%d]\n", HUE_MIN, HUE_MAX);
}

bool Hue_Box::check_hsv_adaptive_hue (unsigned char hue, unsigned char sat, unsigned char val) {
    if (sat >= SAT_MIN && val >= VAL_MIN) {
        /// only 1 of these 3 can be true
        bool wrap_around_min_out = (HUE_MIN_OUT > HUE_MIN_ADP);
        bool wrap_around_max_out = (HUE_MAX_OUT < HUE_MAX_ADP);
        bool wrap_around = (HUE_MAX_ADP < HUE_MIN_ADP);

        if (wrap_around_min_out) {          // if the 180deg wraparound is between min_out and min_adp
            if (hue >= HUE_MIN_ADP && hue <= HUE_MAX_ADP) {
                inner_count++;
                return true;
            }
            else if ((hue <= HUE_MIN_OUT && hue <= HUE_MIN_ADP) || (hue >= HUE_MIN_OUT && hue >= HUE_MIN_ADP)) {
                min_outside_count++;
                return false;
            }
            else if (hue >= HUE_MAX_ADP && hue <= HUE_MAX_OUT) {
                max_outside_count++;
                return false;
            }
        }
        else if (wrap_around) {
            if ((hue >= HUE_MIN_ADP && hue >= HUE_MAX_ADP) || (hue <= HUE_MIN_ADP && hue <= HUE_MAX_ADP)) {
                inner_count++;
                return true;
            }
            else if (hue >= HUE_MIN_OUT && hue <= HUE_MIN_ADP) {
                min_outside_count++;
                return false;
            }
            else if (hue >= HUE_MAX_ADP && hue <= HUE_MAX_OUT) {
                max_outside_count++;
                return false;
            }
        }
        else if (wrap_around_max_out) {
            if (hue >= HUE_MIN_ADP && hue <= HUE_MAX_ADP) {
                inner_count++;
                return true;
            }
            else if (hue >= HUE_MIN_OUT && hue <= HUE_MIN_ADP) {
                min_outside_count++;
                return false;
            }
            else if ((hue >= HUE_MAX_ADP && hue >= HUE_MAX_OUT) || (hue <= HUE_MAX_ADP && hue <= HUE_MAX_OUT)) {
                max_outside_count++;
                return false;
            }
        }
        else {
            if (hue >= HUE_MIN_ADP && hue <= HUE_MAX_ADP) {
                inner_count++;
                return true;
            }
            else if (hue >= HUE_MIN_OUT && hue <= HUE_MIN_ADP) {
                min_outside_count++;
                return false;
            }
            else if (hue >= HUE_MAX_ADP && hue <= HUE_MAX_OUT) {
                max_outside_count++;
                return false;
            }
        }
    }
    return false;
}

void Hue_Box::update_hue () {
    DEBUG_PRINT ("Counts: %d  %d  %d  %d  %d\n", min_outside_count,min_inside_count, inner_count, max_inside_count, max_outside_count);
    
    if (abs(HUE_MIN_ADP-HUE_MIN) <= 10 && 6*min_outside_count > inner_count) {
        HUE_MIN_ADP = (HUE_MIN_ADP < HUE_ADP_LEN) ? 180+HUE_MIN_ADP-HUE_ADP_LEN : HUE_MIN_ADP-HUE_ADP_LEN;
        HUE_MIN_OUT = (HUE_MIN_OUT < HUE_ADP_LEN) ? 180+HUE_MIN_OUT-HUE_ADP_LEN : HUE_MIN_OUT-HUE_ADP_LEN;
    }
    else if (HUE_MAX_ADP > HUE_MIN_ADP+HUE_ADP_LEN && 60*min_outside_count < inner_count) {
        HUE_MIN_ADP = (HUE_MIN_ADP + HUE_ADP_LEN >= 180) ? HUE_MIN_ADP+HUE_ADP_LEN-180 : HUE_MIN_ADP+HUE_ADP_LEN;
        HUE_MIN_OUT = (HUE_MIN_OUT + HUE_ADP_LEN >= 180) ? HUE_MIN_OUT+HUE_ADP_LEN-180 : HUE_MIN_OUT+HUE_ADP_LEN;
    }

    if (abs(HUE_MAX_ADP-HUE_MAX) <= 10 && 6*max_outside_count > inner_count) {
        HUE_MAX_ADP = (HUE_MAX_ADP + HUE_ADP_LEN >= 180) ? HUE_MAX_ADP+HUE_ADP_LEN-180 : HUE_MAX_ADP+HUE_ADP_LEN;
        HUE_MAX_OUT = (HUE_MAX_OUT + HUE_ADP_LEN >= 180) ? HUE_MAX_OUT+HUE_ADP_LEN-180 : HUE_MAX_OUT+HUE_ADP_LEN;
    }
    else if (HUE_MAX_ADP > HUE_MIN_ADP+HUE_ADP_LEN && 60*max_outside_count < inner_count) {
        HUE_MAX_ADP = (HUE_MAX_ADP < HUE_ADP_LEN) ? 180+HUE_MAX_ADP-HUE_ADP_LEN : HUE_MAX_ADP-HUE_ADP_LEN;
        HUE_MAX_OUT = (HUE_MAX_OUT < HUE_ADP_LEN) ? 180+HUE_MAX_OUT-HUE_ADP_LEN : HUE_MAX_OUT-HUE_ADP_LEN;
    }

    inner_count = min_inside_count = max_inside_count = min_outside_count = max_outside_count = 0;
}
