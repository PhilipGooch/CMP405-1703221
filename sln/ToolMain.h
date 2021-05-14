#pragma once

#include <afxext.h>
#include "pch.h"
#include "Game.h"
#include "sqlite3.h"
#include "SceneObject.h"
#include "InputCommands.h"
#include <vector>

///
#include "Camera.h"
///

class ToolMain
{
public: //methods
	ToolMain();
	~ToolMain();

	enum MODE
	{
		OBJECT,
		TERRAIN
	};

	enum TERRAIN_MODE
	{
		BUILD,
		DIG,
		FLATTEN,
		SMOOTH
	};

	enum OBJECT_MODE
	{
		TRANSLATE_X,
		TRANSLATE_Y,
		TRANSLATE_Z,
		ROTATE_X,
		ROTATE_Y,
		ROTATE_Z,
		SCALE_X,
		SCALE_Y,
		SCALE_Z,
		FREE
	};

	//onAction - These are the interface to MFC

	// Updates the tool currently selected and sets the render boolean of the widgets and mouse cursors accordingly.
	void setTerrainMode(TERRAIN_MODE terrain_mode);
	void setObjectMode(OBJECT_MODE object_mode);

	// Undo, redo, duplicate and delte functions for objects.
	void Undo();
	void Redo();
	void Duplicate();

	inline void setCursor(HCURSOR cursor) { m_cursor = cursor; };
	
	int		getCurrentSelectionID();																										//returns the selection number of currently selected object so that It can be displayed.
	void	onActionInitialise(HWND window_handle, int width, int height, HCURSOR red_cursor, HCURSOR green_cursor, HCURSOR blue_cursor);	//initialises DirectX renderer and SQL LITE
	void	onActionLoad();																													//load the current chunk
	afx_msg	void	onActionSave();																											//save the current chunk
	afx_msg void	onActionSaveTerrain();																									//save chunk geometry

	void	Tick(MSG *msg);
	void	UpdateInput(MSG *msg);

public:	//variables			
	std::vector<SceneObject> m_scene_objects;						//our scenegraph storing all the objects in the current chunk
	ChunkObject m_chunk;											//our landscape chunk
	int m_selected_object_ID;		
								
private:	//methods
	void SnapSceneObjectsToGround(int radius);		// Snaps objects in the radius of the terrain tool sphere to the height of the ground.
		
private:	//variables
	HWND	m_window_handle;		//Handle to the  window
	Game	m_d3d_renderer;		//Instance of D3D rendering system for our tool
	InputCommands m_input;		//input commands that we want to use and possibly pass over to the renderer
	char	m_key_array[256];

	char	m_last_key_array[256];	// This holds the state of the keys in the last frame to check for initial key presses and releases

	sqlite3 *m_databaseConnection;	//sqldatabase handle

	int m_width;		//dimensions passed to directX
	int m_height;
	
	Camera* m_camera;

	// Variables for terrain intersection.
	bool m_terrain_intersecting;
	Vector3 m_terrain_intersection_point;

	// Modes.
	MODE m_mode;
	TERRAIN_MODE m_terrain_mode;
	OBJECT_MODE m_object_mode;

	int m_terrain_widget_radius;

	// Widgets.
	SceneObject m_terrain_widget;
	SceneObject m_translation_widget;
	SceneObject m_rotation_widget;
	SceneObject m_scale_widget;
	SceneObject m_selection_widget;

	// Vector storing all scene states.
	std::vector<std::vector<SceneObject>> m_scene_objects_memory;
	// Index of the current scene state.
	int m_scene_index;
									
	int m_last_selected_object_ID;	// Used for duplication of objects.

	// Variables for transforming objects.
	Matrix m_saved_local_rotation;
	Vector2 m_saved_mouse_position;
	Vector3 m_saved_object_position;
	bool m_object_dragged;

	// Selection widget movement variables.
	float m_arrow_movement;
	float m_arrow_speed;
	float m_arrow_max;
	float m_arrow_min;

	// Mouse cursors.
	HCURSOR m_red_cursor;
	HCURSOR m_green_cursor;
	HCURSOR m_blue_cursor;
	HCURSOR m_white_cursor;
	HCURSOR m_cursor;

	// Mouse position in relation to screen.
	POINT m_mouse_screen_position;
	// Rect object storing position and dimensions of directX window.
	RECT m_directX_window_rect;

	// Water terrain.
	ChunkObject m_water_chunk;	

};
