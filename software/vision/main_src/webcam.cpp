#include <stdio.h>
#include <cv.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mgui.h"
#include "mv.h"
#include "mvColorFilter.h"
#include "mvLines.h"
#include "mvContours.h"
#include "../../tasks/mda_vision.h"
#include "profile_bin.h"

unsigned CAM_NUMBER=0;
unsigned WRITE=0;
unsigned TEST=0;
unsigned GRAD=0;
unsigned WATERSHED=0;
unsigned LINE=0;
unsigned CIRCLE=0;
unsigned RECT=0;
unsigned LOAD=0;
unsigned BREAK=0;
unsigned GATE=0;
unsigned PATH=0;
unsigned BUOY=0;
unsigned GOALPOST=0;
unsigned TIMEOUT=0;
unsigned POS=0;


int main( int argc, char** argv ) {
    unsigned long nframes = 0, t_start, t_end;
    
    if (argc == 1) 
        printf ("For options use --help\n\n");

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i],"1") || !strcmp(argv[i],"2"))
            CAM_NUMBER = atoi (argv[i]);
        else if (!strcmp (argv[i], "--test"))
            TEST = 1;
        else if (!strcmp (argv[i], "--grad"))
            GRAD = 1;
        else if (!strcmp (argv[i], "--watershed"))
            WATERSHED = 1;
        else if (!strcmp (argv[i], "--write"))
            WRITE = 1;
        else if (!strcmp (argv[i], "--break"))
            BREAK = 1;
        else if (!strcmp (argv[i], "--line"))
            LINE = 1;
        else if (!strcmp (argv[i], "--circle"))
            CIRCLE = 1;
        else if (!strcmp (argv[i], "--rect"))
            RECT = 1;
        else if (!strcmp (argv[i], "--gate"))
            GATE = 1;
        else if (!strcmp (argv[i], "--path"))
            PATH = 1;
        else if (!strcmp (argv[i], "--buoy"))
            BUOY = 1;
        else if (!strcmp (argv[i], "--goalpost"))
            GOALPOST = 1;
        else if (!strcmp (argv[i], "--timeout"))
            TIMEOUT = atoi(argv[++i]);
        else if (!strcmp (argv[i], "--pos"))
            POS = atoi(argv[++i]); 
        else if (!strcmp (argv[i], "--load"))
            LOAD = ++i; // put the next argument index into LOAD
        else if (!strcmp (argv[i], "--help")) {
            printf ("OpenCV based webcam program. Hit 'q' to exit. Defaults to cam0, writes to \"webcam.avi\"\n");
            printf ("Put any integer as an argument (without --) to use that as camera number\n\n");
            printf ("  --write\n    Write captured video to file.\n\n");
            printf ("  --load\n    Use a video file (following --load) instead of a webcam.\n\n");
            printf ("  --line\n    Run line finding code.\n\n");
            printf ("  --circle\n    Run circle finding code.\n\n");
            printf ("  --break\n    Pause the input (press any key to go to the next frame).\n\n");
            printf ("  Example: `./webcam 1 --write` will use cam1, and will write to disk\n\n");
            return 0;
        }
    }

    /// initialization
    // init camera
    mvCamera* camera = NULL;
    if (LOAD == 0) {
        camera = new mvCamera (CAM_NUMBER);
    }
    else {
        camera = new mvCamera (argv[LOAD]);
    }
    if (camera != 0 && POS > 0) {
        camera->set_relative_position(static_cast<double>(POS)/1000);
    }

    mvVideoWriter* frame_writer = NULL;
    mvVideoWriter* filter_writer_1 = NULL;
    mvVideoWriter* filter_writer_2 = NULL;
    if (WRITE) {
        frame_writer = new mvVideoWriter ("frames.avi");
        filter_writer_1 = new mvVideoWriter ("filtered_1.avi");
        filter_writer_2 = new mvVideoWriter ("filtered_2.avi");
    }

    // init windows
    mvWindow* win1 = new mvWindow ("webcam");
    mvWindow* win2 = new mvWindow ("win2");
    mvWindow* win3 = new mvWindow ("win3");

    // declare filters we need
    mvHSVFilter HSVFilter ("HSVFilter_settings.csv"); // color filter
    mvBinaryMorphology Morphology7 (9,9, MV_KERN_ELLIPSE);
    mvBinaryMorphology Morphology5 (5,5, MV_KERN_ELLIPSE);
    mvHoughLines HoughLines ("HoughLines_settings.csv");
    mvLines lines; // data struct to store lines
    mvKMeans kmeans;
    mvWatershedFilter watershed_filter;
    mvContours contour_filter;

    MDA_VISION_MODULE_GATE* gate=GATE? new MDA_VISION_MODULE_GATE : 0;
    MDA_VISION_MODULE_PATH* path=PATH? new MDA_VISION_MODULE_PATH : 0;
    MDA_VISION_MODULE_BUOY* buoy=BUOY? new MDA_VISION_MODULE_BUOY : 0;
    MDA_VISION_MODULE_GOALPOST* goalpost=GOALPOST? new MDA_VISION_MODULE_GOALPOST : 0;

    // declare images we need
    IplImage* scratch_color = mvCreateImage_Color();
    IplImage* scratch_color_2 = mvCreateImage_Color();
    IplImage* filter_img = mvCreateImage ();
    IplImage* filter_img_2 = mvCreateImage ();
 
    /// execution
    char c = 0;
    IplImage* frame;

    t_start = clock();

    for (;;) {
        frame = camera->getFrameResized(); // read frame from cam
        if (LOAD) {
            usleep(17000);
        }
	if (TIMEOUT > 0) {
	    if ((clock() - t_start)/CLOCKS_PER_SEC > TIMEOUT) {
	         break;
            }
	}

        if (!frame) {
            printf ("Video Finished.\n");
            break;
        }
        
        if (nframes < 10) {
            nframes++;
            continue;
        }
 
        cvCopy (frame, scratch_color);
        win1->showImage (scratch_color);
        
        if (TEST) {             
        }
        else if (GATE) {
            if (gate->filter (frame) == FULL_DETECT) {
                cvWaitKey(200);
            }
        }
        else if (PATH) {
            if (path->filter (frame) == FULL_DETECT) {
                cvWaitKey(00);
            }
        }
        else if (BUOY) {
            if (buoy->filter (frame) == FULL_DETECT) {
                cvWaitKey(200);
            }
        }
        else if (GOALPOST) {
            if (goalpost->filter (frame) == FULL_DETECT) {
                cvWaitKey(200);
            }
        }
        else if (WATERSHED) {
            watershed_filter.watershed(frame, filter_img);
            win1->showImage (frame);
            win2->showImage (filter_img);
            
            COLOR_TRIPLE color;
            MvCircle circle;
            MvCircleVector circle_vector;
            MvRotatedBox rbox;
            MvRBoxVector rbox_vector;
            
            while ( watershed_filter.get_next_watershed_segment(filter_img_2, color) ) {
                if (CIRCLE) {
                    contour_filter.match_circle(filter_img_2, &circle_vector, color);
                }
                else if (RECT) {
                    //contour_filter.match_rectangle(filter_img_2, &rbox_vector, color, params);
                }
/*
                win3->showImage(filter_img_2);
                cvWaitKey(200);
*/              
            }

            if (CIRCLE && circle_vector.size() > 0) {
                MvCircleVector::iterator iter = circle_vector.begin();
                MvCircleVector::iterator iter_end = circle_vector.end();
                int index = 0;
                printf ("%d Circles Detected:\n", static_cast<int>(circle_vector.size()));
                for (; iter != iter_end; ++iter) {
                    printf ("\tCircle #%d: (%3d,%3d), Rad=%5.1f, <%3d,%3d,%3d>\n", ++index,
                        iter->center.x, iter->center.y, iter->radius, iter->m1, iter->m2, iter->m3);
                    iter->drawOntoImage(filter_img_2);
                }
                
                win3->showImage (filter_img_2);
                cvWaitKey(200);
            }
            else if (RECT && rbox_vector.size() > 0) {
                MvRBoxVector::iterator iter = rbox_vector.begin();
                MvRBoxVector::iterator iter_end = rbox_vector.end();
                int index = 0;
                printf ("%d Rectangles Detected:\n", static_cast<int>(rbox_vector.size()));
                for (; iter != iter_end; ++iter) {
                    printf ("\tRect #%d: (%3d,%3d), Len=%5.1f, Width=%5.1f, Angle=%5.1f <%3d,%3d,%3d>\n", ++index, 
                        iter->center.x, iter->center.y, iter->length, iter->width, iter->angle, iter->m1, iter->m2, iter->m3);
                    iter->drawOntoImage(filter_img_2);
                }
                
                win3->showImage (filter_img_2);
                cvWaitKey(200);
            }
        }

        if (GRAD) {
            Morphology5.gradient(filter_img, filter_img);
        }

        if (LINE) {
            HoughLines.findLines (filter_img, &lines);
            kmeans.cluster_auto (1, 8, &lines, 1);
        
            //lines.drawOntoImage (filter_img);
            kmeans.drawOntoImage (filter_img);
            lines.clearData(); // erase line data and reuse allocated mem
            //kmeans.clearData();
            
            win3->showImage (filter_img);
        }
        /*
        else if (CIRCLE) {
            CvPoint centroid;
            float radius;
            contour_filter.match_circle(filter_img, centroid, radius);
            contour_filter.drawOntoImage(filter_img);
            win3->showImage (filter_img);
        }
        else if (RECT) {
            CvPoint centroid;
            float length, angle;
            contour_filter.match_rectangle(filter_img, centroid, length, angle);
            contour_filter.drawOntoImage(filter_img);
            win3->showImage (filter_img);
        }
        */
        if (WRITE) {
            frame_writer->writeFrame (frame);
            cvCvtColor (filter_img, scratch_color, CV_GRAY2BGR);
            filter_writer_1->writeFrame (scratch_color);
            cvCvtColor (filter_img_2, scratch_color, CV_GRAY2BGR);
            filter_writer_2->writeFrame (scratch_color);
        }

        nframes++;
        if (BREAK)
            c = cvWaitKey(0);
        else if (LOAD)
            c = cvWaitKey(3); // go for about 15 frames per sec
        else
            c = cvWaitKey(5);

        if (c == 'q')
            break;
        else if (c == 'w') {
            mvDumpPixels (frame, "webcam_img_pixel_dump.csv");
            mvDumpHistogram (frame, "webcam_img_histogram_dump.csv");
        }
    }
    
    t_end = clock ();
    printf ("\nAverage Framerate = %f\n", (float)nframes/(t_end - t_start)*CLOCKS_PER_SEC);
    
    cvReleaseImage (&scratch_color);
    cvReleaseImage (&scratch_color_2);
    cvReleaseImage (&filter_img);
    cvReleaseImage (&filter_img_2);
    delete camera;
    delete frame_writer;
    delete filter_writer_1;
    delete filter_writer_2;
    delete win1;
    delete win2;
    delete win3;
    return 0;
}
