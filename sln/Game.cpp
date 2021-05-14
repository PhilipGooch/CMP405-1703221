// https://courses.cs.washington.edu/courses/csep557/10au/lectures/triangle_intersection.pdf
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "DisplayObject.h"
#include <string>
#include <map>

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
	m_display_objects.clear();
	
	//initial Settings
	//modes
	//m_grid = false;
}

Game::~Game()
{

#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height, Camera* camera)
{
	GetClientRect(window, &m_ScreenDimensions); 

	m_camera = camera;

    ///m_gamePad = std::make_unique<GamePad>();

    ///m_keyboard = std::make_unique<Keyboard>();

    ///m_mouse = std::make_unique<Mouse>();
    ///m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	

#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

///void Game::SetGridState(bool state)
///{
///	m_grid = state;
///}

int Game::MousePicking()
{
	int selected_object_ID = -1;
	float pickedDistance = 0;

	//setup near and far planes of frustum with mouse X and mouse y passed down from Toolmain. 
	//they may look the same but note, the difference in Z
	const XMVECTOR nearSource = XMVectorSet(m_input.mouse_x, m_input.mouse_y, 0.0f, 1.0f);
	const XMVECTOR farSource = XMVectorSet(m_input.mouse_x, m_input.mouse_y, 1.0f, 1.0f);

	// Stores IDs of intersected objects and their distances from the ray origin, in order of shortest distance.
	std::map<float, int, std::less<float>> distances_and_IDs;

	//Loop through entire display list of objects and pick with each in turn. 
	for (int i = 0; i < m_display_objects.size(); i++)
	{
		//Get the scale factor and translation of the object
		const XMVECTORF32 scale = { m_display_objects[i].m_scale.x,	m_display_objects[i].m_scale.y,	m_display_objects[i].m_scale.z };
		const XMVECTORF32 translate = { m_display_objects[i].m_position.x, m_display_objects[i].m_position.y, m_display_objects[i].m_position.z };

		//convert euler angles into a quaternion for the rotation of the object
		XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(m_display_objects[i].m_orientation.y *3.1415 / 180, m_display_objects[i].m_orientation.x *3.1415 / 180, m_display_objects[i].m_orientation.z *3.1415 / 180);

		//create set the matrix of the selected object in the world based on the translation, scale and rotation.
		XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);

		//Unproject the points on the near and far plane, with respect to the matrix we just created.
		XMVECTOR nearPoint = XMVector3Unproject(nearSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_camera->m_view, local);

		XMVECTOR farPoint = XMVector3Unproject(farSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_camera->m_view, local);

		//turn the transformed points into our picking vector. 
		XMVECTOR pickingVector = farPoint - nearPoint;
		pickingVector = XMVector3Normalize(pickingVector);

		//loop through mesh list for object
		for (int y = 0; y < m_display_objects[i].m_model.get()->meshes.size(); y++)
		{
			//checking for ray intersection
			if (m_display_objects[i].m_model.get()->meshes[y]->boundingBox.Intersects(nearPoint, pickingVector, pickedDistance))
			{
				// Insert distance and ID as a pair into map.
				distances_and_IDs.insert({ pickedDistance, i });
				break;
			}
		}
	}
	// If there has been at least one intersection.
	if (distances_and_IDs.size() > 0)
	{
		// The desired object ID is the first in the list as it has already been sorted.
		selected_object_ID = distances_and_IDs.begin()->second;
	}
	distances_and_IDs.clear();

	//if we got a hit.  return it.  
	return selected_object_ID;
}


#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick(InputCommands *Input)
{
	//copy over the input commands so we have a local version to use elsewhere.
	m_input = *Input;

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	m_fps = timer.GetFramesPerSecond();


	m_batchEffect->SetView(m_camera->m_view);
	m_batchEffect->SetWorld(Matrix::Identity);

	m_display_chunk.m_terrainEffect->SetView(m_camera->m_view);
	m_display_chunk.m_terrainEffect->SetWorld(Matrix::Identity);

	m_water_chunk.m_terrainEffect->SetView(m_camera->m_view);
	m_water_chunk.m_terrainEffect->SetWorld(Matrix::Identity);

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

   
}
#pragma endregion

void Game::RenderDisplayObject(ID3D11DeviceContext* context, DisplayObject& display_object, bool wireframe)
{
	m_deviceResources->PIXBeginEvent(L"Draw model");
	const XMVECTORF32 scale = { display_object.m_scale.x, display_object.m_scale.y, display_object.m_scale.z };
	const XMVECTORF32 translate = { display_object.m_position.x, display_object.m_position.y, display_object.m_position.z };

	//convert degrees into radians for rotation matrix
	XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(display_object.m_orientation.y * 3.1415 / 180,
		display_object.m_orientation.x * 3.1415 / 180,
		display_object.m_orientation.z * 3.1415 / 180);

	XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);

	display_object.m_model->Draw(context, *m_states, local, m_camera->m_view, m_projection, wireframe);	//last variable in draw,  make TRUE for wireframe

	m_deviceResources->PIXEndEvent();
}

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

	///if (m_grid)
	///{
	///	// Draw procedurally generated dynamic grid
	///	const XMVECTORF32 xaxis = { 512.f, 0.f, 0.f };
	///	const XMVECTORF32 yaxis = { 0.f, 0.f, 512.f };
	///	DrawGrid(xaxis, yaxis, g_XMZero, 512, 512, Colors::Gray);
	///}

	// RENDER OBJECTS FROM SCENEGRAPH
	int numRenderObjects = m_display_objects.size();
	for (int i = numRenderObjects - 1; i >= 0; i--) // looping backwards for duplicate!
	{
		if (m_display_objects[i].m_render)
		{
			RenderDisplayObject(context, m_display_objects[i], false);
		}
	}
	// TERRAIN WIDGET
	if (m_terrain_widget.m_render)
	{
		RenderDisplayObject(context, m_terrain_widget, true);
	}
	// TRANSLATION WIDGET
	if (m_translation_widget.m_render)
	{
		RenderDisplayObject(context, m_translation_widget, false);
	}
	// ROTATION WIDGET
	if (m_rotation_widget.m_render)
	{
		RenderDisplayObject(context, m_rotation_widget, false);
	}
	// SCALE WIDGET
	if (m_scale_widget.m_render)
	{
		RenderDisplayObject(context, m_scale_widget, false);
	}
	// SELECTION WIDGET
	if (m_selection_widget.m_render)
	{
		RenderDisplayObject(context, m_selection_widget, false);
	}

    m_deviceResources->PIXEndEvent();

	//RENDER TERRAIN
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(),0);
	context->RSSetState(m_states->CullNone());
	//wcontext->RSSetState(m_states->Wireframe());		//uncomment for wireframe

	//Render the batch,  This is handled in the Display chunk becuase it has the potential to get complex
	m_display_chunk.RenderBatch(m_deviceResources);
	m_water_chunk.RenderBatch(m_deviceResources);

	m_sprites->Begin();

	// debug
	std::wstring var;
	//var = L"position: " + std::to_wstring(m_camera->m_position.x) + L", " + 
	//					 std::to_wstring(m_camera->m_position.y) + L", " + 
	//					 std::to_wstring(m_camera->m_position.z);
	//m_font->DrawString(m_sprites.get(), var.c_str(), XMFLOAT2(100, 10), Colors::Black);
	//var = L"rotation: " + std::to_wstring(m_camera->m_roll) + L", " +
	//						 std::to_wstring(m_camera->m_pitch) + L", " +
	//						 std::to_wstring(m_camera->m_yaw);
	//var = L"x: " + std::to_wstring(m_input.mouse_x) + L"y: " + std::to_wstring(m_input.mouse_y);
	//var = L"ascii: " + std::to_wstring(m_input.ascii);
	//var = L"selected: " + std::to_wstring(m_selected_object_ID);
	//var = L"mouseX: " + std::to_wstring(m_input.mouse_x) + L" mouseY" + std::to_wstring(m_input.mouse_y);
	//var = L"dragged: " + std::to_wstring(dragged);
	//var = L"sceneID: " + std::to_wstring(scene_ID);
	//var = L"number of display objects: " + std::to_wstring(m_display_objects.size());
	//var = L"distance squared: " + std::to_wstring(distance_squared);
	var = L" " + std::to_wstring(m_fps);
	m_font->DrawString(m_sprites.get(), var.c_str(), XMFLOAT2(800, 10), Colors::Black);
	//

	m_sprites->End();

    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

