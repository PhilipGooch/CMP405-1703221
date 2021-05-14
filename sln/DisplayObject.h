#pragma once
#include "pch.h"

using namespace DirectX;

class DisplayObject
{
public:
	DisplayObject();
	~DisplayObject();

	// For highlighting the object with fog effect.
	void SetHighlight(XMVECTORF32 color);

	std::shared_ptr<DirectX::Model>						m_model;					// main mesh
	ID3D11ShaderResourceView *							m_texture;					// diffuse texture

	int m_ID;
	DirectX::SimpleMath::Vector3			m_position;
	DirectX::SimpleMath::Vector3			m_orientation;
	DirectX::SimpleMath::Vector3			m_scale;
	bool									m_render;
	bool									m_wireframe;

	int		m_light_type;
	float	m_light_diffuse_r,	m_light_diffuse_g,	m_light_diffuse_b;
	float	m_light_specular_r, m_light_specular_g, m_light_specular_b;
	float	m_light_spot_cutoff;
	float	m_light_constant;
	float	m_light_linear;
	float	m_light_quadratic;
};

