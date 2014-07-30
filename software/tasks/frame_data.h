#ifndef __MDA_VISION__MDA_FRAMEDATA__
#define __MDA_VISION__MDA_FRAMEDATA__

#include "mgui.h"
#include "mv.h"

// this class stores data that is remembered between frames for buoy
class MDA_FRAME_DATA {
public:
    // valid if one or more objects found
    bool circle_valid;
    bool rboxes_valid[2];
    bool frame_has_data;

    // each buoy frame can have a single circle
    MvCircle m_frame_circle;
    
    // each buoy frame can have 2 boxes. If one box is found it always goes into m_frame_boxes[0]
    // otherwise the left box goes in [0] and right box goes in [1]
    MvRotatedBox m_frame_boxes[2];

    MDA_FRAME_DATA () { frame_has_data = circle_valid = rboxes_valid[0] = rboxes_valid[1] = false; }
    
    MDA_FRAME_DATA& operator = (MDA_FRAME_DATA right) {
        this->circle_valid = right.circle_valid;
        this->rboxes_valid[0] = right.rboxes_valid[0];
        this->rboxes_valid[1] = right.rboxes_valid[1];
        this->m_frame_circle = right.m_frame_circle;
        this->m_frame_boxes[0] = right.m_frame_boxes[0];
        this->m_frame_boxes[1] = right.m_frame_boxes[1];
        return *this;
    }

    // assignment by checking validity
    // a newly created circle/rect has validity of -1
    bool assign_circle_by_validity (MvCircle circle) {
        frame_has_data = true;
        if (circle.validity > m_frame_circle.validity) {
            m_frame_circle = circle;
            circle_valid = true;
            return true;
        }
        return false;
    }
    bool assign_rbox_by_validity (MvRotatedBox rbox) {
        frame_has_data = true;
        if (rbox.validity > m_frame_boxes[0].validity-1) 
        {
            m_frame_boxes[1] = m_frame_boxes[0];
            rboxes_valid[1] = rboxes_valid[0];
            m_frame_boxes[0] = rbox;
            rboxes_valid[0] = true;
            return true;
        }
        else if (rbox.validity > m_frame_boxes[1].validity-1) {
            m_frame_boxes[1] = rbox;
            rboxes_valid[1] = true;
            return true;
        }
        return false;
    }
    void sort_rbox_by_x () {
        if (rboxes_valid[0] && rboxes_valid[1] && m_frame_boxes[0].center.x > m_frame_boxes[1].center.x) {
            MvRotatedBox B = m_frame_boxes[0];
            m_frame_boxes[0] = m_frame_boxes[1];
            m_frame_boxes[1] = B;
        }
    }
    bool has_data () {
        return frame_has_data;
    }
    bool is_valid () {
        return (circle_valid || rboxes_valid[0] || rboxes_valid[1]);
    }
    void clear () {
        frame_has_data = false;
        circle_valid = false;
        rboxes_valid[0] = false;
        rboxes_valid[1] = false;
    }

    void drawOntoImage (IplImage* img) {
        if (circle_valid)
            m_frame_circle.drawOntoImage(img);
        if (rboxes_valid[0])
            m_frame_boxes[0].drawOntoImage(img);
        if (rboxes_valid[1])
            m_frame_boxes[1].drawOntoImage(img);
    }
    void print () {
        if (is_valid())
            printf ("FRAME_DATA: (Circ/Box0/Box1) = (%d, %d, %d)\n", circle_valid?1:0, rboxes_valid[0]?1:0, rboxes_valid[1]?1:0);
        else
            printf ("FRAME_DATA: Invalid\n");
    }
};

inline void shift_frame_data (MDA_FRAME_DATA frame_data_vector[], int &read_index, int num_frames) {
    assert (read_index >= 0);
    assert (read_index < num_frames);
    
    read_index--;
    if (read_index < 0)
        read_index = num_frames-1;
    frame_data_vector[read_index].clear();
}

#endif