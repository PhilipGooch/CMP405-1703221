#include "ToolMain.h"
#include "resource.h"
#include <vector>
#include <sstream>

//
//ToolMain Class
ToolMain::ToolMain() 
{
	m_selected_object_ID = -1;		
	m_databaseConnection = NULL;

	//zero input commands
	m_input.forward		= false;
	m_input.back		= false;
	m_input.left		= false;
	m_input.right		= false;
}


ToolMain::~ToolMain()
{
	sqlite3_close(m_databaseConnection);		//close the database connection
}


void ToolMain::setTerrainMode(TERRAIN_MODE terrain_mode)
{
	m_mode = TERRAIN;
	m_terrain_mode = terrain_mode;
	m_terrain_widget.render = true;
	m_translation_widget.render = false;
	m_rotation_widget.render = false;
	m_scale_widget.render = false;
	m_selection_widget.render = false;
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetTerrainWidget(), m_terrain_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetTranslationWidget(), m_translation_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetRotationWidget(), m_rotation_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetScaleWidget(), m_scale_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetSelectionWidget(), m_selection_widget);
	m_cursor = m_white_cursor;
}

void ToolMain::setObjectMode(OBJECT_MODE object_mode)
{
	m_mode = OBJECT;
	m_object_mode = object_mode;
	m_terrain_widget.render = false;
	if (object_mode == TRANSLATE_X || object_mode == TRANSLATE_Y || object_mode == TRANSLATE_Z)
	{
		m_translation_widget.render = true;
		m_rotation_widget.render = false;
		m_scale_widget.render = false;
	}
	else if (object_mode == ROTATE_X || object_mode == ROTATE_Y || object_mode == ROTATE_Z)
	{
		m_translation_widget.render = false;
		m_rotation_widget.render = true;
		m_scale_widget.render = false;
	}
	else if(object_mode == SCALE_X || object_mode == SCALE_Y || object_mode == SCALE_Z)
	{
		m_translation_widget.render = false;
		m_rotation_widget.render = false;
		m_scale_widget.render = true;
	}
	else
	{
		m_translation_widget.render = false;
		m_rotation_widget.render = false;
		m_scale_widget.render = false;
	}
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetTerrainWidget(), m_terrain_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetTranslationWidget(), m_translation_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetRotationWidget(), m_rotation_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetScaleWidget(), m_scale_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetSelectionWidget(), m_selection_widget);

	// cursor
	if (object_mode == TRANSLATE_X || object_mode == ROTATE_X || object_mode == SCALE_X)
	{
		m_cursor = m_red_cursor;
	}
	else if (object_mode == TRANSLATE_Y || object_mode == ROTATE_Y || object_mode == SCALE_Y)
	{
		m_cursor = m_green_cursor;
	}
	else if (object_mode == TRANSLATE_Z || object_mode == ROTATE_Z || object_mode == SCALE_Z)
	{
		m_cursor = m_blue_cursor;
	}
	else
	{
		m_cursor = m_white_cursor;
	}
}

void ToolMain::Undo()
{
	// If there is a previous scene state in memory.
	if (m_scene_index > 0)
	{
		// If no mouse input.
		if (!m_input.mouse_left_pressed && !m_input.mouse_left_pressed)
		{
			// Set current scene state to the previous state and decrement scene index.
			m_scene_objects = m_scene_objects_memory.at(--m_scene_index);
			m_d3d_renderer.BuildDisplayObjects(m_scene_objects);

			// If object selected.
			if (m_selected_object_ID != -1)
			{
				// De-select object.
				m_selection_widget.render = false;
				m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetSelectionWidget(), m_selection_widget);
				m_selected_object_ID = -1;
			}
		}
	}
}

void ToolMain::Redo()
{
	// If the current scene state is not the most recent.
	if (m_scene_index < m_scene_objects_memory.size() - 1)
	{
		// Set current scene state to the next state and increment scene index.
		m_scene_objects = m_scene_objects_memory[++m_scene_index];
		m_d3d_renderer.BuildDisplayObjects(m_scene_objects);
	}
}

void ToolMain::Duplicate()
{
	// If object selected.
	if (m_selected_object_ID != -1)
	{
		// Save the current scene state to memory.
		while (m_scene_index < m_scene_objects_memory.size() - 1)
		{
			m_scene_objects_memory.pop_back();
		}
		m_scene_objects_memory.push_back(m_scene_objects);
		m_scene_index++;

		// Create duplicate object. 
		SceneObject scene_object = m_scene_objects[m_selected_object_ID];
		scene_object.ID = m_scene_objects.size();
		m_scene_objects.push_back(scene_object);
		m_d3d_renderer.BuildDisplayObjects(m_scene_objects);
	}
}

int ToolMain::getCurrentSelectionID()
{
	return m_selected_object_ID;
}

