#include <string>
#include "DisplayChunk.h"
#include "Game.h"

#include <map>

using namespace DirectX;
using namespace DirectX::SimpleMath;

DisplayChunk::DisplayChunk()
{
	//terrain size in meters. note that this is hard coded here, we COULD get it from the terrain chunk along with the other info from the tool if we want to be more flexible.
	m_terrainSize = 512;
	m_terrainHeightScale = 0.25;  //convert our 0-256 terrain to 64
	m_textureCoordStep = 1.0 / (TERRAINRESOLUTION-1);	//-1 becuase its split into chunks. not vertices.  we want tthe last one in each row to have tex coord 1
	m_terrainPositionScalingFactor = m_terrainSize / (TERRAINRESOLUTION-1);


	// Initializing bounding boxes for oprimized terrain collisions.

	m_boxes_resolution = 4;
	m_box_extent = m_terrainSize / m_boxes_resolution / 2;

	for (int z = -m_terrainSize / 2; z < m_terrainSize / 2; z += m_box_extent * 2)
	{
		m_boxes.push_back(std::vector<BoundingBox>());
		for (int x = -m_terrainSize / 2; x < m_terrainSize / 2; x += m_box_extent * 2)
		{
			BoundingBox box(XMFLOAT3(x + m_box_extent, 0, z + m_box_extent), XMFLOAT3(m_box_extent, m_box_extent, m_box_extent));
			m_boxes.back().push_back(box);
		}
	}
}


DisplayChunk::~DisplayChunk()
{
}

void DisplayChunk::PopulateChunkData(ChunkObject * SceneChunk)
{
	m_name = SceneChunk->name;
	m_chunk_x_size_metres = SceneChunk->chunk_x_size_metres;
	m_chunk_y_size_metres = SceneChunk->chunk_y_size_metres;
	m_chunk_base_resolution = SceneChunk->chunk_base_resolution;
	m_heightmap_path = SceneChunk->heightmap_path;
	m_tex_diffuse_path = SceneChunk->tex_diffuse_path;
	m_tex_splat_alpha_path = SceneChunk->tex_splat_alpha_path;
	m_tex_splat_1_path = SceneChunk->tex_splat_1_path;
	m_tex_splat_2_path = SceneChunk->tex_splat_2_path;
	m_tex_splat_3_path = SceneChunk->tex_splat_3_path;
	m_tex_splat_4_path = SceneChunk->tex_splat_4_path;
	m_render_wireframe = SceneChunk->render_wireframe;
	m_render_normals = SceneChunk->render_normals;
	m_tex_diffuse_tiling = SceneChunk->tex_diffuse_tiling;
	m_tex_splat_1_tiling = SceneChunk->tex_splat_1_tiling;
	m_tex_splat_2_tiling = SceneChunk->tex_splat_2_tiling;
	m_tex_splat_3_tiling = SceneChunk->tex_splat_3_tiling;
	m_tex_splat_4_tiling = SceneChunk->tex_splat_4_tiling;
}

void DisplayChunk::RenderBatch(std::shared_ptr<DX::DeviceResources>  DevResources)
{
	auto context = DevResources->GetD3DDeviceContext();

	m_terrainEffect->Apply(context);
	context->IASetInputLayout(m_terrainInputLayout.Get());

	m_batch->Begin();
	for (size_t i = 0; i < TERRAINRESOLUTION-1; i++)	//looping through QUADS.  so we subtrack one from the terrain array or it will try to draw a quad starting with the last vertex in each row. Which wont work
	{
		for (size_t j = 0; j < TERRAINRESOLUTION-1; j++)//same as above
		{
			m_batch->DrawQuad(m_terrainGeometry[i][j], m_terrainGeometry[i][j+1], m_terrainGeometry[i+1][j+1], m_terrainGeometry[i+1][j]); //bottom left bottom right, top right top left.
		}
	}
	m_batch->End();
}

