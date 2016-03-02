#include "mbed.h"
#include "TFC.h"
#include "Telemetry.h"

int main()
{
	float servo_pos = 0.0;
	float avg_brightness = 0.0;
	int exposure_time_us = 8000;
	int16_t deriv_image[128];
	int frames = 0, fps = 0;
	int ticks = 0;
	int display_interval = 100;
	
	Timer tm_tick, tm_fps_upd, tm_servo_upd, tm_motor_upd, tm_camera, tm_display, tm_tlm;
	
	// Initialize modules
	TFC_Init();
	
	//TFC_SetLineScanExposureTime(40000);
	
	//Serial pc(USBTX, USBRX);
	//pc.baud(115200);
	
	Telemetry TM(115200);
	
	// Initialize timers
	tm_fps_upd.start();
	tm_servo_upd.start();
	tm_motor_upd.start();
	//tm_camera.start();
	tm_display.start();
	tm_tlm.start();
	
	while (1) {
		// Servo mode 
		if (TFC_GetDIP_Switch() &0x01 && tm_servo_upd.read_ms() > 10) {
			tm_servo_upd.reset();
			servo_pos = TFC_ReadPot(0);
			TFC_SetServo(0, servo_pos);
		}
		
		// Camera mode
		if (TFC_GetDIP_Switch() &0x02 && TFC_LineScanImageReady == 1) {
			//tm_camera.reset();
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
			
			// Publish data
			for (int i = 0; i < 127; i++) {
				//TM.pub_u8("cam", TFC_LineScanImage0[i]);
				TM.pub_i8("cam2", deriv_image[i]);
			}
			
			frames++;
			
			// Compute average brightness and display it with LEDs
			avg_brightness = ((float) buffer) / 128.0;
			TFC_SetBatteryLED_Level((avg_brightness + 1) / 1024);
		}
		
		// Motor mode
		if (TFC_GetDIP_Switch() &0x04 && tm_motor_upd.read_ms() > 10) {
			tm_motor_upd.reset();
			// TODO
		}
		
		// Display debug information
		if (tm_display.read_ms() > display_interval) {
			tm_display.reset();
			float avg_tick_t = (float) display_interval / (float) ticks * 1000.0;
			//pc.printf("t: %dus   CAM: %d   FPS: %d\n", (int) avg_tick_t,
			//(int) avg_brightness, fps);
			TM.pub_u8("fps", (uint8_t) fps);
			TM.pub_u8("tick", (uint8_t) avg_tick_t);
			ticks = 0;
		}
		
		// Update average fps
		if (tm_fps_upd.read_ms() > 1000) {
			tm_fps_upd.reset();
			fps = frames;
			frames = 0;
		}
		
		ticks++;
	}
}