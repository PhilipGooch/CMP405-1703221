#include "Camera.h"

Camera::Camera(InputCommands& input, float width, float height) :
	m_input(input),
	m_width(width),
	m_height(height),
	m_speed(0.2f),
	m_rotate_speed(0.1f),
	m_roll(0),
	m_pitch(0),
	m_yaw(0),
	m_up(0, 1, 0),
	m_position(35.0f, 98.0f, 360.0f),
	m_forward(0, 0, -1),
	m_right(1, 0, 0)
{
}

Camera::~Camera()
{
}

void Camera::update()
{
	m_input.speed_boost ? m_speed = 3.0f : m_speed = 1.0f;

	if (m_input.forward) m_position += m_forward * m_speed;
	if (m_input.back) m_position -= m_forward * m_speed;
	if (m_input.right) m_position += m_right * m_speed;
	if (m_input.left) m_position -= m_right * m_speed;
	if (m_input.up) m_position += m_up * m_speed;
	if (m_input.down) m_position -= m_up * m_speed;

	float delta_mouse_x = (m_input.mouse_x - m_width / 2);
	float delta_mouse_y = (m_input.mouse_y - m_height / 2);

	if (m_input.mouse_right_pressed)
	{
		m_pitch -= delta_mouse_y * m_rotate_speed;
		m_yaw += delta_mouse_x * m_rotate_speed;

		if (m_pitch > 90) m_pitch = 90;
		if (m_pitch < -90) m_pitch = -90;

		float cos_pitch = cosf(m_pitch * XM_PI / 180);
		float cos_yaw = cosf(m_yaw * XM_PI / 180);
		float cos_roll = cosf(m_roll * XM_PI / 180);
		float sin_pitch = sinf(m_pitch * XM_PI / 180);
		float sin_yaw = sinf(m_yaw * XM_PI / 180);
		float sin_roll = sinf(m_roll * XM_PI / 180);

		m_forward.x = sin_yaw * cos_pitch;
		m_forward.y = sin_pitch;
		m_forward.z = cos_pitch * -cos_yaw;

		m_up.x = -cos_yaw * sin_roll - sin_yaw * sin_pitch * cos_roll;
		m_up.y = cos_pitch * cos_roll;
		m_up.z = -sin_yaw * sin_roll - sin_pitch * cos_roll * -cos_yaw;

		m_forward.Cross(m_up, m_right);
	}

	m_look_at = m_position + m_forward;

	m_view = Matrix::CreateLookAt(m_position, m_look_at, m_up);
}