void DisplayChunk::InitialiseBatch()
{
	//build geometry for our terrain array
	//iterate through all the vertices of our required resolution terrain.
	int index = 0;

	for (size_t i = 0; i < TERRAINRESOLUTION; i++)
	{
		for (size_t j = 0; j < TERRAINRESOLUTION; j++)
		{
			index = (TERRAINRESOLUTION * i) + j;
			m_terrainGeometry[i][j].position =			Vector3(j*m_terrainPositionScalingFactor-(0.5*m_terrainSize), (float)(m_height_map[index])*m_terrainHeightScale, i*m_terrainPositionScalingFactor-(0.5*m_terrainSize));	//This will create a terrain going from -64->64.  rather than 0->128.  So the center of the terrain is on the origin
			m_terrainGeometry[i][j].normal =			Vector3(0.0f, 1.0f, 0.0f);						//standard y =up
			m_terrainGeometry[i][j].textureCoordinate =	Vector2(((float)m_textureCoordStep*j)*m_tex_diffuse_tiling, ((float)m_textureCoordStep*i)*m_tex_diffuse_tiling);				//Spread tex coords so that its distributed evenly across the terrain from 0-1
		}
	}
	CalculateTerrainNormals();
}

void DisplayChunk::LoadHeightMap(std::shared_ptr<DX::DeviceResources>  DevResources)
{
	auto device = DevResources->GetD3DDevice();
	auto devicecontext = DevResources->GetD3DDeviceContext();

	//load in heightmap .raw
	FILE *pFile = NULL;

	// Open The File In Read / Binary Mode.

	fopen_s(&pFile, m_heightmap_path.c_str(), "rb");			// <---- Changed to use fopen_s as fopen is deprecated. Needed this for release mode.
	//pFile = fopen(m_heightmap_path.c_str(), "rb");

	// Check To See If We Found The File And Could Open It
	if (pFile == NULL)
	{
		// Display Error Message And Stop The Function
		MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
		return;
	}

	// Here We Load The .RAW File Into Our pHeightMap Data Array
	// We Are Only Reading In '1', And The Size Is (Width * Height)
	fread(m_height_map, 1, TERRAINRESOLUTION*TERRAINRESOLUTION, pFile);

	fclose(pFile);

	//load in texture diffuse
	
	//load the diffuse texture
	std::wstring texturewstr = StringToWCHART(m_tex_diffuse_path);
	HRESULT rs;	
	rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), NULL, &m_texture_diffuse);	//load tex into Shader resource	view and resource

	//setup terrain effect
	m_terrainEffect = std::make_unique<BasicEffect>(device);
	m_terrainEffect->EnableDefaultLighting();
	m_terrainEffect->SetLightingEnabled(true);
	m_terrainEffect->SetTextureEnabled(true);
	m_terrainEffect->SetTexture(m_texture_diffuse);

	void const* shaderByteCode;
	size_t byteCodeLength;

	m_terrainEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	//setup batch
	DX::ThrowIfFailed(
		device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
			VertexPositionNormalTexture::InputElementCount,
			shaderByteCode,
			byteCodeLength,
			m_terrainInputLayout.GetAddressOf())
		);

	m_batch = std::make_unique<PrimitiveBatch<VertexPositionNormalTexture>>(devicecontext);
}

void DisplayChunk::SaveHeightMap()
{
	FILE *pFile = NULL;

	// Open The File In Read / Binary Mode.
	fopen_s(&pFile, m_heightmap_path.c_str(), "wb+");;
	// Check To See If We Found The File And Could Open It
	if (pFile == NULL)
	{
		// Display Error Message And Stop The Function
		MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
		return;
	}

	fwrite(m_height_map, 1, TERRAINRESOLUTION*TERRAINRESOLUTION, pFile);
	fclose(pFile);
	
}

void DisplayChunk::UpdateTerrain()
{
	//all this is doing is transferring the height from the heigtmap into the terrain geometry.
	int index;
	for (size_t i = 0; i < TERRAINRESOLUTION; i++)
	{
		for (size_t j = 0; j < TERRAINRESOLUTION; j++)		
		{
			index = (TERRAINRESOLUTION * i) + j;
			m_terrainGeometry[i][j].position.y = (float)(m_height_map[index])*m_terrainHeightScale;	
		}
	}
	CalculateTerrainNormals();
}

Vector3 DisplayChunk::CaluclateClosestVertex(Vector3 & intersection)
{
	// Allready have functionality to get closest vertex mapped between 0 and 128 as created this first, so this is used and unmap. (could re-think...)
	
	Vector2 closest_grid_vertex = CaluclateClosestGridVertex(intersection);

	int index = (TERRAINRESOLUTION * closest_grid_vertex.y) + closest_grid_vertex.x;

	Vector3 closest_vertex((closest_grid_vertex.x * 4) - 256, m_height_map[index] * m_terrainHeightScale, (closest_grid_vertex.y * 4) - 256);

	return closest_vertex;
}

