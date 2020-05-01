#version 460 core

#extension GL_ARB_bindless_texture : require

in vec3 ex_color;
in vec3 ex_normal;
in vec2 ex_tex_coord;
in float light;

flat in int ex_material_index;
struct Material
{
	vec3 diffuse;
	vec3 specular;
	vec3 ambient;
	uvec2 tex_diffuse_handle;
};

layout (std430, binding = 0) readonly buffer Materials
{
	Material materials[];
};


out vec4 FragColor;

void main( void )
{
	vec4 diff = vec4(materials[ex_material_index].diffuse.rgb * texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), ex_tex_coord ).rgb,1);

	vec4 amb = vec4(materials[ex_material_index].ambient.rgb, 1.0f);
	
	vec4 spec = vec4(materials[ex_material_index].specular.rgb, 1.0f);

//	FragColor =  diff ;
//	FragColor =  amb;
//	FragColor =  spec;
//	FragColor =  vec4(texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), ex_tex_coord ).rgb,1);

	FragColor =  (diff + spec) * light + amb;
//	FragColor =  vec4(ex_normal.rgb, 1);

}
