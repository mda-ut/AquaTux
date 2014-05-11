/** mvlines - mda Line Finding algorithm library
 *  Author - Ritchie Zhao
 *  July 2012
 */ 
#ifndef __MDA_VISION_MVLINES__
#define __MDA_VISION_MVLINES__ 

#include <cv.h>
#include "profile_bin.h"

#define LINE_THICKNESS 1
    
/** mvLines - object representing a collection of lines using the two endpoints */
/* When created, the class automatically allocates 6400 bytes of memory that it uses to 
 * store data. This can be recycled by calling clearData(); This memory cannot be reclaimed
 * without destroying the class instance.
 * 
 * Use nlines() to get the number of lines currently stored.
 * 
 * To access line data: lines[i] returns a 2 element array of CvPoint* => the 2 endpoints.
 * So lines[i][0] and lines[i][1] are the endpoints of ith line.
 * Use lines[i][0].x and lines[i][0].y to get x and y coordinates of an endpoint. 
 * 
 * Use drawOntoImage (img) to draw the lines onto an image.
 */
class mvLines {
    CvSeq* _data;
    CvMemStorage* _storage; // this stores actual line data in opencv
    
    friend class mvHoughLines;
    friend class mvKmeans;
    
    public:
    // the constructor allocates 6400 bytes of storage space, which is like 400 lines...
    mvLines () { _data=NULL; _storage=cvCreateMemStorage(6400); } 
    ~mvLines () { cvReleaseMemStorage(&_storage); }
    
    unsigned nlines () { return (_data != NULL) ? unsigned(_data->total) : 0; }
    void removeHoriz ();
    void removeVert ();
    void sortXY (); // each Horiz line has smaller X value first, each Vert smaller Y first
    void drawOntoImage (IplImage* img);
    
    CvPoint* operator [] (unsigned index) { return (CvPoint*)cvGetSeqElem(_data,index); }
    
    // note clearData does NOT deallocate memory, it only allows recycling of used memory. 
    void clearData () { 
        if (_data) {
            cvClearSeq(_data); 
            _data=NULL; 
            cvClearMemStorage(_storage);
        }
    } 
};

/** mvHoughLines - Hough Line finding filter */
// you must provide an mvLines object for it to work 
class mvHoughLines {
    unsigned PIX_RESOLUTION;
    float ANG_RESOLUTION; // ang_resolution is in radians
    float _ACCUMULATOR_THRESHOLD_;
    float _MIN_LINE_LENGTH_, _MIN_COLINEAR_LINE_DIST_;

    PROFILE_BIN bin_findLines;
    
    public:
    mvHoughLines (const char* settings_file);
    /* use default destructor */
    void findLines (IplImage *img, mvLines* lines);
};



/** The following code deal with K-Means clustering */
#define GLOBAL_INT_FACTOR 100.0  // factor to keep all numbers integer

class mvKMeans {
    static const unsigned MAX_CLUSTERS = 10;
    static const float SINGLE_LINE__CLUSTER_INTRA_DIFF = 200;      // intra cluster diff if cluster has 1 line
    static const float SINGLE_CLUSTER__INTER_DIFF = 12000;         // inter cluster diff if only 1 cluster
    
    // The below _Clusters_**** are arrays of CvPoint[2]. Each CvPoint[2] which store the beginning 
    // and end points of a line. The lines are the "clusters" we are trying to get and each array
    // is an intermediate variable to get _Clusters_Best
    //
    // _Seed stores the starting cluster, which we use to bin the lines to. Note each element
    //      will be used to point to an existing line, so no need to allocate this
    // _Temp store the clusters and they grow during the binning process. 
    //
    // _Best stores the most suitable cluster configuration (based on nClusters). It will point
    //      to _Temp if _Temp is deemed the best so far.
    CvPoint* _Clusters_Seed[MAX_CLUSTERS];  
    CvPoint* _Clusters_Temp[MAX_CLUSTERS];
    CvPoint* _Clusters_Best[MAX_CLUSTERS];  
    
    mvLines* _Lines;                        // points to lines we are try to cluster
    unsigned _nLines;
    unsigned _nClusters_Final; // only used for storing the number of clusters after the algorithm
    
    private:    
    // helper functions
    unsigned Get_Line_ClusterSeed_Diff (unsigned cluster_index, unsigned line_index);
    
    // steps in the algorithm
    void KMeans_CreateStartingClusters (unsigned nClusters);
    float KMeans_Cluster (unsigned nClusters, unsigned iterations);
    
    
    public:
    mvKMeans ();
    ~mvKMeans ();

    unsigned nClusters () { return _nClusters_Final; }
    void printClusters ();

    // this function does all the clustering, trying each value of nclusters between the min and max specified
    int cluster_auto (unsigned nclusters_min, unsigned nclusters_max, mvLines* lines, unsigned iterations=1);
    void clearData () { _nClusters_Final = 0; }
    
    // accessor function for the lines
    CvPoint* operator [] (unsigned index) { return (CvPoint*)_Clusters_Best[index]; }
    
    void drawOntoImage (IplImage* img);
};


/** Helper functions, even for stuff outside of these classes */
inline 
unsigned line_sqr_length (const CvPoint* line) {
    int dx = (line[1].x - line[0].x);
    int dy = (line[1].y - line[0].y);
    return unsigned(dx*dx + dy*dy);
}
inline 
float line_angle_to_vertical (const CvPoint* line) {
    int dx = (line[1].x - line[0].x);
    int dy = (line[1].y - line[0].y);
    if (dy == 0)
        return 0;
    else
        return atan((float)dx/dy);
}

#endif