//void XM_CALLCONV Game::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
//{
//    m_deviceResources->PIXBeginEvent(L"Draw grid");
//
//    auto context = m_deviceResources->GetD3DDeviceContext();
//    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
//    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
//    context->RSSetState(m_states->CullCounterClockwise());
//
//    m_batchEffect->Apply(context);
//
//    context->IASetInputLayout(m_batchInputLayout.Get());
//
//    m_batch->Begin();
//
//    xdivs = std::max<size_t>(1, xdivs);
//    ydivs = std::max<size_t>(1, ydivs);
//
//    for (size_t i = 0; i <= xdivs; ++i)
//    {
//        float fPercent = float(i) / float(xdivs);
//        fPercent = (fPercent * 2.0f) - 1.0f;
//        XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
//        vScale = XMVectorAdd(vScale, origin);
//
//        VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
//        VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
//        m_batch->DrawLine(v1, v2);
//    }
//
//    for (size_t i = 0; i <= ydivs; i++)
//    {
//        float fPercent = float(i) / float(ydivs);
//        fPercent = (fPercent * 2.0f) - 1.0f;
//        XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
//        vScale = XMVectorAdd(vScale, origin);
//
//        VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
//        VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
//        m_batch->DrawLine(v1, v2);
//    }
//
//    m_batch->End();
//
//    m_deviceResources->PIXEndEvent();
//}
#pragma endregion

