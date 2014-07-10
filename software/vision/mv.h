/** mv - MDA vision library
 *  includes objects for filters and detectors
 *  Author - Ritchie Zhao
 */
#ifndef __MDA_VISION_MV__
#define __MDA_VISION_MV__

#include <cv.h>
#include "mgui.h"

typedef unsigned char uchar;
const int MV_UNDEFINED_VALUE = -8888888;

const int MV_NUMBER_OF_COLORS = 4;
const int MV_RED = 50;
const int MV_YELLOW = 100;
const int MV_GREEN = 150;
const int MV_BLUE = 200;
const int MV_COLOR_VECTOR[4] = { MV_RED, MV_YELLOW, MV_GREEN, MV_BLUE };
const int MV_UNCOLORED = 255;

inline int color_str_to_int (std::string str) {
    if (str == "RED")
        return MV_RED;
    else if (str == "YELLOW")
        return MV_YELLOW;
    else if (str == "GREEN")
        return MV_GREEN;
    else if (str == "BLUE")
        return MV_BLUE;
    return MV_UNCOLORED;
}
inline std::string color_int_to_string (int color) {
    switch (color) {
        case MV_RED: return std::string("RED");
        case MV_YELLOW: return std::string("YELLOW");
        case MV_GREEN: return std::string("GREEN");
        case MV_BLUE: return std::string("BLUE");
        default:;
    }
    return std::string("UNKNOWN");
}

// ##########################################################################
// Utility Functions
// ##########################################################################

// This function wraps cvWaitKey but will exit if q is pressed
inline char WAITKEY (int msecs)
{
    char c = cvWaitKey(msecs);
    if (c == 'q')
        exit(0);
    return c;
}

// ##########################################################################
// MvShape, base for MvCircle and MvRotatedBox
// ##########################################################################
class MvShape {
public:
    CvPoint center;
    int m1,m2,m3;
    int h1,h2,h3;
    int color_int;
    float validity;
    int count;
    MvShape () { validity = -1; count = 1; center.x=center.y=0; m1=m2=m3=0; color_int = MV_UNCOLORED; }
    virtual int center_diff (MvShape second) {
        return (abs(center.x-second.center.x) + abs(center.y-second.center.y));
    }
    virtual int color_diff (MvShape second) {
        return abs(m1-second.m1)+abs(m2-second.m2)+abs(m3-second.m3);
    }
    virtual int color_check (MvShape second, int limit) {
        return (abs(m1-second.m1)<limit && abs(m2-second.m2)<limit && abs(m3-second.m3)<limit);
    }
};

// ##########################################################################
// MvCircle
// ##########################################################################
class MvCircle : public MvShape {
public:
    float radius;
    
    MvCircle () { radius=0; }
    void drawOntoImage (IplImage* img) {
        cvCircle (img, center, static_cast<int>(radius), CV_RGB(50,50,50), 2);
    }
    MvCircle& operator = (MvCircle other) {
        this->center.x = other.center.x;
        this->center.y = other.center.y;
        this->radius = other.radius;
        this->m1 = other.m1;  this->m2 = other.m2;  this->m3 = other.m3;
        this->h1 = other.h1;  this->h2 = other.h2;  this->h3 = other.h3;
        this->color_int = other.color_int;
        this->validity = other.validity;
        this->count = this->count;
        return *this;
    }
    float diff(MvCircle &other) {
       const int RADIUS_WEIGHT = 2;
       return abs (this->center.x - other.center.x)
            + abs (this->center.y - other.center.y)
            +fabsf(this->radius - other.radius) * RADIUS_WEIGHT;
    }
    void shape_merge (MvCircle second) {
        int total_count = count + second.count;
        m1 = (count*m1 + second.count*second.m1) / total_count;
        m2 = (count*m2 + second.count*second.m2) / total_count;
        m3 = (count*m3 + second.count*second.m3) / total_count;
        center.x = (count*center.x + second.count*second.center.x) / total_count;
        center.y = (count*center.y + second.count*second.center.y) / total_count;
        radius = (count*radius + second.count*second.radius) / total_count;
        count = total_count;
    }
};

// ##########################################################################
// MvRotatedBox
// ##########################################################################
class MvRotatedBox : public MvShape {
public:
    float length, width;    // length is the long edge
    float angle;            // angle points in direction of length
    
