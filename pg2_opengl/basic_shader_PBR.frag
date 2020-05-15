#version 460 core

#extension GL_ARB_bindless_texture : require

in vec3 ex_color;
in vec3 ex_normal;
in vec2 ex_tex_coord;
in vec3 ex_pos;      // pixel view space position
in vec3 ex_eye;
flat in int ex_material_index;
in mat3 ex_TBN;
in mat4x4 normal_matrix;

uniform uvec2 brdf_map;
uniform uvec2 ir_map;
uniform uvec2 env_map;
uniform int max_level;


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
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}  
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

void main()
{		
	//Get material textures data 
	//bump mapping
	vec3 norm = normalize(ex_normal);
	if(materials[ex_material_index].normal.x < 0)
		norm = normalize(ex_TBN * normalize(texture(sampler2D(materials[ex_material_index].tex_normal_handle), ex_tex_coord ).rgb * 2.0 - 1.0));    

	//diffuse
	vec3 albedo = texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), ex_tex_coord ).rgb * materials[ex_material_index].diffuse;
	// rma
    float roughness =texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).r * materials[ex_material_index].rma.x;
	roughness = saturate(roughness); 

    float metallic = texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).g * materials[ex_material_index].rma.y;

	float ambientOc = texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).b;

	float ior = max(  materials[ex_material_index].rma.z, 1.0025);

	//prepare vectors
    vec3 N = normalize(norm);
	vec3 V = normalize(ex_eye - ex_pos);
	vec3 L = normalize(light_pos - ex_pos);
    vec3 H = normalize(V + L);
    vec3 R = reflect(-V, N);   

   // prepare Dot products
	float NdH = max(0.001, dot(N, H));		
	float NdL = max(dot(N, L), 0.001);                
	float NdV = max(dot(N, V), 0.001);                
	float HdV = max(dot(H, V), 0.001);     
	
   //compute radiance
        float dist = length(light_pos - ex_pos)/200;
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance     = light_color * attenuation;        
        
                

        float NDF = D_GGX(roughness, NdH);        
        float G   = GeometrySmith(N, V, L, roughness);  
		
		vec3 F0 = vec3((1 - ior) / (1 + ior));
		F0 = F0 * F0;
		vec3 F  = fresnelSchlick(HdV, F0);   
        
		vec3 kD = vec3(1.0) - F;
        kD *= 1.0 - metallic;	  

		vec3 numerator    = NDF * G * F;
		float denominator = 4.0 * NdV * NdL;
		vec3 specular  = numerator / max(denominator, 0.001);  
		
		//Compute outgoing radiance Lo
		vec3 Lo = (kD * (albedo / PI) *0.5 +  0.5*specular) * radiance * NdL; 


		//IBL
		 // ambient lighting (we now use IBL as the ambient term)
		F = fresnelSchlickRoughness(NdV, F0, roughness);    
		kD = 1.0 - F;
		kD *= 1.0 - metallic;	

		//BrdfIntMAp
		vec2 brdf = texture(sampler2D(brdf_map), vec2(NdV, roughness)).rg;
		if(brdf_map == vec2(0))
			brdf  =  vec2(1.0, 0.0);

		//Prefiltered Env Map
		vec3 prefilteredColor = textureLod(sampler2D( env_map), SampleSphericalMap(R), roughness * max_level).rgb ;   
		if(env_map == vec2(0))
			prefilteredColor =  vec3(1.0); 

		//irradiance map
		vec3 irradiance = texture(sampler2D(ir_map), SampleSphericalMap(N)).rgb;
		if(ir_map == vec2(0))
			irradiance =vec3(1.0); 
		
		vec3 spec = prefilteredColor *  (F * brdf.x + brdf.y);

		vec3 Ld = (albedo/PI) * irradiance;
		//Complete IBL
		vec3 Li = ( 0.5*kD * Ld + 0.5*spec) * ambientOc; 	

		//Complete PBR IBL
		vec3 color = Lo + Li;

		//gamma correction
		color = color / (color + vec3(1.0));
		color = pow(color, vec3(1.0/2.2));  

		FragColor = vec4(color, 1.0); //PBR
		//FragColor = vec4(ambientOc, ambientOc, ambientOc, 1.0); //Ao
		//FragColor = vec4(N * 0.5 + 0.5, 1.0); //Normal transformed from [-1, 1] to [0,1]
} 
                  