///#pragma region Message Handlers
/// // Message handlers
///void Game::OnActivated()
///{
///	int asdf = 0;
///}
///
///void Game::OnDeactivated()
///{
///	int asdf = 0;
///}
///
///void Game::OnSuspending()
///{
///	int asdf = 0;
///#ifdef DXTK_AUDIO
///    m_audEngine->Suspend();
///#endif
///}
///
///void Game::OnResuming()
///{
///	int asdf = 0;
///    m_timer.ResetElapsedTime();
///
///#ifdef DXTK_AUDIO
///    m_audEngine->Resume();
///#endif
///}
///
///void Game::OnWindowSizeChanged(int width, int height)
///{
///    if (!m_deviceResources->WindowSizeChanged(width, height))
///        return;
///
///    CreateWindowSizeDependentResources();
///}

void Game::BuildDisplayObject(DisplayObject& display_object, SceneObject& scene_object)
{
	auto device = m_deviceResources->GetD3DDevice();
	auto devicecontext = m_deviceResources->GetD3DDeviceContext();

	// load model
	std::wstring model_path = StringToWCHART(scene_object.model_path);
	display_object.m_model = Model::CreateFromCMO(device, model_path.c_str(), *m_fxFactory, true);

	// load texture
	std::wstring texture_path = StringToWCHART(scene_object.texture_path);
	HRESULT result;
	result = CreateDDSTextureFromFile(device, texture_path.c_str(), nullptr, &display_object.m_texture);
	// if load fails, load error texture
	if (result)
	{
		CreateDDSTextureFromFile(device, L"database/data/Error.dds", nullptr, &display_object.m_texture);
	}
}

void Game::BuildDisplayObjects(std::vector<SceneObject>& scene_objects)
{
	ClearDisplayObjects();

	int numObjects = scene_objects.size();
	for (int i = 0; i < numObjects; i++)
	{
		DisplayObject display_object;
		
		// Split into build and update as there is no need to re-build objects when they are moved.

		BuildDisplayObject(display_object, scene_objects.at(i));

		UpdateDisplayObject(display_object, scene_objects.at(i));

		m_display_objects.push_back(display_object);

	}

	number_of_scene_objects = scene_objects.size();
}

