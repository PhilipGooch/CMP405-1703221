#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include "ChunkObject.h"

//geometric resoltuion - note,  hard coded.
#define TERRAINRESOLUTION 128

using namespace DirectX;
using namespace DirectX::SimpleMath;

class DisplayChunk
{
public:
	DisplayChunk();
	~DisplayChunk();

	enum EDIT_MODE
	{
		BUILD,
		DIG,
		FLATTEN,
		SMOOTH
	};

	void PopulateChunkData(ChunkObject * SceneChunk);
	void RenderBatch(std::shared_ptr<DX::DeviceResources>  DevResources);
	void InitialiseBatch();	//initial setup, base coordinates etc based on scale
	void LoadHeightMap(std::shared_ptr<DX::DeviceResources>  DevResources);
	void SaveHeightMap();			//saves the heigtmap back to file.
	void UpdateTerrain();			//updates the geometry based on the heigtmap	

	///

	// Calculates closest vertex to intersection point on terrain.
	Vector3 CaluclateClosestVertex(Vector3 & intersection);

	// Calculates closest vertex to intersection point mapped to between 0 and 128 for height map.
	Vector3 CaluclateClosestGridVertex(Vector3 & intersection);

	// Saves the height of the closest vertex to the ray intersection. (used for "Flatten" terrain tool)
	void SaveFlattenVertexHeight(Vector3 & intersection);

	// Edits height map depending on the terrain tool selected.
	void EditHeightmap(Vector3 & intersection, int radius, EDIT_MODE edit_mode);

	// Returns if there is an intersection between the ray and triangle provided. Stores intersection point and distance to it in 6th and 7th parameters.
	bool RayTriangleIntersct(Vector3 origin, Vector3 direction, Vector3 A, Vector3 B, Vector3 C, Vector3& intersection, float& distance);

	// Returns if there is an intersection between the ray and the terrain. Stores intersection point and distance to it in 3rd and 4th parameters.
	bool TerrainIntersect(Vector3 origin, Vector3 direction, Vector3& intersection, float& distance);

	///


	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionNormalTexture>>  m_batch;
	std::unique_ptr<DirectX::BasicEffect>       m_terrainEffect;

	ID3D11ShaderResourceView *					m_texture_diffuse;				//diffuse texture
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_terrainInputLayout;

private:
	
	DirectX::VertexPositionNormalTexture m_terrainGeometry[TERRAINRESOLUTION][TERRAINRESOLUTION];
	BYTE m_height_map[TERRAINRESOLUTION*TERRAINRESOLUTION];
	void CalculateTerrainNormals();

	float	m_terrainHeightScale;
	int		m_terrainSize;					//size of terrain in metres
	float	m_textureCoordStep;				//step in texture coordinates between each vertex row / column
	float   m_terrainPositionScalingFactor;	//factor we multiply the position by to convert it from its native resolution( 0- Terrain Resolution) to full scale size in metres dictated by m_Terrainsize
	
	std::string m_name;
	int m_chunk_x_size_metres;
	int m_chunk_y_size_metres;
	int m_chunk_base_resolution;
	std::string m_heightmap_path;
	std::string m_tex_diffuse_path;

	std::string m_tex_splat_alpha_path;
	std::string m_tex_splat_1_path;
	std::string m_tex_splat_2_path;
	std::string m_tex_splat_3_path;
	std::string m_tex_splat_4_path;
	bool m_render_wireframe;
	bool m_render_normals;
	int m_tex_diffuse_tiling;
	int m_tex_splat_1_tiling;
	int m_tex_splat_2_tiling;
	int m_tex_splat_3_tiling;
	int m_tex_splat_4_tiling;

	///

	// Bounding boxes for terrain intersection optimization.	
	std::vector<std::vector<BoundingBox>> m_boxes;
	// How many bounding boxes the terrain is split into.
	int m_boxes_resolution;
	// Size of the bounding boxes from the centre.
	int m_box_extent;

	// Variable to store the height of the closest vertex to the ray intersection. (used for "Flatten" terrain tool)
	float m_saved_vertex_height;
	///
};