Vector3 DisplayChunk::CaluclateClosestGridVertex(Vector3& intersection)
{
	// corners of quad intersected with, mapped between 0 and 128:
	Vector2 top_left(((int)(intersection.x + 256)) / 4, ((int)(intersection.z + 256)) / 4);
	Vector2 top_right(top_left + Vector2(1, 0));
	Vector2 bottom_left(top_left + Vector2(0, 1));
	Vector2 bottom_right(top_left + Vector2(1, 1));

	// floating point x / z intersection point, mapped between 0 and 128:
	Vector2 intersection_point((intersection.x + 256) / 4, (intersection.z + 256) / 4);

	// Storing distances and intersection points in sorted map.
	std::map<float, Vector2, std::less<float>> distances_to_corners;
	distances_to_corners.insert({ Vector2::DistanceSquared(top_left, intersection_point), top_left });
	distances_to_corners.insert({ Vector2::DistanceSquared(top_right, intersection_point) , top_right });
	distances_to_corners.insert({ Vector2::DistanceSquared(bottom_left, intersection_point) , bottom_left });
	distances_to_corners.insert({ Vector2::DistanceSquared(bottom_right, intersection_point) , bottom_right });

	// closest corner is the first in the map.
	Vector2 closest_grid_vertex((int)distances_to_corners.begin()->second.x, (int)distances_to_corners.begin()->second.y);

	return closest_grid_vertex;
}

void DisplayChunk::SaveFlattenVertexHeight(Vector3 & intersection)
{
	m_saved_vertex_height = CaluclateClosestVertex(intersection).y;
}

void DisplayChunk::EditHeightmap(Vector3& intersection, int radius, EDIT_MODE edit_mode)
{
	// Calculate closest vertex to the intersection point.
	Vector2 closest_grid_vertex = CaluclateClosestGridVertex(intersection);

	// Calculate extents of a rectangle around the closest vertex the size of the terrain tool's radius, clamped to the edges of the heightmap.
	int x_start = std::max(closest_grid_vertex.x - radius, 0.0f);
	int z_start = std::max(closest_grid_vertex.y - radius, 0.0f);
	int x_stop = std::min((float)x_start + (radius * 2) + 1, (float)TERRAINRESOLUTION);
	int z_stop = std::min((float)z_start + (radius * 2) + 1, (float)TERRAINRESOLUTION);
	
	if (edit_mode == BUILD || edit_mode == DIG || edit_mode == FLATTEN)
	{
		// Loop over rectangle.
		for (int z = z_start; z < z_stop; z++)
		{
			for (int x = x_start; x < x_stop; x++)
			{
				// If vertex is within a circle with radius of terrain tool sphere.
				if(Vector2::DistanceSquared(Vector2(x, z), closest_grid_vertex) <= radius * radius)
				{
					// Calculate index of vertex in one dimensional heightmap.
					int index = (TERRAINRESOLUTION * z) + x;

					// Speed of raising and lowering terrain.
					int speed = 2;

					if (edit_mode == DIG)
					{
						// Lower terrain while it is greater than min height.
						if (m_height_map[index] >= speed)
						{
							m_height_map[index] -= speed;
						}
					}
					else if (edit_mode == BUILD)
					{
						// Raise terrain while it is lower than max height.
						if (m_height_map[index] < 255 - speed)
						{
							m_height_map[index] += speed;
						}
					}
					else if (edit_mode == FLATTEN)
					{
						// Set terrain to height of heing of initial vertex.
						m_height_map[index] = m_saved_vertex_height / m_terrainHeightScale;
					}
				}
			}
		}
	}
	else if (edit_mode == SMOOTH)
	{
		// Make a save of the current heightmap as need this for reference.
		BYTE old_height_map[TERRAINRESOLUTION*TERRAINRESOLUTION];
		memcpy(&old_height_map[0], &m_height_map[0], sizeof(BYTE) * TERRAINRESOLUTION*TERRAINRESOLUTION);

		// Loop over rectangle.
		for (int z = z_start + 1; z < z_stop - 1; z++)
		{
			for (int x = x_start + 1; x < x_stop - 1; x++)
			{
				// Calculate index of vertex in one dimensional heightmap.
				int index = (TERRAINRESOLUTION * z) + x;

				// Store indices of four surrounding vertices on x and z axis.
				int top_index = (TERRAINRESOLUTION * (z - 1)) + x;
				int bottom_index = (TERRAINRESOLUTION * (z + 1)) + x;
				int left_index = (TERRAINRESOLUTION * z) + (x - 1);
				int right_index = (TERRAINRESOLUTION * z) + (x + 1);

				// Calculating mid points between height of vertex and height of horizontal and vertical neighboring vertices. 
				float top = old_height_map[top_index] + ((old_height_map[index] - old_height_map[top_index]) * 0.5f);
				float bottom = old_height_map[bottom_index] + ((old_height_map[index] - old_height_map[bottom_index]) * 0.5f);
				float left = old_height_map[left_index] + ((old_height_map[index] - old_height_map[left_index]) * 0.5f);
				float right= old_height_map[right_index] + ((old_height_map[index] - old_height_map[right_index]) * 0.5f);

				// Setting height of vertex in heightmap to average of these heights. 
				m_height_map[index] = (top + bottom + left + right) / 4;
			}
		}
	}
	UpdateTerrain();
}