void ToolMain::onActionInitialise(HWND window_handle, int width, int height, HCURSOR red_cursor, HCURSOR green_cursor, HCURSOR blue_cursor)
{
	m_red_cursor = red_cursor;
	m_green_cursor = green_cursor;
	m_blue_cursor = blue_cursor;
	m_white_cursor = LoadCursor(NULL, IDC_ARROW);

	m_window_handle = window_handle;
	m_scene_objects_memory.push_back(std::vector<SceneObject>());

	bool m_terrain_intersecting = false;

	m_mode = OBJECT;
	m_terrain_mode = BUILD;
	m_object_mode = FREE;

	m_scene_index = 0;

	bool m_object_dragged = false;

	m_arrow_movement = 0;
	m_arrow_speed = 0.07f;
	m_arrow_max = 3;
	m_arrow_min = 1;

	//window size, handle etc for directX
	m_width		= width;
	m_height	= height;

	m_camera = new Camera(m_input, m_width, m_height);
	m_input.mouse_x = m_width / 2;
	m_input.mouse_y = m_height / 2;
	m_camera->update();

	m_d3d_renderer.Initialize(window_handle, m_width, m_height, m_camera);

	//database connection establish
	int rc;
	rc = sqlite3_open_v2("database/data.db",&m_databaseConnection, SQLITE_OPEN_READWRITE, NULL);

	if (rc) 
	{
		TRACE("Can't open database");
		//if the database cant open. Perhaps a more catastrophic error would be better here
	}
	else 
	{
		TRACE("Opened database successfully");
	}

	onActionLoad();

	// Loading water terrain chunk.
	m_water_chunk.ID = 1;
	m_water_chunk.name = "water";
	m_water_chunk.chunk_x_size_metres = 512;
	m_water_chunk.chunk_y_size_metres = 512;
	m_water_chunk.chunk_base_resolution = 128;
	m_water_chunk.heightmap_path = "database/data/water_heightmap.raw";
	m_water_chunk.tex_diffuse_path = "database/data/water.dds";
	m_water_chunk.tex_splat_alpha_path = "database/data/water.dds";
	m_water_chunk.tex_splat_1_path = "";
	m_water_chunk.tex_splat_2_path = "";
	m_water_chunk.tex_splat_3_path = "";
	m_water_chunk.tex_splat_4_path = "";
	m_water_chunk.render_wireframe = false;
	m_water_chunk.render_normals = false;
	m_water_chunk.tex_diffuse_tiling = 16;
	m_water_chunk.tex_splat_1_tiling = 1;
	m_water_chunk.tex_splat_2_tiling = 1;
	m_water_chunk.tex_splat_3_tiling = 1;
	m_water_chunk.tex_splat_4_tiling = 1;

	// Loading models.

	m_terrain_widget.model_path = "database/data/sphere.cmo";
	m_d3d_renderer.BuildDisplayObject(m_d3d_renderer.GetTerrainWidget(), m_terrain_widget);

	float scale = 0.06f;
	m_translation_widget.scaX = scale;
	m_translation_widget.scaY = scale;
	m_translation_widget.scaZ = scale;
	m_translation_widget.model_path = "database/data/translation.cmo";
	m_d3d_renderer.BuildDisplayObject(m_d3d_renderer.GetTranslationWidget(), m_translation_widget);

	scale = 0.1f;
	m_rotation_widget.scaX = scale;
	m_rotation_widget.scaY = scale;
	m_rotation_widget.scaZ = scale;
	m_rotation_widget.model_path = "database/data/rotation.cmo";
	m_d3d_renderer.BuildDisplayObject(m_d3d_renderer.GetRotationWidget(), m_rotation_widget);

	scale = 0.06f;
	m_scale_widget.scaX = scale;
	m_scale_widget.scaY = scale;
	m_scale_widget.scaZ = scale;
	m_scale_widget.model_path = "database/data/scale.cmo";
	m_d3d_renderer.BuildDisplayObject(m_d3d_renderer.GetScaleWidget(), m_scale_widget);

	scale = 2.0f;
	m_selection_widget.scaX = scale;
	m_selection_widget.scaY = scale;
	m_selection_widget.scaZ = scale;
	m_selection_widget.model_path = "database/data/selection.cmo";
	m_d3d_renderer.BuildDisplayObject(m_d3d_renderer.GetSelectionWidget(), m_selection_widget);

	//Process results into renderable
	m_d3d_renderer.BuildDisplayObjects(m_scene_objects);

	m_d3d_renderer.BuildDisplayChunk(&m_chunk);

	m_d3d_renderer.BuildWaterDisplayChunk(&m_water_chunk);

	// Saving current scene state.
	m_scene_objects_memory.back() = m_scene_objects;

	setTerrainMode(BUILD);
	setObjectMode(FREE);
}

