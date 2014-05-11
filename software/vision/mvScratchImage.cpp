#include "mv.h"

// Uncomment to disable scratch images (more memory usage, but better for debugging)
//#define DISABLE_SCRATCH_IMAGE

static IplImage* scratch_img_1channel = NULL;
static int instances_1channel = 0;
static IplImage* scratch_img_1channel_2 = NULL;
static int instances_1channel_2 = 0;
static IplImage* scratch_img_1channel_3 = NULL;
static int instances_1channel_3 = 0;

static IplImage* scratch_img_3channel = NULL;
static int instances_3channel = 0;

IplImage* mvGetScratchImage() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_1channel == 0) {
        assert (scratch_img_1channel == NULL);
#endif
        instances_1channel++;
        scratch_img_1channel = mvCreateImage();
        return scratch_img_1channel;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else {
        instances_1channel++;
        return scratch_img_1channel;
    }
#endif
}

void mvReleaseScratchImage() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_1channel == 1) {
#endif
        cvReleaseImage(&scratch_img_1channel);
        scratch_img_1channel = NULL;
        instances_1channel = 0;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else if (instances_1channel >= 1) {
        instances_1channel--;
    }
    else {
        fprintf (stderr, "Tried to release 1 channel image #2 without getting it first\n");
        exit (1);
    }
#endif
}

IplImage* mvGetScratchImage2() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_1channel_2 == 0) {
        assert (scratch_img_1channel_2 == NULL);
#endif
        instances_1channel_2++;
        scratch_img_1channel_2 = mvCreateImage();
        return scratch_img_1channel_2;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else {
        instances_1channel_2++;
        return scratch_img_1channel_2;
    }
#endif
}

void mvReleaseScratchImage2() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_1channel_2 == 1) {
#endif
        cvReleaseImage(&scratch_img_1channel_2);
        scratch_img_1channel_2 = NULL;
        instances_1channel_2 = 0;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else if (instances_1channel_2 >= 1) {
        instances_1channel_2--;
    }
    else {
        fprintf (stderr, "Tried to release 1 channel image #2 without getting it first\n");
        exit (1);
    }
#endif
}

IplImage* mvGetScratchImage3() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_1channel_3 == 0) {
        assert (scratch_img_1channel_3 == NULL);
#endif
        instances_1channel_3++;
        scratch_img_1channel_3 = mvCreateImage();
        return scratch_img_1channel_3;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else {
        instances_1channel_3++;
        return scratch_img_1channel_3;
    }
#endif
}

void mvReleaseScratchImage3() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_1channel_3 == 1) {
#endif
        cvReleaseImage(&scratch_img_1channel_3);
        scratch_img_1channel_3 = NULL;
        instances_1channel_3 = 0;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else if (instances_1channel_3 >= 1) {
        instances_1channel_3--;
    }
    else {
        fprintf (stderr, "Tried to release 1 channel image #3 without getting it first\n");
        exit (1);
    }
#endif
}

IplImage* mvGetScratchImage_Color() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_3channel == 0) {
        assert (scratch_img_3channel == NULL);
#endif
        instances_3channel++;
        scratch_img_3channel = mvCreateImage_Color();
        return scratch_img_3channel;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else {
        instances_3channel++;
        return scratch_img_3channel;
    }
#endif
}

void mvReleaseScratchImage_Color() {
#ifndef DISABLE_SCRATCH_IMAGE
    if (instances_3channel == 1) {
#endif
        cvReleaseImage (&scratch_img_3channel);
        scratch_img_3channel = NULL;
        instances_3channel = 0;
#ifndef DISABLE_SCRATCH_IMAGE
    }
    else if (instances_3channel > 1) {
        instances_3channel--;
    }
    else {
        fprintf (stderr, "Tried to release 1 channel image without getting it first\n");
        exit (1);
    }
#endif
}
