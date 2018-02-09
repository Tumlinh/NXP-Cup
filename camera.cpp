#include "camera.h"
#include "mbed.h"
#include "TFC.h"

float avg_brightness = 0.0;
uint16_t image[FRAME_SIZE];
int16_t image_d[FRAME_SIZE];
float image_f[FRAME_SIZE];
uint16_t peak_threshold = 200, peak_end_threshold = 10;
uint8_t lline_pos, rline_pos;
uint8_t pos_state = 4;

void camera_process()
{
	// Copy image to buffer (remove first and last 16 pixels)
	for (uint8_t i = 0; i < FRAME_SIZE; i++)
		image[i] = TFC_LineScanImage0[i + 0];
	
	// TODO: Filter image?
	
	// Derive image
	for (uint8_t i = 0; i < FRAME_SIZE - 1; i++)
		image_d[i] = image[i + 1] - image[i];
	
	// Correct side effect
	image_d[FRAME_SIZE - 1] = image_d[FRAME_SIZE - 2];
	
	// Filter derivative image (IIR low-pass filter)
	float b[] = {0.1439739168, 0.2879478337, 0.1439739168};
	float a[] = {0, -0.2809457379, 0.1855605367};
	filter(image_d, image_f, a, 3, b, 3);
	
	// TODO: Resample filtered image at higher frequency?
	
	// Detect peaks to deduce positions of left and right lines
	// TODO: right of last peak?
	std::vector<Peak> pos_peaks, neg_peaks;
	bool in_pp = false, in_np = false;
	for (uint8_t i = 1; i < FRAME_SIZE; i++) {
		Peak peak;
		uint8_t right;
		// Detect start of positive peak
		/*if (filtered_image[i] >= peak_threshold
		    && filtered_image[i] > filtered_image[i - 1]) {
			peak.left = i;
			in_pp = true;
		}
		// Detect start of negative peak
		else if (filtered_image[i] <= - peak_threshold
			 && filtered_image[i] < filtered_image[i - 1]) {
			peak.left = i;
			in_np = true;
		}
		else if (in_pp) {
			// Detect right of positive peak
			if (filtered_image[i] >= peak_threshold
			    && filtered_image[i] < filtered_image[i - 1]) {
				right = i;
			}
			// Detect end of positive peak
			if (filtered_image[i] <= peak_end_threshold) {
				in_pp = false;
				peak.right = right;
				pos_peaks.push_back(peak);
			}
		}
		else if (in_np) {
			// Detect right of negative peak
			if (filtered_image[i] <= - peak_threshold
			    && filtered_image[i] > filtered_image[i - 1]) {
				right = i;
			}
			// Detect end of negative peak
			if (filtered_image[i] >= peak_end_threshold) {
				in_np = false;
				peak.right = right;
				neg_peaks.push_back(peak);
			}
		}*/
		// Detect start of positive peak
		if (image_f[i] >= peak_threshold && image_f[i] > image_f[i - 1]) {
			peak.left = i;
			in_pp = true;
		}
		// Detect start of negative peak
		else if (image_f[i] <= - peak_threshold && image_f[i] < image_f[i - 1]) {
			peak.left = i;
			in_np = true;
		}
		else if (in_pp) {
			// Detect right of positive peak
			if (image_f[i] <= peak_threshold && image_f[i] < image_f[i - 1]) {
				right = i;
			}
			// Detect end of positive peak
			if (image_f[i] <= peak_end_threshold) {
				in_pp = false;
				peak.right = right;
				pos_peaks.push_back(peak);
			}
		}
		else if (in_np) {
			// Detect right of negative peak
			if (image_f[i] >= - peak_threshold && image_f[i] > image_f[i - 1]) {
				right = i;
			}
			// Detect end of negative peak
			if (image_f[i] >= peak_end_threshold) {
				in_np = false;
				peak.right = right;
				neg_peaks.push_back(peak);
			}
		}
	}
	
	// Deduce positions of left and right lines and update position state
	// TODO
	lline_pos = UNDEFINED;
	rline_pos = UNDEFINED;
	/*if (pos_peaks.size() == 0 && neg_peaks.size() == 0 && pos_state >= 3 && pos_state <= 5)
		pos_state = 4;
	else if (pos_peaks.size() == 1 && neg_peaks.size() == 0) {
		if (pos_state >= 2 && pos_state <= 4) {
			lline_pos = pos_peaks[0].right;
			pos_state = 3;
		}
		else if (pos_state >= 6 && pos_state <= 8) {
			rline_pos = pos_peaks[0].left;
			pos_state = 7;
		}
	}
	else if (pos_peaks.size() == 0 && neg_peaks.size() == 1) {
		if (pos_state >= 0 && pos_state <= 2) {
			lline_pos = neg_peaks[0].right;
			pos_state = 1;
		}
		else if (pos_state >= 4 && pos_state <= 6) {
			rline_pos = neg_peaks[0].left;
			pos_state = 5;
		}
	}
	else if (pos_peaks.size() == 1 && neg_peaks.size() == 1
	         && neg_peaks[0].left < pos_peaks[0].left) {
		if (pos_state >= 1 && pos_state <= 3) {
			lline_pos = neg_peaks[0].right;
			pos_state = 2;
		}
		else if (pos_state >= 5 && pos_state <= 7) {
			rline_pos = neg_peaks[0].left;
			pos_state = 6;
		}
	}
	else
		pos_state = UNDEFINED;*/
	if (pos_peaks.size() == 0 && neg_peaks.size() == 0 && pos_state >= 3 && pos_state <= 5)
		pos_state = 4;
	else if (pos_peaks.size() == 1 && neg_peaks.size() == 0) {
		if (pos_state >= 2 && pos_state <= 4) {
			lline_pos = pos_peaks[0].right;
			pos_state = 3;
		}
	}
	else if (pos_peaks.size() == 0 && neg_peaks.size() == 1) {
		if (pos_state >= 4 && pos_state <= 6) {
			rline_pos = neg_peaks[0].left;
			pos_state = 5;
		}
	}
	else
		pos_state = UNDEFINED;
}

void filter(int16_t x[], float y[], float a[], uint8_t a_size, float b[], uint8_t b_size)
{
	// Offset
	uint8_t offset;
	if (a_size >= b_size)
		offset = a_size - 1;
	else
		offset = b_size - 1;
	for (uint8_t n = 0; n < offset; n++)
		y[n] = 0;
	
	// Difference equation
	for (uint8_t n = offset; n < FRAME_SIZE; n++) {
		y[n] = 0;
		for (uint8_t i = 0; i < b_size; i++)
			y[n] += b[i] * x[n - i];
		for (uint8_t i = 0; i < a_size; i++)
			y[n] -= a[i] * y[n - i];
	}
}
