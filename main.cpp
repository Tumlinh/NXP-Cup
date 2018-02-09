#include "camera.h"
#include "mbed.h"
#include "Telemetry.h"
#include "TFC.h"

struct TM_state {
	float throttle = 0.0;
	float direction = 0.0;
	uint8_t tlm_frames[3] = {0, 0, 0};
};

void process(TM_state *state, TM_msg *msg);

int main()
{
	float servo_pos = 0.0;
	int exposure_time_us = 8000;
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
			
			camera_process();
			
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
			TM.pub_u8("pos_state", pos_state);
			TM.pub_u8("ll", lline_pos);
			TM.pub_u8("rl", rline_pos);
			
			// Send images on PB pressure
			bPB = TFC_ReadPushButton(0);
			char topic1 [] = "cam:000";
			char topic2 [] = "camd:000";
			char topic3 [] = "camf:000";
			if (bPB && !bPB_old) {
				for (uint8_t i = 0; i < FRAME_SIZE; i++) {
					sprintf(topic1, "cam:%u", i);
					TM.pub_i16(topic1, image[i]);
					wait_ms(1);
					sprintf(topic2, "camd:%u", i);
					TM.pub_i16(topic2, image_d[i]);
					wait_ms(1);
					sprintf(topic3, "camf:%u", i);
					TM.pub_f32(topic3, image_f[i]);
					wait_ms(1);
				}
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
