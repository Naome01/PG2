#include "pch.h"
#include "vertex.h"
#include "color.h"

Vertex::Vertex( const Vector3 position, const Vector3 normal, Vector3 color, Coord2f * texture_coords )
{
	this->position = position;
	this->normal = normal;
	this->color = color;

	if ( texture_coords != NULL )
	{
		for ( int i = 0; i < NO_TEXTURE_COORDS; ++i )
		{
			this->texture_coords[i] = texture_coords[i];
		}
	}	
}

MyVertex::MyVertex(Vertex v, int material_index)
{
	this->position = v.position;
	this->color = v.color;
	this->normal = v.normal;
	this->tangent = v.tangent;
	if (v.texture_coords != NULL)
	{
		for (int i = 0; i < NO_TEXTURE_COORDS; ++i)
		{
			this->texture_coords[i] = v.texture_coords[i];
		}
	}

	this->material_index = material_index;

}

