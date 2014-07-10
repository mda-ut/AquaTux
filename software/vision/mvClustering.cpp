#include "mvLines.h"
#include "mgui.h"
#include <stdio.h>
#include <cv.h>

//#define CREATE_STARTING_CLUSTERS_APPROX
//#define CLUSTERING_DEBUG

#define ABS(X) (((X) > 0) ? (X) : (-(X)))
#define SQR(X) ((X)*(X))
//#define CLUSTERING_DEBUG
#ifdef CLUSTERING_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

/** Helper Functions */
inline
unsigned Line_Difference_Metric (int x1,int y1, int x2,int y2, int x3,int y3, int x4,int y4) {

    // this is 8x the area of the quadrilateral
    int a123 = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1);
    int a134 = (x3-x1)*(y4-y1) - (x4-x1)*(y3-y1);
    int a124 = (x2-x1)*(y4-y1) - (x4-x1)*(y2-y1);
    int a234 = (x3-x2)*(y4-y2) - (x4-x2)*(y3-y2);
    
    return GLOBAL_INT_FACTOR*(ABS(a123) + ABS(a134) + ABS(a124) + ABS(a234));

   /* float d1 = SQR(x3-x1) + SQR(y3-y1); // distance between first points of each line
    float d2 = SQR(x4-x2) + SQR(y4-y2); // second points
    unsigned L = SQR(x2-x1) + SQR(y2-y1) + SQR(x4-x3) + SQR(y4-y3); // length of lines
    //unsigned L = ABS(x2-x1) + ABS(y2-y1) + ABS(x4-x3) + ABS(y4-y3); // length of lines
    if (L == 0)
        return 100000;
    return (unsigned)(GLOBAL_INT_FACTOR * (d1+d2) / L);*/
}

inline 
unsigned Line_Difference_Metric (const CvPoint* line1, const CvPoint* line2) {
// overloaded version used for two CvPoint*'s
    return Line_Difference_Metric (
        line1[0].x, line1[0].y,   line1[1].x, line1[1].y,
        line2[0].x, line2[0].y,   line2[1].x, line2[1].y
    );
}

inline
unsigned mvKMeans::Get_Line_ClusterSeed_Diff (unsigned cluster_index, unsigned line_index) {
// this function first reads the Cluster_Line_Diff value cached in the matrix, if it is -1
// that means the value wasnt calculated. So we calculate and store the value
    return Line_Difference_Metric (
        _Clusters_Seed[cluster_index][0].x, _Clusters_Seed[cluster_index][0].y,
        _Clusters_Seed[cluster_index][1].x, _Clusters_Seed[cluster_index][1].y,
        (*_Lines)[line_index][0].x, (*_Lines)[line_index][0].y,
        (*_Lines)[line_index][1].x, (*_Lines)[line_index][1].y
    );
}

inline
void copy_line (const CvPoint* src, CvPoint* dst) {
    dst[0].x = src[0].x;
    dst[0].y = src[0].y;
    dst[1].x = src[1].x;
    dst[1].y = src[1].y;
}

/** True methods of KMeans Algorithm */

mvKMeans::mvKMeans () {   
    unsigned width;
    read_common_mv_setting ("IMG_WIDTH_COMMON", width);
    
    for (unsigned i = 0; i < MAX_CLUSTERS; i++) {
        _Clusters_Seed[i] = new CvPoint[2];
        _Clusters_Temp[i] = new CvPoint[2];
        _Clusters_Best[i] = new CvPoint[2];
    }
    
    _Lines = NULL;
    _nLines = 0;
    _nClusters_Final = 0;
}

mvKMeans::~mvKMeans () {
    for (unsigned i = 0; i < MAX_CLUSTERS; i++) {
        delete [] _Clusters_Seed[i];
        delete [] _Clusters_Temp[i];
        delete [] _Clusters_Best[i];
    }
}