void ToolMain::onActionLoad()
{
	//load current chunk and objects into lists
	if (!m_scene_objects.empty())		//is the vector empty
	{
		m_scene_objects.clear();		//if not, empty it
	}

	//SQL
	int rc;
	char *sqlCommand;
	char *ErrMSG = 0;
	sqlite3_stmt *pResults;								//results of the query
	sqlite3_stmt *pResultsChunk;

	//OBJECTS IN THE WORLD
	//prepare SQL Text
	sqlCommand = "SELECT * from Objects";				//sql command which will return all records from the objects table.
	//Send Command and fill result object
	rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0 );
	
	//loop for each row in results until there are no more rows.  ie for every row in the results. We create and object
	while (sqlite3_step(pResults) == SQLITE_ROW)
	{	
		SceneObject newSceneObject;
		newSceneObject.ID = sqlite3_column_int(pResults, 0);
		newSceneObject.chunk_ID = sqlite3_column_int(pResults, 1);
		newSceneObject.model_path		= reinterpret_cast<const char*>(sqlite3_column_text(pResults, 2));
		newSceneObject.texture_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 3));
		newSceneObject.posX = sqlite3_column_double(pResults, 4);
		newSceneObject.posY = sqlite3_column_double(pResults, 5);
		newSceneObject.posZ = sqlite3_column_double(pResults, 6);
		newSceneObject.rotX = sqlite3_column_double(pResults, 7);
		newSceneObject.rotY = sqlite3_column_double(pResults, 8);
		newSceneObject.rotZ = sqlite3_column_double(pResults, 9);
		newSceneObject.scaX = sqlite3_column_double(pResults, 10);
		newSceneObject.scaY = sqlite3_column_double(pResults, 11);
		newSceneObject.scaZ = sqlite3_column_double(pResults, 12);
		newSceneObject.render = sqlite3_column_int(pResults, 13);
		newSceneObject.collision = sqlite3_column_int(pResults, 14);
		newSceneObject.collision_mesh = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 15));
		newSceneObject.collectable = sqlite3_column_int(pResults, 16);
		newSceneObject.destructable = sqlite3_column_int(pResults, 17);
		newSceneObject.health_amount = sqlite3_column_int(pResults, 18);
		newSceneObject.editor_render = sqlite3_column_int(pResults, 19);
		newSceneObject.editor_texture_vis = sqlite3_column_int(pResults, 20);
		newSceneObject.editor_normals_vis = sqlite3_column_int(pResults, 21);
		newSceneObject.editor_collision_vis = sqlite3_column_int(pResults, 22);
		newSceneObject.editor_pivot_vis = sqlite3_column_int(pResults, 23);
		newSceneObject.pivotX = sqlite3_column_double(pResults, 24);
		newSceneObject.pivotY = sqlite3_column_double(pResults, 25);
		newSceneObject.pivotZ = sqlite3_column_double(pResults, 26);
		newSceneObject.snapToGround = sqlite3_column_int(pResults, 27);
		newSceneObject.AINode = sqlite3_column_int(pResults, 28);
		newSceneObject.audio_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 29));
		newSceneObject.volume = sqlite3_column_double(pResults, 30);
		newSceneObject.pitch = sqlite3_column_double(pResults, 31);
		newSceneObject.pan = sqlite3_column_int(pResults, 32);
		newSceneObject.one_shot = sqlite3_column_int(pResults, 33);
		newSceneObject.play_on_init = sqlite3_column_int(pResults, 34);
		newSceneObject.play_in_editor = sqlite3_column_int(pResults, 35);
		newSceneObject.min_dist = sqlite3_column_double(pResults, 36);
		newSceneObject.max_dist = sqlite3_column_double(pResults, 37);
		newSceneObject.camera = sqlite3_column_int(pResults, 38);
		newSceneObject.path_node = sqlite3_column_int(pResults, 39);
		newSceneObject.path_node_start = sqlite3_column_int(pResults, 40);
		newSceneObject.path_node_end = sqlite3_column_int(pResults, 41);
		newSceneObject.parent_id = sqlite3_column_int(pResults, 42);
		newSceneObject.editor_wireframe = sqlite3_column_int(pResults, 43);
		newSceneObject.name = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 44));

		newSceneObject.light_type = sqlite3_column_int(pResults, 45);
		newSceneObject.light_diffuse_r = sqlite3_column_double(pResults, 46);
		newSceneObject.light_diffuse_g = sqlite3_column_double(pResults, 47);
		newSceneObject.light_diffuse_b = sqlite3_column_double(pResults, 48);
		newSceneObject.light_specular_r = sqlite3_column_double(pResults, 49);
		newSceneObject.light_specular_g = sqlite3_column_double(pResults, 50);
		newSceneObject.light_specular_b = sqlite3_column_double(pResults, 51);
		newSceneObject.light_spot_cutoff = sqlite3_column_double(pResults, 52);
		newSceneObject.light_constant = sqlite3_column_double(pResults, 53);
		newSceneObject.light_linear = sqlite3_column_double(pResults, 54);
		newSceneObject.light_quadratic = sqlite3_column_double(pResults, 55);
	

		//send completed object to scenegraph
		m_scene_objects.push_back(newSceneObject);
	}


	//THE WORLD CHUNK
	//prepare SQL Text
	sqlCommand = "SELECT * from Chunks";				//sql command which will return all records from  chunks table. There is only one tho.
														//Send Command and fill result object
	rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResultsChunk, 0);

	sqlite3_step(pResultsChunk);
	m_chunk.ID = sqlite3_column_int(pResultsChunk, 0);
	m_chunk.name = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 1));
	m_chunk.chunk_x_size_metres = sqlite3_column_int(pResultsChunk, 2);
	m_chunk.chunk_y_size_metres = sqlite3_column_int(pResultsChunk, 3);
	m_chunk.chunk_base_resolution = sqlite3_column_int(pResultsChunk, 4);
	m_chunk.heightmap_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 5));
	m_chunk.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 6));
	m_chunk.tex_splat_alpha_path = "database/data/grass.dds"; ;// reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 7));
	m_chunk.tex_splat_1_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 8));
	m_chunk.tex_splat_2_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 9));
	m_chunk.tex_splat_3_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 10));
	m_chunk.tex_splat_4_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 11));
	m_chunk.render_wireframe = sqlite3_column_int(pResultsChunk, 12);
	m_chunk.render_normals = sqlite3_column_int(pResultsChunk, 13);
	m_chunk.tex_diffuse_tiling = sqlite3_column_int(pResultsChunk, 14);
	m_chunk.tex_splat_1_tiling = sqlite3_column_int(pResultsChunk, 15);
	m_chunk.tex_splat_2_tiling = sqlite3_column_int(pResultsChunk, 16);
	m_chunk.tex_splat_3_tiling = sqlite3_column_int(pResultsChunk, 17);
	m_chunk.tex_splat_4_tiling = sqlite3_column_int(pResultsChunk, 18);
	
}

