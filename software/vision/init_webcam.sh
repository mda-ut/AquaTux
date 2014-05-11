#!/bin/sh

v4l2ctrl -l webcam_video.config -d /dev/video0
v4l2ctrl -l webcam_video.config -d /dev/video1
v4l2ctrl -l webcam_video.config -d /dev/video2