void mvKMeans::KMeans_CreateStartingClusters (unsigned nClusters) {
/* creates N starting clusters for the KMeans algorithm. A "cluster" really means the
 * start/endpoints of the line that other lines can bin to. The clusters are chosen thus:
 * The first cluster is just the strongest line (line[0] from HoughLines)
 * The n+1'th cluster is the line which is most different from existing clusters. This is
 * calculated as the line whose minimum difference wrt existing clusters is the greatest.
 */   
    bool line_already_chosen[_nLines];
    for (unsigned i = 0; i < _nLines; i++)
        line_already_chosen[i] = false;
    
    /// choose first seed cluster, then 2nd, etc in this loop
    for (unsigned N = 0; N < nClusters; N++) {  
        //DEBUG_PRINT ("Choosing cluster %d\n", N);
        
        /// For first cluster choose line[0];
        if (N == 0) {
            // I cant believe this is what happens when you pass in pointers like you are supposed to
            //_Clusters_Seed[N] = (*_Lines)[0];     // pointer copy 
            copy_line ((*_Lines)[0], _Clusters_Seed[N]);
            continue;
        }
        
        /// For other clusters, loop over each line and chose a line based on the method specified
        unsigned next_cluster_index = 0;       
        unsigned next_cluster_max_diff = 0;
        
        #ifdef CREATE_STARTING_CLUSTERS_APPROX   /// find the line most different from the last line chosen as a cluster
        
        for (unsigned line_index = 0; line_index < _nLines; line_index++) { 
            if (line_already_chosen[line_index] == true) continue;
            
            // calculate the diff between this line and last selected cluster
            unsigned diff_from_last_cluster = Get_Line_ClusterSeed_Diff (N-1, line_index);
            
            if (diff_from_last_cluster > next_cluster_max_diff) {
                next_cluster_max_diff = diff_from_last_cluster;
                next_cluster_index = line_index;
            }
        }
        
        #else                                    /// find the line whose min diff from any of the prev clusters is greatest
        
        for (unsigned line_index = 0; line_index < _nLines; line_index++) { 
            if (line_already_chosen[line_index] == true) continue;
            
            unsigned long min_diff_from_cluster = 1E9;       // init this to a huge number
            // loop to find min_diff_from_cluster
            for (unsigned cluster_index = 0; cluster_index < N; cluster_index++) {   
                unsigned diff_from_cluster_i = Get_Line_ClusterSeed_Diff (cluster_index, line_index);
                //DEBUG_PRINT ("      Cluster %d - line %d: diff = %d\n", cluster_index,line_index,diff_from_cluster_i);
                
                if (diff_from_cluster_i < min_diff_from_cluster)
                    min_diff_from_cluster = diff_from_cluster_i;
            }
            
            if (min_diff_from_cluster > next_cluster_max_diff) {
                next_cluster_max_diff = min_diff_from_cluster;
                next_cluster_index = line_index;
            }
        }
        
        #endif
        
        /// now copy the data for the line chosen to be the next cluster
        //printf ("  Using line %d for cluster %d\n", next_cluster_index, N);
        copy_line ((*_Lines)[next_cluster_index], _Clusters_Seed[N]); // copy starting point
        line_already_chosen[next_cluster_index] = true;
    }
}