void ToolMain::onActionSave()
{
	//SQL
	int rc;
	char *sqlCommand;
	char *ErrMSG = 0;
	sqlite3_stmt *pResults;								//results of the query
	

	//OBJECTS IN THE WORLD Delete them all
	//prepare SQL Text
	sqlCommand = "DELETE FROM Objects";	 //will delete the whole object table.   Slightly risky but hey.
	rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0);
	sqlite3_step(pResults);

	//Populate with our new objects
	std::wstring sqlCommand2;
	int numObjects = m_scene_objects.size();	//Loop thru the scengraph.

	for (int i = 0; i < numObjects; i++)
	{
		std::stringstream command;
		command << "INSERT INTO Objects " 
			<<"VALUES(" << m_scene_objects.at(i).ID << ","
			<< m_scene_objects.at(i).chunk_ID  << ","
			<< "'" << m_scene_objects.at(i).model_path <<"'" << ","
			<< "'" << m_scene_objects.at(i).texture_path << "'" << ","
			<< m_scene_objects.at(i).posX << ","
			<< m_scene_objects.at(i).posY << ","
			<< m_scene_objects.at(i).posZ << ","
			<< m_scene_objects.at(i).rotX << ","
			<< m_scene_objects.at(i).rotY << ","
			<< m_scene_objects.at(i).rotZ << ","
			<< m_scene_objects.at(i).scaX << ","
			<< m_scene_objects.at(i).scaY << ","
			<< m_scene_objects.at(i).scaZ << ","
			<< m_scene_objects.at(i).render << ","
			<< m_scene_objects.at(i).collision << ","
			<< "'" << m_scene_objects.at(i).collision_mesh << "'" << ","
			<< m_scene_objects.at(i).collectable << ","
			<< m_scene_objects.at(i).destructable << ","
			<< m_scene_objects.at(i).health_amount << ","
			<< m_scene_objects.at(i).editor_render << ","
			<< m_scene_objects.at(i).editor_texture_vis << ","
			<< m_scene_objects.at(i).editor_normals_vis << ","
			<< m_scene_objects.at(i).editor_collision_vis << ","
			<< m_scene_objects.at(i).editor_pivot_vis << ","
			<< m_scene_objects.at(i).pivotX << ","
			<< m_scene_objects.at(i).pivotY << ","
			<< m_scene_objects.at(i).pivotZ << ","
			<< m_scene_objects.at(i).snapToGround << ","
			<< m_scene_objects.at(i).AINode << ","
			<< "'" << m_scene_objects.at(i).audio_path << "'" << ","
			<< m_scene_objects.at(i).volume << ","
			<< m_scene_objects.at(i).pitch << ","
			<< m_scene_objects.at(i).pan << ","
			<< m_scene_objects.at(i).one_shot << ","
			<< m_scene_objects.at(i).play_on_init << ","
			<< m_scene_objects.at(i).play_in_editor << ","
			<< m_scene_objects.at(i).min_dist << ","
			<< m_scene_objects.at(i).max_dist << ","
			<< m_scene_objects.at(i).camera << ","
			<< m_scene_objects.at(i).path_node << ","
			<< m_scene_objects.at(i).path_node_start << ","
			<< m_scene_objects.at(i).path_node_end << ","
			<< m_scene_objects.at(i).parent_id << ","
			<< m_scene_objects.at(i).editor_wireframe << ","
			<< "'" << m_scene_objects.at(i).name << "'" << ","

			<< m_scene_objects.at(i).light_type << ","
			<< m_scene_objects.at(i).light_diffuse_r << ","
			<< m_scene_objects.at(i).light_diffuse_g << ","
			<< m_scene_objects.at(i).light_diffuse_b << ","
			<< m_scene_objects.at(i).light_specular_r << ","
			<< m_scene_objects.at(i).light_specular_g << ","
			<< m_scene_objects.at(i).light_specular_b << ","
			<< m_scene_objects.at(i).light_spot_cutoff << ","
			<< m_scene_objects.at(i).light_constant << ","
			<< m_scene_objects.at(i).light_linear << ","
			<< m_scene_objects.at(i).light_quadratic

			<< ")";
		std::string sqlCommand2 = command.str();
		rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand2.c_str(), -1, &pResults, 0);
		sqlite3_step(pResults);	
	}
	MessageBox(NULL, L"Terrain and Objects Saved", L"Notification", MB_OK);
}