    MvRotatedBox () { length=width=angle=0; }
    void drawOntoImage (IplImage* img) {
        CvPoint p0, p1;
        float ang_sin = -sin(angle*CV_PI/180.f);
        float ang_cos = cos(angle*CV_PI/180.f);
        int length_x = length/2 * ang_sin;  int length_y = length/2 * ang_cos;
        int width_x = width/2 * ang_cos;    int width_y = width/2 * ang_sin;
        p0.x = center.x-length_x-width_x;  p0.y = center.y-length_y+width_y;
        p1.x = center.x+length_x-width_x;  p1.y = center.y+length_y+width_y;
        cvLine (img, p0, p1, CV_RGB(50,50,50), 2);
        p0.x = center.x-length_x+width_x;  p0.y = center.y-length_y-width_y;
        p1.x = center.x+length_x+width_x;  p1.y = center.y+length_y-width_y;
        cvLine (img, p0, p1, CV_RGB(50,50,50), 2);
    }
    MvRotatedBox& operator = (MvRotatedBox other) {
        this->center.x = other.center.x;
        this->center.y = other.center.y;
        this->length = other.length;
        this->width = other.width;
        this->angle = other.angle;
        this->m1 = other.m1;  this->m2 = other.m2;  this->m3 = other.m3;
        this->h1 = other.h1;  this->h2 = other.h2;  this->h3 = other.h3;
        this->color_int = other.color_int;
        this->validity = other.validity;
        this->count = this->count;
        return *this;    
    }
    float diff(MvRotatedBox &other) {
       return abs (this->center.x - other.center.x)
            + abs (this->center.y - other.center.y)
            ///+ abs (this->angle - other.angle)
            //+ abs (this->length - other.length)
            //+ abs (this->width - other.width)
            ;
    }
    void shape_merge (MvRotatedBox second) {
        int total_count = count + second.count;
        m1 = (count*m1 + second.count*second.m1) / total_count;
        m2 = (count*m2 + second.count*second.m2) / total_count;
        m3 = (count*m3 + second.count*second.m3) / total_count;
        center.x = (count*center.x + second.count*second.center.x) / total_count;
        center.y = (count*center.y + second.count*second.center.y) / total_count;
        length = (count*length + second.count*second.length) / total_count;
        width = (count*width + second.count*second.width) / total_count;
        count = total_count;
    }
};
typedef std::vector<MvCircle> MvCircleVector;
typedef std::vector<MvRotatedBox> MvRBoxVector;

// write the pixel content of an image into a txt file
void mvDumpPixels (IplImage* img, const char* file_name, char delimiter = ',');
void mvDumpHistogram (IplImage* img, const char* file_name, char delimiter = ',');

/** mvHuMoments calculates the Hu moments for a bitmap image
 *  It is a wrapper for cvMoments and cvHuMoments
 */
void mvHuMoments(IplImage *src, double *hus);

/** mvCreateImage is a wrapper for cvCreateImage which drops the need for specifying
 *  depth and nChannels. The default mvCreateImage always returns a greyscale image 
 *  while mvCreateImage_Color always returns a color image;
 */
inline IplImage* mvCreateImageHeader () { // common size image
    unsigned width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
    return cvCreateImageHeader (cvSize(width,height), IPL_DEPTH_8U, 1);
}
inline IplImage* mvCreateImage () { // common size image
    unsigned width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
    return cvCreateImage (cvSize(width,height), IPL_DEPTH_8U, 1);
}
inline IplImage* mvCreateImage (CvSize size) { // specified size
    return cvCreateImage (size, IPL_DEPTH_8U, 1);
}
inline IplImage* mvCreateImage (unsigned width, unsigned height) { // specified size
    return cvCreateImage (cvSize(width,height), IPL_DEPTH_8U, 1);
}
inline IplImage* mvCreateImage (IplImage *img) { // size and origin
    IplImage* temp = cvCreateImage (cvGetSize(img), IPL_DEPTH_8U, 1);
    temp->origin = img->origin;
    return temp;
}

inline IplImage* mvCreateImage_Color () { // common size image
    unsigned width, height;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
    return cvCreateImage (cvSize(width,height), IPL_DEPTH_8U, 3);
}
inline IplImage* mvCreateImage_Color (CvSize size) {
    return cvCreateImage (size, IPL_DEPTH_8U, 3);
}
inline IplImage* mvCreateImage_Color (unsigned width, unsigned height) {
    return cvCreateImage (cvSize(width,height), IPL_DEPTH_8U, 3);
}
inline IplImage* mvCreateImage_Color (IplImage *img) {
    IplImage* temp = cvCreateImage (cvGetSize(img), img->depth, 3);
    temp->origin = img->origin;
    return temp;
}

/* These allow you access to static scratch images */
IplImage* mvGetScratchImage();
void mvReleaseScratchImage();
IplImage* mvGetScratchImage2();
void mvReleaseScratchImage2();
IplImage* mvGetScratchImage3();
void mvReleaseScratchImage3();
IplImage* mvGetScratchImage_Color();
void mvReleaseScratchImage_Color();

/*BGR2HSV -- NOTfaster implementation to convert BGR images into HSV format */
void mvBGR2HSV(IplImage* src, IplImage* dst);