float mvKMeans::KMeans_Cluster (unsigned nClusters, unsigned iterations) {
/* bins each line to the closest starting/seed cluster, then averages the bins to get the starting/seed cluster
 * for the next iteration.
 * We also calculate a "validity" score for the clustering configuration. validity is defined as "avg_intra_cluster_diff"
 * divided by "minimum_inter_cluster_diff". Essentially it is the spread of lines in a cluster divided by the spread
 * amongst clusters. The smaller the validity the better.
 * Note minimum_inter_cluster_diff is not defined for nClusters == 1, so we wing it. See below. 
 */    
    unsigned weight;
    unsigned short cluster_of_line[_nLines]; // stores which cluster a line is binned to
    unsigned lines_per_cluster[nClusters];
    unsigned weight_per_cluster[nClusters];    
    
    for (unsigned iter = 0; iter < iterations; iter++) {       
        /// first clear the temp clusters
        for (unsigned i = 0; i < nClusters; i++) {   
            _Clusters_Temp[i][0] = cvPoint (0,0);
            _Clusters_Temp[i][1] = cvPoint (0,0);
            weight_per_cluster[i] = 0;
            lines_per_cluster[i] = 0;
        }

        /// loop over each line and bin that line to a cluster
        for (unsigned line_index = 0; line_index < _nLines; line_index++) {
            unsigned closest_cluster_index = 0;
            unsigned long closest_cluster_diff = 1E9;
            unsigned cluster_diff_i;
            
            /// go thru each cluster and find the one that is closest (least diff) wrt this line
            for (unsigned cluster_index = 0; cluster_index < nClusters; cluster_index++) {
                cluster_diff_i = Get_Line_ClusterSeed_Diff (cluster_index, line_index);
                
                if (cluster_diff_i < closest_cluster_diff) {
                    closest_cluster_diff = cluster_diff_i;
                    closest_cluster_index = cluster_index;
                }
            }

            /// rejection routine: if the closest_cluster_diff is too high, simply ignore this line


            /// add the line to the temp cluster
            weight = line_sqr_length((*_Lines)[line_index]);
            _Clusters_Temp[closest_cluster_index][0].x += weight * ((*_Lines)[line_index][0].x);
            _Clusters_Temp[closest_cluster_index][0].y += weight * ((*_Lines)[line_index][0].y);
            _Clusters_Temp[closest_cluster_index][1].x += weight * ((*_Lines)[line_index][1].x);
            _Clusters_Temp[closest_cluster_index][1].y += weight * ((*_Lines)[line_index][1].y);

            lines_per_cluster[closest_cluster_index]++;
            weight_per_cluster[closest_cluster_index] += weight;
            cluster_of_line[line_index] = closest_cluster_index; /*****************/
        }
        
        /// calculate the next iteration's clusters and copy them over to clusters_seed
        for (unsigned i = 0; i < nClusters; i++) {
            if (weight_per_cluster[i] == 0) continue;
            
            _Clusters_Temp[i][0].x /= weight_per_cluster[i];
            _Clusters_Temp[i][0].y /= weight_per_cluster[i];
            _Clusters_Temp[i][1].x /= weight_per_cluster[i];
            _Clusters_Temp[i][1].y /= weight_per_cluster[i];
            
            copy_line (_Clusters_Temp[i], _Clusters_Seed[i]);
        }
    }
    
    float avg_intra_cluster_diff = 0.0;
    //float avg_inter_cluster_diff = 0.0;
    float minimum_inter_cluster_diff = 1E12;

    /// calculate avg_intra_cluster_diff
    if (nClusters == _nLines) {
        avg_intra_cluster_diff = GLOBAL_INT_FACTOR * SINGLE_LINE__CLUSTER_INTRA_DIFF;
    }
    else {
        for (unsigned line_index = 0; line_index < _nLines; line_index++) {
            avg_intra_cluster_diff += Get_Line_ClusterSeed_Diff (cluster_of_line[line_index], line_index);
        }
    }
    /*for (unsigned i = 0, product_of_lines_per_cluster = 1; i < nClusters; i++) {
        product_of_lines_per_cluster *= lines_per_cluster[i];
    }*/
    avg_intra_cluster_diff = avg_intra_cluster_diff / _nLines;//product_of_lines_per_cluster / nClusters;
    
    /// now calculate the minimum_inter_cluster_diff and the validity score
    if (nClusters == 1) {
        // this equation basically says "when comparing 1 cluster vs 2 clusters, the 2 clusters have to be a ratio
        // of perpendicular dist over length of less than 0.2 to be better than the 1 cluster configuration
        minimum_inter_cluster_diff = GLOBAL_INT_FACTOR * SINGLE_CLUSTER__INTER_DIFF; // * line_sqr_length(_Clusters_Seed[0]);
        //avg_inter_cluster_diff = GLOBAL_INT_FACTOR * SINGLE_CLUSTER__INTER_DIFF; // * line_sqr_length(_Clusters_Seed[0]);
    }
    else {
        for (unsigned i = 0; i < nClusters; i++) { // loop over all combinations of clusters, find min_inter_cluster_diff
            for (unsigned j = i+1; j < nClusters; j++) {
                float temp_diff = Line_Difference_Metric (_Clusters_Seed[i], _Clusters_Seed[j]);
                if (temp_diff < minimum_inter_cluster_diff)
                    minimum_inter_cluster_diff = temp_diff;
                //avg_inter_cluster_diff += temp_diff;
            }
        }
    }

    DEBUG_PRINT ("  nClusters = %d.  intra = %f,  inter = %f\n", nClusters, avg_intra_cluster_diff, minimum_inter_cluster_diff);
    return avg_intra_cluster_diff / minimum_inter_cluster_diff; 
}