void ToolMain::onActionSaveTerrain()
{
	m_d3d_renderer.SaveDisplayChunk(&m_chunk);
}

void ToolMain::SnapSceneObjectsToGround(int radius)
{
	for (SceneObject& scene_object : m_scene_objects)
	{
		Vector2 object_position(scene_object.posX, scene_object.posZ);
		Vector2 sphere_position(m_terrain_widget.posX, m_terrain_widget.posZ);

		// If the object is within the terrain tool sphere.
		if (Vector2::DistanceSquared(object_position, sphere_position) <= radius * radius * 4 * 4)
		{
			// Snap the object to the height of the ground.
			scene_object.posY = m_d3d_renderer.CalculateGroundHeight(scene_object);
		}
	}
}

void ToolMain::Tick(MSG *msg)
{
	//do we have a selection
	//do we have a mode
	//are we clicking / dragging /releasing
	//has something changed
	//update Scenegraph
	//add to scenegraph
	//resend scenegraph to Direct X renderer

	//Renderer Update Call

	m_camera->update();
	
	// OBJECT MODE //////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (m_mode == OBJECT)
	{
		// MOUSE LEFT CLICKED /////////////////////////////////////////////////////////////////////////
		if (m_input.mouse_left_clicked || m_input.mouse_left_double_click)	
		{
			// Attempt to select an object.
			int intersected_object_ID = m_d3d_renderer.MousePicking();

			// If ray intertects with an object.
			if (intersected_object_ID != -1)
			{
				m_selection_widget.render = true;

				// If the intersected object is allready selected.
				if (intersected_object_ID == m_selected_object_ID)
				{
					// Save the current scene state to memory.
					while (m_scene_index < m_scene_objects_memory.size() - 1)
					{
						m_scene_objects_memory.pop_back();
					}
					m_scene_objects_memory.push_back(m_scene_objects);
					m_scene_index++;

					m_object_dragged = true;

					// If a rotation tool is selected.
					if (m_object_mode == ROTATE_X || 
						m_object_mode == ROTATE_Y || 
						m_object_mode == ROTATE_Z)
					{
						// Save the local rotation of the object.
						SceneObject& object = m_scene_objects[m_selected_object_ID];
						m_saved_local_rotation = Matrix::CreateFromYawPitchRoll(object.rotY * XM_PI / 180, object.rotX * XM_PI / 180, object.rotZ * XM_PI / 180);
						
						// If rotating on the y axis.
						if (m_object_mode == ROTATE_Y)
						{
							// Save the mouse position. (rotation on y axis is based on delta mouse position)
							m_saved_mouse_position = Vector2(m_input.mouse_x, m_input.mouse_y);
						}
					}
				}
				// If selecting a different object.
				else
				{
					// Save last selection and update current selection.
					m_last_selected_object_ID = m_selected_object_ID;
					m_selected_object_ID = intersected_object_ID;
				}
			}
			// If ray does NOT intersect with an object.
			else
			{
				// If there is an object currently selected.
				if (m_selected_object_ID != -1)
				{
					// If the position of the mouse is not on the toolbar. (Handling bug where objects deselect when pressing buttons.)
					GetCursorPos(&m_mouse_screen_position);
					if (m_mouse_screen_position.y > m_directX_window_rect.top)
					{
						// De-select object.
						m_selection_widget.render = false;
						m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetSelectionWidget(), m_selection_widget);
						m_selected_object_ID = -1;
					}
				}
			}
		}
		// MOUSE LEFT PRESSED /////////////////////////////////////////////////////////////////////////
		if (m_input.mouse_left_pressed)
		{
			// If an object is being transformed.
			if (m_object_dragged)
			{
				// Selected object.
				SceneObject& object = m_scene_objects[m_selected_object_ID];

				// Selected object's position.
				Vector3 object_position(object.posX, object.posY, object.posZ);

				// X and Z planes running through object.
				Plane plane_x(object_position, Vector3::UnitZ);
				Plane plane_z(object_position, Vector3::UnitX);

				// Intersection point of ray and plane
				Vector3 intersection;

				// Matrix to store result of local an parent rotation matrices. (to rotate in world space instead of local space)
				Matrix world_rotation;

				// XMFLOAT4X4 to access raw data of world rotation matrix 
				DirectX::XMFLOAT4X4 matrix;

				// Variables to store extracted yaw, pitch and roll from world rotation matrix.
				float yaw, pitch, roll;

				// Which plane to use for which axis of each transformation.
				if (m_object_mode == TRANSLATE_X || m_object_mode == SCALE_X || m_object_mode == ROTATE_Z)
				{
					intersection = m_d3d_renderer.CalculatePlaneIntersection(plane_x);
				}
				if (m_object_mode == TRANSLATE_Z || m_object_mode == SCALE_Z || m_object_mode == ROTATE_X)
				{
					intersection = m_d3d_renderer.CalculatePlaneIntersection(plane_z);
				}
				if (m_object_mode == TRANSLATE_Y || m_object_mode == SCALE_Y)
				{
					// Working out better plane to use using projection / dot product.
					if (abs(Vector2(m_camera->m_forward.x, m_camera->m_forward.z).Dot(Vector2(1, 0))) >=
						abs(Vector2(m_camera->m_forward.x, m_camera->m_forward.z).Dot(Vector2(0, 1))))
					{
						intersection = m_d3d_renderer.CalculatePlaneIntersection(plane_z);
					}
					else
					{
						intersection = m_d3d_renderer.CalculatePlaneIntersection(plane_x);
					}
				}

				// FREE
				if (m_object_mode == FREE)
				{
					// Set object position to be the intersection point on the plane.
					m_d3d_renderer.CheckTerrainIntersection(m_terrain_intersection_point);
					object.posX = m_terrain_intersection_point.x;
					object.posY = m_terrain_intersection_point.y;
					object.posZ = m_terrain_intersection_point.z;
				}

				// TRANSLATION
				else if (m_object_mode == TRANSLATE_X)
				{
					object.posX = min(max(intersection.x, -256), 252);
					object.posY = m_d3d_renderer.CalculateGroundHeight(m_scene_objects[m_selected_object_ID]);
				}
				else if (m_object_mode == TRANSLATE_Y)
				{
					object.posY = min(max(intersection.y, 0), 64);
				}
				else if (m_object_mode == TRANSLATE_Z)
				{
					object.posZ = min(max(intersection.z, -256), 252);
					object.posY = m_d3d_renderer.CalculateGroundHeight(m_scene_objects[m_selected_object_ID]);
				}

				// ROTATION
				else if (m_object_mode == ROTATE_X || m_object_mode == ROTATE_Y || m_object_mode == ROTATE_Z)
				{
					// Creating world rotation matrix.
					if (m_object_mode == ROTATE_X)
					{
						world_rotation = m_saved_local_rotation * Matrix::CreateRotationX((intersection - object_position).z * 0.1f);
					}
					if (m_object_mode == ROTATE_Y)
					{
						world_rotation = m_saved_local_rotation * Matrix::CreateRotationY((m_input.mouse_x - m_saved_mouse_position.x) * 0.01f);
					}
					if (m_object_mode == ROTATE_Z)
					{
						world_rotation = m_saved_local_rotation * Matrix::CreateRotationZ(-(intersection - object_position).x * 0.1f);
					}

					// Getting raw values of world rotation matrix.
					DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixTranspose(world_rotation));

					// Extracting yaw, pitch and roll using trigonometry.
					pitch = (float)asin(-matrix._23) * 180.0f / XM_PI;
					yaw = (float)atan2(matrix._13, matrix._33) * 180.0f / XM_PI;
					roll = (float)atan2(matrix._21, matrix._22) * 180.0f / XM_PI;

					// Setting object's rotation.
					object.rotX = pitch;
					object.rotY = yaw;
					object.rotZ = roll;
				}

				// SCALE
				else if (m_object_mode == SCALE_X)
				{
					object.scaX = abs((intersection - object_position).x);
				}
				else if (m_object_mode == SCALE_Y)
				{
					object.scaY = abs((intersection - object_position).y / 4.4f);
				}
				else if (m_object_mode == SCALE_Z)
				{
					object.scaZ = abs((intersection - object_position).z);
				}
			}
			m_d3d_renderer.UpdateDisplayObjects(m_scene_objects);
		}
		// MOUSE LEFT RELEASED /////////////////////////////////////////////////////////////////////////
		else
		{
			m_object_dragged = false;
		}
	}
	// TERRAIN MODE //////////////////////////////////////////////////////////////////////////////////////////////////////////
	else if (m_mode == TERRAIN)
	{
		m_terrain_widget_radius = m_input.mouse_scroll_wheel;

		// This function stores the intersection point in the float passed in as the first parameter.
		m_terrain_intersecting = m_d3d_renderer.CheckTerrainIntersection(m_terrain_intersection_point);

		// Toggle visibility of terrain widget depending on if the mouse ray is intersecting with the ground.
		m_terrain_widget.render = m_terrain_intersecting;
		m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetTerrainWidget(), m_terrain_widget);

		// Calculate closest vertex to intersection point with terrain.
		Vector3 closest_vertex = m_d3d_renderer.CaluclateClosestVertex(m_terrain_intersection_point);

		// Update terrain widget.
		m_terrain_widget.posX = closest_vertex.x;
		m_terrain_widget.posY = closest_vertex.y;
		m_terrain_widget.posZ = closest_vertex.z;
		m_terrain_widget.scaX = m_terrain_widget_radius * 4;
		m_terrain_widget.scaY = m_terrain_widget_radius * 4;
		m_terrain_widget.scaZ = m_terrain_widget_radius * 4;
		if (m_terrain_widget_radius == 0)
		{
			m_terrain_widget.scaX = 0.5f;
			m_terrain_widget.scaY = 0.5f;
			m_terrain_widget.scaZ = 0.5f;
		}

		// MOUSE LEFT CLICKED /////////////////////////////////////////////////////////////////////////
		if (m_input.mouse_left_clicked || m_input.mouse_left_double_click)
		{
			if (m_terrain_mode == FLATTEN)
			{
				// Save hight of closest vertex to intersection point. (used to set height of surrounding vertices in "Flatten" mode)
				m_d3d_renderer.SaveVertexHeight(m_terrain_intersection_point);
			}
		}

		// MOUSE LEFT PRESSED /////////////////////////////////////////////////////////////////////////
		if (m_input.mouse_left_pressed)
		{
			GetCursorPos(&m_mouse_screen_position);
			// If the position of the mouse is not on the toolbar. (Handling bug where objects deselect when pressing buttons.)
			if (m_mouse_screen_position.y > m_directX_window_rect.top)
			{
				// If the ray is touching the terrain.
				if (m_terrain_intersecting)
				{
					// Edit the terrain.
					m_d3d_renderer.EditDisplayChunk(m_terrain_intersection_point, m_terrain_widget_radius, m_terrain_mode);

					// Snap objects to ground level in area of terrain sphere.
					SnapSceneObjectsToGround(m_terrain_widget_radius);
					m_d3d_renderer.UpdateDisplayObjects(m_scene_objects);
				}
			}
		}
	}

	// WIDGET TRANSFORMS
	
	Matrix camera_translation = Matrix::CreateTranslation(m_camera->m_position.x, m_camera->m_position.y, m_camera->m_position.z);
	Matrix camera_rotation = Matrix::CreateFromYawPitchRoll(-m_camera->m_yaw * XM_PI / 180, m_camera->m_pitch * XM_PI / 180, m_camera->m_roll * XM_PI / 180);
	Matrix camera_transformation = camera_rotation * camera_translation;

	Matrix widget_translation(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -0.48, -1.0f, 1);

	Matrix transformation = widget_translation * camera_transformation;
	Vector3 position = transformation.Translation();

	m_translation_widget.posX = position.x;
	m_translation_widget.posY = position.y;
	m_translation_widget.posZ = position.z;
	m_rotation_widget.posX = position.x;
	m_rotation_widget.posY = position.y;
	m_rotation_widget.posZ = position.z;
	m_scale_widget.posX = position.x;
	m_scale_widget.posY = position.y;
	m_scale_widget.posZ = position.z;

	// If object selected.
	if (m_selected_object_ID != -1)
	{
		// Selection arrow widget movement.
		m_arrow_movement += m_arrow_speed;
		if (m_arrow_movement > m_arrow_max)
		{
			m_arrow_movement = m_arrow_max;
			m_arrow_speed = -m_arrow_speed;
		}
		else if (m_arrow_movement < m_arrow_min)
		{
			m_arrow_movement = m_arrow_min;
			m_arrow_speed = -m_arrow_speed;
		}
		m_selection_widget.posX = m_scene_objects[m_selected_object_ID].posX;
		m_selection_widget.posY = m_scene_objects[m_selected_object_ID].posY + 4.4f * m_scene_objects[m_selected_object_ID].scaY + m_arrow_movement;
		m_selection_widget.posZ = m_scene_objects[m_selected_object_ID].posZ;
	}

	// Update widgets.
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetTranslationWidget(), m_translation_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetRotationWidget(), m_rotation_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetScaleWidget(), m_scale_widget);
	m_d3d_renderer.UpdateDisplayObject(m_d3d_renderer.GetSelectionWidget(), m_selection_widget);

	// MOUSE RIGHT PRESSED /////////////////////////////////////////////////////////////////////////
	if (m_input.mouse_right_pressed)
	{
		// Snap cursor back to center of the screen for camera rotation.
		GetWindowRect(m_window_handle, &m_directX_window_rect);
		SetCursorPos(m_directX_window_rect.left + m_camera->m_width / 2, m_directX_window_rect.top + m_camera->m_height / 2);
		m_input.mouse_x = m_width / 2;
		m_input.mouse_y = m_height / 2;
	}

	// Setting last mouse position.
	m_input.last_mouse_x = m_input.mouse_x;
	m_input.last_mouse_y = m_input.mouse_y;

	// Mouse clicked and double should only be true for one frame.
	m_input.mouse_left_clicked = false;
	m_input.mouse_left_double_click = false;

	// setting most recent scene state in memory to be this scene state.
	m_scene_objects_memory[m_scene_index] = m_scene_objects;

	// debug
	m_d3d_renderer.scene_ID = m_scene_index;
	m_d3d_renderer.m_selected_object_ID = m_selected_object_ID;
	m_d3d_renderer.dragged = m_object_dragged;
	//

	m_d3d_renderer.Tick(&m_input);

}