void Game::UpdateDisplayObjects(std::vector<SceneObject>& scene_objects)
{
	for (SceneObject scene_object : scene_objects)
	{
		UpdateDisplayObject(m_display_objects[scene_object.ID], scene_object);
	}
}

void Game::UpdateDisplayObject(DisplayObject& display_object, SceneObject& scene_object)
{
	// Setting display objects' variables to match corresponding scene objects'
	display_object.m_position.x = scene_object.posX;
	display_object.m_position.y = scene_object.posY;
	display_object.m_position.z = scene_object.posZ;
	display_object.m_orientation.x = scene_object.rotX;
	display_object.m_orientation.y = scene_object.rotY;
	display_object.m_orientation.z = scene_object.rotZ;
	display_object.m_scale.x = scene_object.scaX;
	display_object.m_scale.y = scene_object.scaY;
	display_object.m_scale.z = scene_object.scaZ;
	display_object.m_scale.z = scene_object.scaZ;
	display_object.m_render = scene_object.render;
	display_object.m_wireframe = scene_object.editor_wireframe;
}

void Game::BuildDisplayChunk(ChunkObject * SceneChunk)
{
	//populate our local DISPLAYCHUNK with all the chunk info we need from the object stored in toolmain
	//which, to be honest, is almost all of it. Its mostly rendering related info so...
	m_display_chunk.PopulateChunkData(SceneChunk);		//migrate chunk data
	m_display_chunk.LoadHeightMap(m_deviceResources);
	m_display_chunk.m_terrainEffect->SetProjection(m_projection);
	m_display_chunk.InitialiseBatch();
	m_display_chunk.UpdateTerrain();
}

void Game::BuildWaterDisplayChunk(ChunkObject* SceneChunk)
{
	m_water_chunk.PopulateChunkData(SceneChunk);		//migrate chunk data
	m_water_chunk.LoadHeightMap(m_deviceResources);
	m_water_chunk.m_terrainEffect->SetProjection(m_projection);
	m_water_chunk.InitialiseBatch();
	m_water_chunk.UpdateTerrain();
}

void Game::SaveDisplayChunk(ChunkObject * SceneChunk)
{
	m_display_chunk.SaveHeightMap();	//save heightmap to file.
}

void Game::ClearDisplayObjects()
{
	for (DisplayObject display_object : m_display_objects)
	{
		display_object.m_texture->Release();
	}
	m_display_objects.clear();		
}

Vector3 Game::CaluclateClosestVertex(Vector3 & intersection)
{
	return m_display_chunk.CaluclateClosestVertex(intersection);
}

void Game::SaveVertexHeight(Vector3 & intersection)
{
	m_display_chunk.SaveFlattenVertexHeight(intersection);
}

void Game::SetHighLight(DisplayObject& display_object, XMVECTORF32 color)
{
	display_object.SetHighlight(color);
}

bool Game::CheckTerrainIntersection(Vector3& intersection)
{
	// Create ray from mouse position.
	const XMVECTOR nearSource = XMVectorSet(m_input.mouse_x, m_input.mouse_y, 0.0f, 1.0f);
	const XMVECTOR farSource = XMVectorSet(m_input.mouse_x, m_input.mouse_y, 1.0f, 1.0f);

	XMVECTOR nearPoint = XMVector3Unproject(nearSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_camera->m_view, Matrix::Identity);
	XMVECTOR farPoint = XMVector3Unproject(farSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_camera->m_view, Matrix::Identity);

	XMVECTOR pickingVector = farPoint - nearPoint;
	pickingVector = XMVector3Normalize(pickingVector);

	float distance;

	// Check for intersection with terrain.
	return m_display_chunk.TerrainIntersect(nearPoint, pickingVector, intersection, distance);
}