int mvKMeans::cluster_auto (unsigned nclusters_min, unsigned nclusters_max, mvLines* lines, unsigned iterations) {
    _Lines = lines; 
    _nLines = _Lines->nlines();
    assert (nclusters_min > 0 && nclusters_max <= MAX_CLUSTERS);
    assert (nclusters_min <= nclusters_max);
    
    /// if the nlines < nclusters then quit (with the clusters being NULL)
    DEBUG_PRINT ("_nLines = %d\n", _nLines);
    if (_nLines < nclusters_min) { 
        DEBUG_PRINT ("Not enough lines for %d clusters.\n", nclusters_min);
        return 1;
    }
    
    /// make the lines all "flow" in 1 direction so we can add them up without cancelling
    _Lines->sortXY ();
    
    /// we now run the algorithm
    float min_validity = 1E20;
    
    for (unsigned N = nclusters_min; N <= nclusters_max; N++) {
        // create required num of clusters
        KMeans_CreateStartingClusters (N);
        
        // run the clustering algorithm through the desired num of iterations
        float validity = KMeans_Cluster (N, iterations);
        DEBUG_PRINT ("  nClusters = %d. validity = %f\n", N, validity);

        // check validity. If better than current validity pointer copy temp to best and
        // reallocate temp. Otherwise leave temp and it will be overwritten next iteration
        if (validity < min_validity) {
            min_validity = validity;
            _nClusters_Final = N;
            for (unsigned i = 0; i < N; i++) {
                copy_line (_Clusters_Seed[i], _Clusters_Best[i]);
            }
        }
    }
    
    DEBUG_PRINT ("Final nClusters = %d, validity = %f\n", _nClusters_Final, min_validity);

    /// cleanup
    _Lines = NULL;
    _nLines = 0;
    return 0;
}

void mvKMeans::drawOntoImage (IplImage* img) {
    assert (img != NULL);
    //assert (img->nChannels == 1);
    
    for (unsigned i = 0; i < _nClusters_Final; i++) {
        if (_Clusters_Best[i] != NULL)
        	cvLine (img, _Clusters_Best[i][0],_Clusters_Best[i][1], CV_RGB(100,100,100), 2*LINE_THICKNESS);
    }
}

void mvKMeans::printClusters () {
    for (unsigned i = 0; i < _nClusters_Final; i++) {
        if (_Clusters_Best[i] != NULL)
            printf ("Cluster %d:  (%d,%d) - (%d,%d)\n", i,_Clusters_Best[i][0].x,_Clusters_Best[i][0].y,_Clusters_Best[i][1].x,_Clusters_Best[i][1].y);
    }
}
