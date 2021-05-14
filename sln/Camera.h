#pragma once
#include "pch.h"

#include "InputCommands.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class Camera
{
public:
	Camera(InputCommands& input, float width, float height);
	~Camera();
	
	void update();

	InputCommands& m_input; 

	float m_width;
	float m_height;

	float m_speed;
	float m_rotate_speed;

	float m_roll;
	float m_pitch;
	float m_yaw;

	Vector3 m_forward;
	Vector3 m_right;
	Vector3 m_up;
	Vector3 m_position;
	Vector3 m_look_at;

	Matrix m_view;
	Matrix m_projection;
};