inline void mvGaussian (IplImage* src, IplImage* dst, unsigned kern_w, unsigned kern_h) {
    cvSmooth (src, dst, CV_GAUSSIAN, kern_w, kern_h);
}
inline void mvDilate (IplImage* src, IplImage* dst, unsigned kern_w, unsigned kern_h, unsigned iterations=1) {
    IplConvKernel* kernel = cvCreateStructuringElementEx (kern_w, kern_h, (kern_w+1)/2, (kern_h+1)/2, CV_SHAPE_ELLIPSE);
    cvDilate (src, dst, kernel, iterations);
    cvReleaseStructuringElement (&kernel);
}
inline void mvErode (IplImage* src, IplImage* dst, unsigned kern_w, unsigned kern_h, unsigned iterations=1) {
    IplConvKernel* kernel = cvCreateStructuringElementEx (kern_w, kern_h, (kern_w+1)/2, (kern_h+1)/2, CV_SHAPE_ELLIPSE);
    cvErode (src, dst, kernel, iterations);
    cvReleaseStructuringElement (&kernel);
}
inline void mvOpen (IplImage* src, IplImage* dst, unsigned kern_w, unsigned kern_h, unsigned iterations=1) {
    IplConvKernel* kernel = cvCreateStructuringElementEx (kern_w, kern_h, (kern_w+1)/2, (kern_h+1)/2, CV_SHAPE_ELLIPSE);
    cvMorphologyEx (src, dst, NULL, kernel, CV_MOP_OPEN, iterations);
    cvReleaseStructuringElement (&kernel);
}
inline void mvClose (IplImage* src, IplImage* dst, unsigned kern_w, unsigned kern_h, unsigned iterations=1) {
    IplConvKernel* kernel = cvCreateStructuringElementEx (kern_w, kern_h, (kern_w+1)/2, (kern_h+1)/2, CV_SHAPE_ELLIPSE);
    cvMorphologyEx (src, dst, NULL, kernel, CV_MOP_CLOSE, iterations);
    cvReleaseStructuringElement (&kernel);
}
inline void mvGradient (IplImage* src, IplImage* dst, unsigned kern_w, unsigned kern_h, unsigned iterations=1) {
    IplImage* temp = cvCreateImage (cvGetSize(src), IPL_DEPTH_8U, 1);
    IplConvKernel* kernel = cvCreateStructuringElementEx (kern_w, kern_h, (kern_w+1)/2, (kern_h+1)/2, CV_SHAPE_ELLIPSE);

    cvMorphologyEx (src, dst, temp, kernel, CV_MOP_GRADIENT, iterations);
    
    cvReleaseImage (&temp);
    cvReleaseStructuringElement (&kernel);
}

/** Binary Filters */
// These are fast filters designed specifically for binary images with very
// few bright pixels
enum MV_KERNEL_SHAPE {MV_KERN_RECT, MV_KERN_ELLIPSE};
enum MV_MORPHOLOGY_TYPE {MV_DILATE, MV_ERODE, MV_OPEN, MV_CLOSE, MV_GRADIENT};

class mvBinaryMorphology {
	IplImage* temp;
	int kernel_width, kernel_height;
	unsigned kernel_area;
	int* kernel_point_array;

    PROFILE_BIN bin_morph;
    PROFILE_BIN bin_gradient;

	void mvBinaryMorphologyMain (MV_MORPHOLOGY_TYPE morphology_type, IplImage* src, IplImage* dst);

	public:
	mvBinaryMorphology (int Kernel_Width, int Kernel_Height, MV_KERNEL_SHAPE Shape);
	~mvBinaryMorphology ();

	void dilate (IplImage* src, IplImage* dst) {
          bin_morph.start();
		mvBinaryMorphologyMain (MV_DILATE, src, dst);
          bin_morph.stop();
	}
	void erode (IplImage* src, IplImage* dst) {
          bin_morph.start();
		mvBinaryMorphologyMain (MV_ERODE, src, dst);
          bin_morph.stop();
	}
	void close (IplImage* src, IplImage* dst) {
          bin_morph.start();
		mvBinaryMorphologyMain (MV_CLOSE, src, dst);
          bin_morph.stop();
	}
	void open (IplImage* src, IplImage* dst) {
          bin_morph.start();
		mvBinaryMorphologyMain (MV_OPEN, src, dst);
          bin_morph.stop();
	}
	void gradient (IplImage* src, IplImage* dst) {
          bin_gradient.start();
		mvBinaryMorphologyMain (MV_GRADIENT, src, dst);
          bin_gradient.stop();
	}
};

// Helper function for HSV conversion
void tripletBGR2HSV (uchar Blue, uchar Green, uchar Red, uchar &Hue, uchar &Sat, uchar &Val);
void tripletBGR2HSV (int Blue, int Green, int Red, int &Hue, int &Sat, int &Val);

#endif