void ToolMain::UpdateInput(MSG * msg)
{
	POINT m_mouse_screen_position;
	switch (msg->message)
	{
		//Global inputs,  mouse position and keys etc
	case WM_KEYDOWN:
	{
		m_input.ascii = msg->wParam;

		m_key_array[msg->wParam] = true;

		if (msg->wParam == VK_ESCAPE)
		{
			// QUIT
			PostQuitMessage(0);
		}

		break;
	}
	case WM_KEYUP:
		m_key_array[msg->wParam] = false;
		break;

	case WM_MOUSEMOVE:
		m_input.mouse_x = GET_X_LPARAM(msg->lParam);
		m_input.mouse_y = GET_Y_LPARAM(msg->lParam);
		GetCursorPos(&m_mouse_screen_position);
		GetWindowRect(m_window_handle, &m_directX_window_rect);
		if (m_mouse_screen_position.y > m_directX_window_rect.top)	// <--- cursor flickers in toolbar so just use default cursor in toolbar (dont need this if i can disable cursor in toolbar...)
		{
			// Set cursor for selected tool.
			SetCursor(m_cursor);
		}
		break;
	case WM_SETCURSOR:
		
		break;
	case WM_LBUTTONDOWN:
		// Mouse clicked only true for one frame.
		if (!m_input.mouse_left_pressed)
		{
			m_input.mouse_left_clicked = true;
		}
		m_input.mouse_left_pressed = true;
		break;
	case WM_LBUTTONUP:
		m_input.mouse_left_pressed = false;
		break;
	case WM_RBUTTONDOWN:
		m_input.mouse_right_pressed = true;
		break;
	case WM_RBUTTONUP:
		m_input.mouse_right_pressed = false;
		break;
	case WM_LBUTTONDBLCLK:
		m_input.mouse_left_pressed = true;
		m_input.mouse_left_double_click = true;
		break;
	case WM_MOUSEWHEEL:
	{
		if (GET_WHEEL_DELTA_WPARAM(msg->wParam) < 0)
		{
			if (m_input.mouse_scroll_wheel < 32)
			{
				m_input.mouse_scroll_wheel++;
			}
		}
		else
		{
			if (m_input.mouse_scroll_wheel > 0)
			{
				m_input.mouse_scroll_wheel--;
			}
		}
		break;
	}
	case WM_SIZE:
		//m_d3dRenderer.OnWindowSizeChanged(m_width, m_height);		<---- not sent...?
		break;
	default:
		break;
	}

	m_key_array['W'] ? m_input.forward = true : m_input.forward = false;
	m_key_array['S'] ? m_input.back = true : m_input.back = false;
	m_key_array['A'] ? m_input.left = true : m_input.left = false;
	m_key_array['D'] ? m_input.right = true : m_input.right = false;
	m_key_array['E'] ? m_input.up = true : m_input.up = false;
	m_key_array['Q'] ? m_input.down = true : m_input.down = false;

	// shift
	m_key_array[16] ? m_input.speed_boost = true : m_input.speed_boost = false;

	// DUPLICATE
	if (m_key_array['F'] && !m_last_key_array['F'])
	{
		Duplicate();
	}

	// UNDO
	if (m_key_array['C'] && !m_last_key_array['C'])
	{
		Undo();
	}

	// REDO
	if (m_key_array['V'] && !m_last_key_array['V'])
	{
		Redo();
	}

	// Set last key states.
	memcpy(&m_last_key_array[0], &m_key_array[0], sizeof(char) * 256);
}