bool DisplayChunk::RayTriangleIntersct(Vector3 origin, Vector3 direction, Vector3 A, Vector3 B, Vector3 C, Vector3& intersection, float& distance)
{
// Switch from 1 to 0 to use own ray / triangle intersection. (directX function is faster)
#if	1	

	// Create ray.
	Ray ray;
	ray.position = origin;
	ray.direction = direction;
	ray.direction.Normalize();
	// Call overloaded intersects function with vertices of triangle.
	if (ray.Intersects(A, B, C, distance))
	{		
		intersection = origin + direction * distance;
		return true;
	}
	return false;

// I worked out every step of the process for this type of intersection. (heavily commented)
#else

	// to calculate the normal of the plane:
	// take 2 relative vectors on the plane and get the cross product
	Vector3 normal;
	(B - A).Cross((C - A), normal);  // <--- no need to normalise.

	// if the direction of the ray is parallel to the plane, there is no intersection.
	float normal_dot_direction = normal.Dot(direction);
	if (abs(normal_dot_direction) <= 0.00001f) return false;

	// the scaler equation of a plane is:
	// ax + by + cz = d
	// (a b c are components of the normal)
	// (x y z are components of an arbitrary point on the plane)
	// (d is the dot product of the two)
	float d = normal.Dot(A);

	// now we have all components of the equation of a plane.

	// equation of a line is:
	// r = a + t b
	// (r is the position vector of any point on the line)
	// (a is a given point on the line)
	// (b is the direction vector of the line)
	// (t is the scaler needed for b get to r from a)
	// in this case:
	// r is the point of intersection of the line and plane.
	// a is the origin.
	// b is direction.
	// in cartesian form:
	// | x | = | a.x | + t | b.x |
	// | y | = | a.y | + t | b.y |
	// | z | = | a.z | + t | b.z |
	// or
	// | x | = | origin.x | + t | direction.x |
	// | y | = | origin.y | + t | direction.y |
	// | z | = | origin.z | + t | direction.z |
	// substituting into equation of plane:
	// ax + by + cz = d 
	// or
	// normal.x(origin.x + t * direction.x) + normal.y(origin.y + t * direction.y) + normal.z(origin.z + t * direction.z) = d
	// expanding brackets:
	// (normal.x * origin.x) + (normal.x * direction.x * t) + (normal.y * origin.y) + (normal.y * direction.y * t) + (normal.z * origin.z) + (normal.z * direction.z * t) = d
	// rearanging for t:
	// (normal.x * direction.x * t) + (normal.y * direction.y * t) + (normal.z * direction.z * t) = d - (normal.x * origin.x) - (normal.y * origin.y) - (normal.z * origin.z)
	// or
	// (normal . direction) * t = d - (normal . origin)
	// t = (d - (normal . origin)) / (normal . direction)
	float t = (d - (normal.Dot(origin))) / normal.Dot(direction);

	// if the scaler needed for the direction vector to get to the point on the plane from the origin is negative, the plane is behind the projection point, so no intersection.
	if (t < 0) return false;

	// to calculate the point of intersection of the line and plane:
	// i = a + t b
	// (i is the intersection point of the line and plane)
	Vector3 I = origin + t * direction;

	// check if point of intersection is within the triangle on the plane. 
	// inside-outside test.

	// counter clockwize edge vectors from each corner:
	Vector3 AB = B - A;
	Vector3 BC = C - B;
	Vector3 CA = A - C;

	// vectors from each corner to the intersection point:
	Vector3 AI = I - A;
	Vector3 BI = I - B;
	Vector3 CI = I - C;

	// cross products of edge vectors and corner to intersection vectors:
	Vector3 ABcrossAI = AB.Cross(AI);
	Vector3 BCcrossBI = BC.Cross(BI);
	Vector3 CAcrossCI = CA.Cross(CI);

	// if the cross products of the edge vectors and corner to intersection vectors are pointing the same direction,
	// the intersection point is withing the triangle. (right hand rule)
	// if it is in the opposite direction,  it is outside the triangle.
	if (normal.Dot(ABcrossAI) < 0 || normal.Dot(BCcrossBI) < 0 || normal.Dot(CAcrossCI) < 0) return false;

	// set passed in intersection point and distance:
	intersection = I;
	distance = t;

	return true;
#endif
}

