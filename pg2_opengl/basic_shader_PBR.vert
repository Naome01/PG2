#version 460 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_texcoord;

layout (location = 5) in int in_material_index;
struct Material
{
	vec3 diffuse;
    uvec2 tex_diffuse_handle;

	vec3 rma;
	uvec2 tex_rma_handle;

	vec3 normal;
	uvec2 tex_normal_handle;

};

layout (std430, binding = 0) readonly buffer Materials
{
	Material materials[];
};
uniform mat4 MVP;
uniform mat4 MV; 

out vec3 ex_color;
out vec3 ex_normal;
out vec3 ex_binormal;

out vec3 ex_pos;
out vec2 ex_tex_coord;
out vec3 ex_lightPos;
out float light;
out vec3 ex_color_amb;
out vec3 ex_color_spec;
out mat4x4 view_matrix;
flat out int ex_material_index;

void main( void )
{
	gl_Position = MVP * vec4(in_position.x, in_position.y, in_position.z, 1.0f);

	vec3 tangent;
	vec3 c1 = cross(in_normal, vec3(0.0, 0.0, 1.0));
	vec3 c2 = cross(in_normal, vec3(0.0, 1.0, 0.0));

	if (length(c1)>length(c2))
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	tangent = normalize(tangent);

	ex_binormal = cross(in_normal, tangent);
	ex_binormal = normalize(ex_binormal);
	ex_material_index = in_material_index;
	ex_color = in_color;
	ex_normal = in_normal;
	ex_tex_coord = in_texcoord;
	view_matrix = MV;
	ex_pos = (MV * vec4(in_position.x, in_position.y, in_position.z, 1.0f)).xyz;
}
