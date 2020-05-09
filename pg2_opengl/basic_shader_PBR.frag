#version 460 core

#extension GL_ARB_bindless_texture : require

in vec3 ex_color;
in vec3 ex_normal;
in vec2 ex_tex_coord;
in vec3 ex_pos;      // pixel view space position
in vec3 ex_eye;
flat in int ex_material_index;
in mat3 ex_TBN;

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
uniform vec3 light_pos; // starting camera pos for ship
const vec3 light_color = vec3(1,1,1);

out vec4 FragColor;

in mat4x4 view_matrix;   // view (camera) transform

#define PI 3.1415926


// handy value clamping to 0 - 1 range
float saturate(in float value)
{
    return clamp(value, 0.0, 1.0);
}


// phong (lambertian) diffuse term
float phong_diffuse()
{
    return (1.0 / PI);
}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// following functions are copies from learnopengl.com
// for computing cook-torrance specular lighting terms

float D_GGX(in float roughness, in float NdH)
{
    float m = roughness * roughness;
    float m2 = m * m;
    float num   = m2;
	float NdH2 = NdH * NdH;
    float denom = (NdH2 * (m2 - 1.0) + 1.0);
    denom = PI * denom * denom;	
    return num / denom;
}
float GeometrySchlickGGX(float NdV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdV;
    float denom = NdV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}


void main()
{		
	//Material 
	vec3 norm = ex_normal;
	if(materials[ex_material_index].normal.x >= 0)
		norm = normalize(ex_TBN * normalize(texture(sampler2D(materials[ex_material_index].tex_normal_handle), ex_tex_coord ).rgb * 2.0 - 1.0));    
    // Fix oposite normal
    if(dot(norm, ex_eye) < 0)
        norm = -norm;
	vec3 albedo = texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), ex_tex_coord ).rgb * materials[ex_material_index].diffuse;
	albedo = pow(albedo, vec3(2.2));
	// roughness
    float roughness =texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).r * materials[ex_material_index].rma.x;

    // material params
    float metallic = texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).g * materials[ex_material_index].rma.y;

	float ambientOc = texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).b * materials[ex_material_index].rma.z;

    vec3 N = normalize(norm);
    vec3 V = normalize(ex_eye);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
   
   //compute radiance
        vec3 L = normalize(light_pos);
        vec3 H = normalize(V + L);
        float distance = length(light_pos - ex_pos) /1000;
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = light_color * attenuation;        
        
        // cook-torrance brdf
		float NdH = max(0.001, dot(N, H));

        float NDF = D_GGX(roughness, NdH);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kD = vec3(1.0) - F;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        // compute outgoing radiance Lo
        float NdL = max(dot(N, L), 0.0);                
        Lo = (kD * (albedo / PI) + specular) * radiance * NdL; 
  
    vec3 ambient = kD * albedo * ambientOc;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    FragColor = vec4(color, 1.0);
} 
                  