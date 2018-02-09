#ifndef CAMERA_H_
#define CAMERA_H_

#include "mbed.h"
#include <vector>
#define FRAME_SIZE 128
#define UNDEFINED 255

struct Peak {
	uint8_t left;
	uint8_t right;
};

void camera_process();
void filter(int16_t x[], float y[], float a[], uint8_t a_size, float b[], uint8_t b_size);

extern float avg_brightness;
extern uint16_t image[FRAME_SIZE];
extern int16_t image_d[FRAME_SIZE];
extern float image_f[FRAME_SIZE];
extern uint16_t peak_threshold, peak_end_threshold;
extern uint8_t lline_pos, rline_pos;
extern uint8_t pos_state;

#endif /* CAMERA_H_ */
