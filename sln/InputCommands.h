#pragma once

struct InputCommands
{
	bool forward						= false;
	bool back							= false;
	bool left							= false;
	bool right							= false;
	bool up								= false;
	bool down							= false;
	float mouse_x						= 0.0f;
	float mouse_y						= 0.0f;
	float last_mouse_x					= 0.0f;
	float last_mouse_y					= 0.0f;
	bool mouse_left_pressed				= false;
	bool mouse_right_pressed			= false;
	bool mouse_left_clicked				= false;
	bool mouse_right_clicked			= false;
	bool mouse_left_double_click		= false;
	int mouse_scroll_wheel				= 3;
	bool speed_boost					= false;
	int ascii							= 0;
};
