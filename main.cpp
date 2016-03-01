#include "mbed.h"
#include "TFC.h"

int main()
{
	float servo_pos = 0.0;
	float avg_brightness = 0.0;
	int exposure_time_us = 8000;
	int16_t deriv_image[128];
	int frames = 0, fps = 0;
	int ticks = 0;
	int display_interval = 100;
	
	Timer t_tick, t_fps_upd, t_servo_upd, t_motor_upd, t_camera, t_display;
	
	// Initialize modules
	TFC_Init();
	
	//TFC_SetLineScanExposureTime(40000);
	
	Serial pc(USBTX, USBRX);
	pc.baud(115200);
	
	// Initialize timers
	t_fps_upd.start();
	t_servo_upd.start();
	t_motor_upd.start();
	t_camera.start();
	t_display.start();
	
	while (1) {
		// Servo mode 
		if (TFC_GetDIP_Switch() &0x01 && t_servo_upd.read_ms() > 10) {
			t_servo_upd.reset();
			servo_pos = TFC_ReadPot(0);
			TFC_SetServo(0, servo_pos);
		}
		
		// Camera mode
		if (TFC_GetDIP_Switch() &0x02 && TFC_LineScanImageReady == 1) {
			t_camera.reset();
			TFC_LineScanImageReady = 0;
			
			// Derive frame
			uint32_t buffer = 0;
			for (int i = 0; i < 127; i++) {
				deriv_image[i] = TFC_LineScanImage0[i + 1] - TFC_LineScanImage0[i];
				buffer += TFC_LineScanImage0[i];
			}
			
			// Correct side effect
			deriv_image[127] = deriv_image[126];
			buffer += TFC_LineScanImage0[127];
			
			frames++;
			
			// Compute average brightness and display it with LEDs
			avg_brightness = ((float) buffer) / 128.0;
			TFC_SetBatteryLED_Level((avg_brightness + 1) / 1024);
		}
		
		// Motor mode
		if (TFC_GetDIP_Switch() &0x04 && t_motor_upd.read_ms() > 10) {
			t_motor_upd.reset();
			// TODO
		}
		
		// Display debug information
		if (t_display.read_ms() > display_interval) {
			t_display.reset();
			float avg_tick_t = (float) display_interval / (float) ticks * 1000.0;
			pc.printf("t: %dus   CAM: %d   FPS: %d\n", (int) avg_tick_t,
			(int) avg_brightness, fps);
			ticks = 0;
		}
		
		// Update average fps
		if (t_fps_upd.read_ms() > 1000) {
			t_fps_upd.reset();
			fps = frames;
			frames = 0;
		}
		
		ticks++;
	}
}