void Game::EditDisplayChunk(Vector3 & intersection, int radius, int mode)
{
	DisplayChunk::EDIT_MODE edit_mode;
	switch (mode)
	{
	case 0: edit_mode = DisplayChunk::EDIT_MODE::BUILD; break;
	case 1: edit_mode = DisplayChunk::EDIT_MODE::DIG; break;
	case 2: edit_mode = DisplayChunk::EDIT_MODE::FLATTEN; break;
	case 3: edit_mode = DisplayChunk::EDIT_MODE::SMOOTH; break;
	}
	m_display_chunk.EditHeightmap(intersection, radius, edit_mode);
}

float Game::CalculateGroundHeight(SceneObject& scene_object)
{
	float top = 64;			// Max height of terrain
	float bottom = 0;		// Min height of terrain.
	Vector3 intersection;	
	float distance_from_top;

	// Origin and direction of ray cast directly down from ceiling at x / z position of object.
	Vector3 orign(scene_object.posX, top, scene_object.posZ);
	Vector3 direction(0, -1, 0);

	// This function stores the distance to the intersection in the variable passed as fourth parameter.
	m_display_chunk.TerrainIntersect(orign, direction, intersection, distance_from_top);

	float distance_from_bottom = bottom + (top - distance_from_top);

	return distance_from_bottom;
}

Vector3 Game::CalculatePlaneIntersection(Plane plane)
{
	Vector3 nearSource = XMVectorSet(m_input.mouse_x, m_input.mouse_y, 0.0f, 1.0f);
	Vector3 farSource = XMVectorSet(m_input.mouse_x, m_input.mouse_y, 1.0f, 1.0f);

	Vector3 nearPoint = XMVector3Unproject(nearSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_camera->m_view, Matrix::Identity);
	Vector3 farPoint = XMVector3Unproject(farSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_camera->m_view, Matrix::Identity);

	Vector3 ray_direction = farPoint - nearPoint;
	ray_direction = XMVector3Normalize(ray_direction);

	Ray ray(nearPoint, ray_direction);

	Vector3 intersection;
	float distance;
	if (ray.Intersects(plane, distance))
	{
		intersection = nearPoint + ray_direction * distance;
	}

	return intersection;
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);
	m_fxFactory->SetDirectory(L"database/data/"); //fx Factory will look in the database directory
	m_fxFactory->SetSharing(false);	//we must set this to false otherwise it will share effects based on the initial tex loaded (When the model loads) rather than what we will change them to.

    m_sprites = std::make_unique<SpriteBatch>(context);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    m_batchEffect = std::make_unique<BasicEffect>(device);
    m_batchEffect->SetVertexColorEnabled(true);

    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        m_batchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        DX::ThrowIfFailed(
            device->CreateInputLayout(VertexPositionColor::InputElements,
                VertexPositionColor::InputElementCount,
                shaderByteCode, byteCodeLength,
                m_batchInputLayout.ReleaseAndGetAddressOf())
        );
    }

    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");

	//m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

    // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    //m_model = Model::CreateFromSDKMESH(device, L"tiny.sdkmesh", *m_fxFactory);
	

    // Load textures
    //DX::ThrowIfFailed(
    //    CreateDDSTextureFromFile(device, L"water.dds", nullptr, m_texture1.ReleaseAndGetAddressOf())
    //);
	//
    //DX::ThrowIfFailed(
    //    CreateDDSTextureFromFile(device, L"windowslogo.dds", nullptr, m_texture2.ReleaseAndGetAddressOf())
    //);

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    RECT rect = m_deviceResources->GetOutputSize();
    float aspectRatio = float(rect.right) / float(rect.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
	m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        1000.0f
    );

    m_batchEffect->SetProjection(m_projection);
	
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_batch.reset();
    m_batchEffect.reset();
    m_font.reset();
    //m_shape.reset();
    //m_model.reset();
    //m_texture1.Reset();
    //m_texture2.Reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

std::wstring StringToWCHART(std::string s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
