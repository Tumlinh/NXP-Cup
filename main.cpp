#include "mbed.h"
#include "TFC.h"
#include "Telemetry.h"
#include <vector>
#define FRAME_SIZE 96
#define UNDETECTED -1

struct TM_state {
	float throttle = 0.0;
	float direction = 0.0;
	uint8_t tlm_frames[3] = {0, 0, 0};
};

struct Peak {
	uint8_t left;
	uint8_t right;
};

void process(TM_state *state, TM_msg *msg);

int main()
{
	float servo_pos = 0.0;
	float avg_brightness = 0.0;
	int exposure_time_us = 8000;
	uint16_t image[FRAME_SIZE];
	int16_t deriv_image[FRAME_SIZE];
	float filtered_image[FRAME_SIZE];
	uint16_t peak_threshold = 100;
	int cam_frames = 0, fps = 0;
	int ticks = 0;
	uint16_t info_interval = 100;
	bool bLed = false, bPB = false, bPB_old = false;
	
	Timer tm_tick, tm_fps_upd, tm_servo_upd, tm_motor_upd, tm_camera, tm_info, tm_tlm, tm_led;
	
	// Initialize modules
	TFC_Init();
	
	//TFC_SetLineScanExposureTime(40000);
	
	// Initialize telemetry
	Telemetry TM(115200);
	TM_state state;
	TM.sub(process, &state);
	
	// Activate the engines
	//TFC_HBRIDGE_ENABLE;
	
	// Initialize timers
	tm_fps_upd.start();
	tm_servo_upd.start();
	tm_motor_upd.start();
	//tm_camera.start();
	tm_info.start();
	tm_tlm.start();
	tm_led.start();
	
	while (1) {
		// Blink
		if (tm_led.read_ms() > 500) {
			tm_led.reset();
			TFC_SetBatteryLED_Level(bLed);
			bLed = !bLed;
		}
		
		// Servo mode 
		if (TFC_GetDIP_Switch() &0x01 && tm_servo_upd.read_ms() > 10) {
			tm_servo_upd.reset();
			// TODO: correct value of direction?
			TFC_SetServo(0, state.direction);
		}
		
		// Camera mode
		if (TFC_GetDIP_Switch() &0x02 && TFC_LineScanImageReady == 1) {
			//tm_camera.reset();
			TFC_LineScanImageReady = 0;
			
			// Copy image to buffer (remove first and last 16 pixels)
			for (uint8_t i = 0; i < FRAME_SIZE; i++) {
				image[i] = TFC_LineScanImage0[i + 16];
			}
			
			// TODO: Filter image?
			
			// Derive image
			for (uint8_t i = 0; i < FRAME_SIZE - 1; i++) {
				deriv_image[i] = image[i + 1] - image[i];
			}
			
			// Correct side effect
			deriv_image[FRAME_SIZE - 1] = deriv_image[FRAME_SIZE - 2];
			
			// Filter derivative image (IIR low-pass filter)
			float b[] = {0.1439739168, 0.2879478337, 0.1439739168};
			float a[] = {0, -0.2809457379, 0.1855605367};
			filtered_image[0] = 0;
			filtered_image[1] = 0;
			for (uint8_t n = 2; n < FRAME_SIZE; n++) {
				filtered_image[n] = 0;
				for (uint8_t i = 0; i < 3; i++)
					filtered_image[n] += b[i] * deriv_image[n - i];
				for (uint8_t i = 0; i < 3; i++)
					filtered_image[n] -= a[i] * filtered_image[n - i];
			}
			
			// TODO: Resample filtered image at higher frequency?
			
			// Detect peaks to deduce positions of left and right lines
			std::vector<Peak> pos_peaks, neg_peaks;
			bool in_pp = false, in_np = false;
			for (uint8_t i = 2; i < FRAME_SIZE; i++) {
				Peak peak;
				uint8_t right;
				if (in_pp) {
					// Detect right of positive peak
					if (filtered_image[i] >= peak_threshold
					    && filtered_image[i] < filtered_image[i - 1]) {
						right = i;
					}
					// Detect end of positive peak
					if (filtered_image[i] <= 0) {
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
					if (filtered_image[i] >= 0) {
						in_np = false;
						peak.right = right;
						neg_peaks.push_back(peak);
					}
				}
				else {
					// Detect start of positive peak
					if (filtered_image[i] >= peak_threshold
					    && filtered_image[i] > filtered_image[i - 1]) {
						peak.left = i;
						in_pp = true;
					}
					// Detect start of negative peak
					else if (filtered_image[i] <= - peak_threshold
					    && filtered_image[i] < filtered_image[i - 1] && !a) {
						peak.left = i;
						in_np = true;
					}
				}
			}
			
			// Deduce positions of left and right lines
			// TODO
			int8_t lline_pos = UNDETECTED, rline_pos = UNDETECTED;
			if (pos_peaks.size() == 1 && neg_peaks.size() == 0)
				lline_pos = pos_peaks[0].right;
			else if (pos_peaks.size() == 0 && neg_peaks.size() == 1)
				rline_pos = neg_peaks[0].left;
			else if (pos_peaks.size() == 1 && neg_peaks.size() == 1
			         && neg_peaks[0].left < pos_peaks[0].left)
			else
			
			cam_frames++;
		}
		
		// Motor mode
		if (TFC_GetDIP_Switch() &0x04 && tm_motor_upd.read_ms() > 10) {
			tm_motor_upd.reset();
			// TODO
		}
		
		// Receive and send telemetry data
		if (tm_info.read_ms() > info_interval) {
			tm_info.reset();
			
			// Receive data
			TM.update();
			
			// Update variables
			//TFC_SetServo(0, state.direction);
			
			// Send data
			TM.pub_f32("bat", TFC_ReadBatteryVoltage());
			TM.pub_f32("direction", state.direction);
			TM.pub_u8("fps", (uint8_t) fps);
			float avg_tick_t = (float) info_interval / (float) ticks * 1000.0;
			ticks = 0;
			TM.pub_u8("tick", (uint8_t) avg_tick_t);
			TM.pub_u8("tlm0", (uint8_t) state.tlm_frames[0]);
			
			// Send images on PB pressure
			bPB = TFC_ReadPushButton(0);
			char topic1 [] = "cam:000";
			char topic2 [] = "camd:000";
			char topic3 [] = "camf:000";
			if (bPB && !bPB_old) {
				for (uint8_t i = 0; i < FRAME_SIZE; i++) {
					sprintf(topic1, "cam:%u", i);
					TM.pub_i16("cam", image[i]);
					wait_ms(1);
					sprintf(topic2, "camd:%u", i);
					TM.pub_i16("camd", deriv_image[i]);
					wait_ms(1);
					sprintf(topic3, "camf:%u", i);
					TM.pub_f32("camf", filtered_image[i]);
					wait_ms(1);
				}
				/*for (uint8_t i = 0; i < 128; i++) {
					TM.pub_i16("cam", 0);
					wait_ms(1);
					TM.pub_i16("camd", 0);
					wait_ms(1);
					TM.pub_f32("camf", 0);
					wait_ms(1);
				}*/
			}
			bPB_old = bPB;
		}
		
		// Update average fps
		if (tm_fps_upd.read_ms() > 1000) {
			tm_fps_upd.reset();
			fps = cam_frames;
			cam_frames = 0;
		}
		
		ticks++;
	}
}

void process(TM_state *state, TM_msg *msg)
{
	state->tlm_frames[0]++;
	if (msg->type == TM_float32) {
		float buffer;
		if (strcmp("throttle", msg->topic) == 0)
			if (emplace_f32(msg, &buffer))
				state->throttle = buffer;
		else if (strcmp("direction", msg->topic) == 0)
			if (emplace_f32(msg, &buffer))
				state->direction = buffer;
	}
}
