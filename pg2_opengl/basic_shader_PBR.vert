#version 460 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_texcoord;
layout (location = 3) in vec3 in_tangent;

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
uniform vec3 in_eye; 

out vec3 ex_color;
out vec3 ex_normal;
out vec3 ex_pos;
out vec3 ex_eye;
out mat3 ex_TBN;
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

	vec3 T = normalize(in_tangent);
	vec3 N = in_normal;
	vec3 B = cross(T, N);
	if(dot(cross(T, N), B) < 0)
		T = -T;
	ex_TBN = mat3(T, B, N);

	ex_material_index = in_material_index;
	ex_color = in_color;
	ex_normal = in_normal;
	ex_tex_coord = vec2(in_texcoord.x, 1.0 - in_texcoord.y);
	view_matrix = MV;
	ex_pos = gl_Position.rgb;
	ex_eye = in_eye;
}
