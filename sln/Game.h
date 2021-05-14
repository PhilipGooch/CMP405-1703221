//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SceneObject.h"
#include "DisplayObject.h"
#include "DisplayChunk.h"
#include "ChunkObject.h"
#include "InputCommands.h"
#include <vector>

///
#include "Camera.h"
///

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public DX::IDeviceNotify
{
public:

	Game();
	~Game();

	// Initialization and management
	void Initialize(HWND window, int width, int height, Camera* camera);
	//void SetGridState(bool state);

	// Basic game loop
	void Tick(InputCommands * Input);
	void Render();

	// Rendering helpers
	void Clear();

	// IDeviceNotify
	virtual void OnDeviceLost() override;
	virtual void OnDeviceRestored() override;

	/// // Messages
	///void OnActivated();
	///void OnDeactivated();
	///void OnSuspending();
	///void OnResuming();
	///void OnWindowSizeChanged(int width, int height);

	// BuildDisplayObject split into build and update as no need to rebuild objects when they are transformed.
	void BuildDisplayObject(DisplayObject& display_object, SceneObject& scene_object);
	void UpdateDisplayObject(DisplayObject& display_object, SceneObject& scene_object);

	// Builds and updates all objects in scene graph.
	void BuildDisplayObjects(std::vector<SceneObject>& scene_objects); 
	void UpdateDisplayObjects(std::vector<SceneObject>& scene_objects);

	// Releases memory on heap for each object in scene graph and deletes them.
	void ClearDisplayObjects();

	// Terrain mesh.
	void BuildDisplayChunk(ChunkObject* scene_chunk);
	// Water mesh.
	void BuildWaterDisplayChunk(ChunkObject* scene_chunk);	
	
	// Editing terrain heightmap.
	void EditDisplayChunk(Vector3& intersection, int radius, int mode);

	void SaveDisplayChunk(ChunkObject* scene_chunk);	

	// Enables fog effect on display object with given color.
	void SetHighLight(DisplayObject& display_object, XMVECTORF32 color);

	// Calculates closest vertex to intersection point on terrain mesh.
	Vector3 CaluclateClosestVertex(Vector3 & intersection);

	// Stores vertex height of closest vertex to intersection point. (used for "Flatten" terrain tool)
	void SaveVertexHeight(Vector3 & intersection);

	// For selecting objcets.
	int MousePicking();

	// Returns if ray intersects with terrain mesh and stores intersection point.
	bool CheckTerrainIntersection(Vector3& intersection);

	// Calculating ground height at x / z position of an object.
	float CalculateGroundHeight(SceneObject& scene_object);

	// Calculates the point of intersection of the mouse ray and a given plane. (used for object controlling object transforms)
	Vector3 CalculatePlaneIntersection(Plane plane);

	// Getters for widget display objects.
	DisplayObject& GetTerrainWidget() { return m_terrain_widget; }			// <--- dont like having getters i am calling every frame to send back to same class...
	DisplayObject& GetTranslationWidget() { return m_translation_widget; }	//		alternative is having DisplayObject in toolmain...?
	DisplayObject& GetRotationWidget() { return m_rotation_widget; }
	DisplayObject& GetScaleWidget() { return m_scale_widget; }
	DisplayObject& GetSelectionWidget() { return m_selection_widget; }
	DisplayObject& GetTreeDisplayObject(int index) { return m_display_objects[index]; }

	// debug
	int m_selected_object_ID;
	bool dragged;
	int scene_ID;
	int number_of_scene_objects;
	float distance_squared;
	//

#ifdef DXTK_AUDIO
	void NewAudioDevice();
#endif

private:

	void Update(DX::StepTimer const& timer);

	// Renders a single display object. (widget display objects are not in scene graph)
	void RenderDisplayObject(ID3D11DeviceContext * context, DisplayObject & display_object, bool wireframe);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	///void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

	//tool specific
	std::vector<DisplayObject>			m_display_objects;
	DisplayChunk						m_display_chunk;
	InputCommands						m_input;

	//control variables
	///bool m_grid;							//grid rendering on / off
	// Device resources.
    std::shared_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // Input devices.
    ///std::unique_ptr<DirectX::GamePad>       m_gamePad;
    ///std::unique_ptr<DirectX::Keyboard>      m_keyboard;
    ///std::unique_ptr<DirectX::Mouse>         m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    ///std::unique_ptr<DirectX::GeometricPrimitive>                            m_shape;
    ///std::unique_ptr<DirectX::Model>                                         m_model;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;

#ifdef DXTK_AUDIO
    std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif

    ///Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;		
    ///Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;

    bool                                                                    m_retryDefault;
#endif

    DirectX::SimpleMath::Matrix                                             m_world;
	DirectX::SimpleMath::Matrix												m_projection;

	///

	Camera* m_camera;
	RECT m_ScreenDimensions;			// Used for ray casting from mouse position.
	int m_fps;

	// Water terrain.
	DisplayChunk m_water_chunk;

	// Widgets.
	DisplayObject m_terrain_widget;
	DisplayObject m_translation_widget;
	DisplayObject m_rotation_widget;
	DisplayObject m_scale_widget;
	DisplayObject m_selection_widget;

	///

};

std::wstring StringToWCHART(std::string s);