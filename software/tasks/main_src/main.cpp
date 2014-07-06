#include <stdlib.h>
#include <stdio.h>

#include "mda_vision.h"
#include "mda_tasks.h"

// This is just a program to test proper linking of code in task

#define IMAGE_NAME "../vision/color1.jpg"

int main (int argc, char** argv) {
    IplImage * raw_img = cvLoadImage(IMAGE_NAME);
    IplImage * img = mvCreateImage_Color();

    cvResize (raw_img, img);

    MDA_VISION_MODULE_BASE* test_module = NULL;
    MDA_VISION_MODULE_BASE* gate_module = NULL;
    MDA_VISION_MODULE_BASE* path_module = NULL;
    MDA_VISION_MODULE_BASE* buoy_module = NULL;
    MDA_VISION_MODULE_BASE* goalpost_module = NULL;
    
    test_module = new MDA_VISION_MODULE_TEST;
    gate_module = new MDA_VISION_MODULE_GATE;
    path_module = new MDA_VISION_MODULE_PATH;
    buoy_module = new MDA_VISION_MODULE_BUOY;
    goalpost_module = new MDA_VISION_MODULE_GOALPOST;


    test_module->filter (img);
    printf ("test module passed\n");
    gate_module->filter (img);
    printf ("gate module passed\n");
    path_module->filter (img);
    printf ("path module passed\n");
    buoy_module->filter (img);
    printf ("buoy module passed\n");
    goalpost_module->filter (img);
    printf ("goalpost module passed\n");

    cvWaitKey(500);
    delete test_module;
    cvWaitKey(500);
    delete gate_module;
    cvWaitKey(500);
    delete path_module;
    cvWaitKey(500);
    delete buoy_module;
    cvWaitKey(500);
    delete goalpost_module;
    cvWaitKey(500);


    cvReleaseImage (&raw_img);
    cvReleaseImage (&img);

    printf ("\nTest PASSED.\n");
    return 0;
}
