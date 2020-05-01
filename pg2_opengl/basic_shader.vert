#version 460 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_texcoord;

layout (location = 5) in int in_material_index;

uniform mat4 MVP;
uniform mat4 MV; 

out vec3 ex_color;
out vec3 ex_normal;
out vec2 ex_tex_coord;
out vec3 ex_lightPos;
out float light;
out vec3 ex_color_amb;
out vec3 ex_color_spec;

flat out int ex_material_index;

void main( void )
{
	gl_Position = MVP * vec4(in_position.x, in_position.y, in_position.z, 1.0f);

	vec3 lightPossition = vec3(150, -500, 400);
	vec3 vectorToLight = normalize(lightPossition - in_position.xyz);

	vec3 unified_normal_es = normalize( ( MV * vec4( in_normal.xyz, 0.0f ) ).xyz );
	vec4 hit_es = MV * vec4(in_position.xyz,1.0f); // in_position = (nx, ny, nz, 0)
	vec3 omega_i_es = normalize( hit_es.xyz / hit_es.w );
	if ( dot( unified_normal_es, omega_i_es ) > 0.0f )
	{
		unified_normal_es *= -1.0f;
	}

	ex_material_index = in_material_index;

	vectorToLight = normalize((MV * vec4(vectorToLight, 0.0f)).xyz);

	light = dot(unified_normal_es, vectorToLight.xyz);

	ex_color = in_color;
	ex_normal = unified_normal_es;
	ex_tex_coord = in_texcoord;
}