bool DisplayChunk::TerrainIntersect(Vector3 origin, Vector3 direction, Vector3& intersection, float& distance)
{
	// Map to store all triangle intersection points with their distances from the ray origin, sorted by shortest distance.
	std::map<float, Vector3, std::less<float>> intersections;

	// Number of terrain vertices in a bounding box.
	int box_resolution = TERRAINRESOLUTION / m_boxes_resolution;

	// Loop over bounding boxes.
	for (int z = 0; z < m_boxes_resolution; z++)
	{
		for (int x = 0; x < m_boxes_resolution; x++)
		{
			// If ray intersects with bounding box.
			if (m_boxes[z][x].Intersects(XMVECTOR{ origin.x, origin.y, origin.z, 1.0f }, XMVECTOR{ direction.x, direction.y, direction.z, 1.0f }, distance))
			{
				// Loop over all terrain mesh vertices in bounding box.
				for (int i = 0; i < (z == 3 ? box_resolution - 1 : box_resolution); i++)
				{
					for (int j = 0; j < (x == 3 ? box_resolution - 1 : box_resolution); j++)
					{
						// Vertices of corners of quad in terrain mesh.
						Vector3 top_left = m_terrainGeometry[z * box_resolution + i][x * box_resolution + j].position;
						Vector3 top_right = m_terrainGeometry[z * box_resolution + i + 1][x * box_resolution + j].position;
						Vector3 bottom_left = m_terrainGeometry[z * box_resolution + i][x * box_resolution + j + 1].position;
						Vector3 bottom_right = m_terrainGeometry[z * box_resolution + i + 1][x * box_resolution + j + 1].position;

						// Check for intersection in both triangles in the quad.
						if (RayTriangleIntersct(origin, direction, top_left, bottom_left, bottom_right, intersection, distance))
						{
							intersections.insert({ distance, intersection });
						}
						if (RayTriangleIntersct(origin, direction, top_left, bottom_right, top_right, intersection, distance))
						{
							intersections.insert({ distance, intersection });
						}
					}
				}
			}
		}
	}
	// If there is at least one intersection.
	if (intersections.size() > 0)
	{
		// The closest intersection point and distance is the first element of the map.
		intersection = intersections.begin()->second;
		distance = intersections.begin()->first;
		return true;
	}
	else
	{
		return false;
	}
}

void DisplayChunk::CalculateTerrainNormals()
{
	int index1, index2, index3, index4;
	DirectX::SimpleMath::Vector3 upDownVector, leftRightVector, normalVector;

	for (int i = 0; i<(TERRAINRESOLUTION - 1); i++)
	{
		for (int j = 0; j<(TERRAINRESOLUTION - 1); j++)
		{
			upDownVector.x = (m_terrainGeometry[i + 1][j].position.x - m_terrainGeometry[i - 1][j].position.x);
			upDownVector.y = (m_terrainGeometry[i + 1][j].position.y - m_terrainGeometry[i - 1][j].position.y);
			upDownVector.z = (m_terrainGeometry[i + 1][j].position.z - m_terrainGeometry[i - 1][j].position.z);

			leftRightVector.x = (m_terrainGeometry[i][j - 1].position.x - m_terrainGeometry[i][j + 1].position.x);
			leftRightVector.y = (m_terrainGeometry[i][j - 1].position.y - m_terrainGeometry[i][j + 1].position.y);
			leftRightVector.z = (m_terrainGeometry[i][j - 1].position.z - m_terrainGeometry[i][j + 1].position.z);


			leftRightVector.Cross(upDownVector, normalVector);	//get cross product
			normalVector.Normalize();			//normalise it.

			m_terrainGeometry[i][j].normal = normalVector;	//set the normal for this point based on our result
		}
	}
}
