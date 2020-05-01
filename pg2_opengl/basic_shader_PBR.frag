#version 460 core

#extension GL_ARB_bindless_texture : require

in vec3 ex_color;
in vec3 ex_normal;
in vec2 ex_tex_coord;
in vec3 ex_pos;      // pixel view space position
in vec3 ex_binormal; // binormal (for TBN basis calc)

flat in int ex_material_index;
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


out vec4 FragColor;


out vec4 color;

// mat4x4 world_matrix;  // object's world position
in mat4x4 view_matrix;   // view (camera) transform
//    mat4x4 proj_matrix;   // projection matrix
//    mat3x3 normal_matrix; // normal transformation matrix ( transpose(inverse(W * V)) )

vec4 material; // x - metallic, y - roughness, w - "rim" lighting
vec4 albedo = vec4(1.0, 0.0, 0.0, 1.0);   // constant albedo color, used when textures are off
//
//uniform samplerCube envd;  // prefiltered env cubemap
//uniform sampler2D tex;     // base texture (albedo)
//uniform sampler2D norm;    // normal map
//uniform sampler2D spec;    // "factors" texture (G channel used as roughness)
//uniform sampler2D iblbrdf; // IBL BRDF normalization precalculated tex
//
   
#define PI 3.1415926


// constant light position, only one light source for testing (treated as point light)
const vec4 light_pos = vec4(150,-500,400, 1);


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


// compute fresnel specular factor for given base specular and product
// product could be NdV or VdH depending on used technique
vec3 fresnel_factor(in vec3 f0, in float product)
{
    return mix(f0, vec3(1.0), pow(1.01 - product, 5.0));
}


// following functions are copies of UE4
// for computing cook-torrance specular lighting terms

float D_blinn(in float roughness, in float NdH)
{
    float m = roughness * roughness;
    float m2 = m * m;
    float n = 2.0 / m2 - 2.0;
    return (n + 2.0) / (2.0 * PI) * pow(NdH, n);
}

float D_beckmann(in float roughness, in float NdH)
{
    float m = roughness * roughness;
    float m2 = m * m;
    float NdH2 = NdH * NdH;
    return exp((NdH2 - 1.0) / (m2 * NdH2)) / (PI * m2 * NdH2 * NdH2);
}

float D_GGX(in float roughness, in float NdH)
{
    float m = roughness * roughness;
    float m2 = m * m;
    float d = (NdH * m2 - NdH) * NdH + 1.0;
    return m2 / (PI * d * d);
}

float G_schlick(in float roughness, in float NdV, in float NdL)
{
    float k = roughness * roughness * 0.5;
    float V = NdV * (1.0 - k) + k;
    float L = NdL * (1.0 - k) + k;
    return 0.25 / (V * L);
}


// cook-torrance specular calculation                      
vec3 cooktorrance_specular(in float NdL, in float NdV, in float NdH, in vec3 specular, in float roughness)
{
    float D = D_GGX(roughness, NdH);

    float G = G_schlick(roughness, NdV, NdL);

    float rim = mix(1.0 - roughness * material.w * 0.9, 1.0, NdV);

    return (1.0 / rim) * specular * G * D;
}

                      
void main() {
    // point light direction to point in view space
    vec3 local_light_pos = (view_matrix * (light_pos)).xyz;

    // light attenuation
    float A = 20.0 / dot(local_light_pos - ex_pos, local_light_pos - ex_pos);

    // L, V, H vectors
    vec3 L = normalize(local_light_pos - ex_pos);
    vec3 V = normalize(-ex_pos);
    vec3 H = normalize(L + V);
    vec3 nn = normalize(ex_normal);

    vec3 nb = normalize(ex_binormal);
    mat3x3 tbn = mat3x3(nb, cross(nn, nb), nn);
    vec2 texcoord = ex_tex_coord;

    // normal map
    // tbn basis
    vec3 N = tbn * (texture(sampler2D(materials[ex_material_index].tex_normal_handle), ex_tex_coord ).rgb * materials[ex_material_index].normal * 2.0 - 1.0);

    vec3 base =texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), ex_tex_coord ).rgb * materials[ex_material_index].diffuse ;
// roughness
    float roughness = material.y= texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).r;

    // material params
    float metallic = material.x = texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).b;

	float ambientOc = material.w = texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).g;

    // mix between metal and non-metal material, for non-metal
    // constant base specular factor of 0.04 grey is used
    vec3 specular = mix(vec3(0.04), base, metallic);

   
    // compute material reflectance
    float NdL = max(0.0, dot(N, L));
    float NdV = max(0.001, dot(N, V));
    float NdH = max(0.001, dot(N, H));
    float HdV = max(0.001, dot(H, V));
    float LdV = max(0.001, dot(L, V));

    // specular reflectance with COOK-TORRANCE
    vec3 specfresnel = fresnel_factor(specular, HdV);
    vec3 specref = cooktorrance_specular(NdL, NdV, NdH, specfresnel, roughness);

    specref *= vec3(NdL);

    // diffuse is common for any model
//    vec3 diffref = (vec3(1.0) - specfresnel) * phong_diffuse() * NdL;
    vec3 diffref = (vec3(1.0) - specfresnel) * (1 - metallic);

    // compute lighting
    vec3 reflected_light = vec3(0);
    vec3 diffuse_light = vec3(0); // initial value == constant ambient light

    // point light
    vec3 light_color = vec3(1.0);// * A;
    reflected_light += specref * light_color; // PrefEnvMap(Wi, a); Wi = reflect(Wo, n); a=roughness
    diffuse_light += diffref * light_color;

    // IBL lighting
    vec2 brdf = texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), vec2(roughness, 1.0 - NdV) ).rg; //BRDFIntMap
    vec3 iblspec = min(vec3(0.99), fresnel_factor(specular, NdV) * brdf.x + brdf.y);
    reflected_light += iblspec;

//     final result
    vec3 result =
        diffuse_light * mix(base, vec3(0.0), metallic) //diffuseLight * albedo/PI * IrradianceMap(n)
		+ reflected_light;
//
//	vec3 result = vec3(roughness, 0, 0);
    color = vec4(result, 1);
}
//
//void main( void )
//{
//	vec4 diff = vec4(texture(sampler2D(materials[ex_material_index].tex_diffuse_handle), ex_tex_coord ).rgb,1);
//
//	vec4 rma = vec4(texture(sampler2D(materials[ex_material_index].tex_rma_handle), ex_tex_coord ).rgb,1);
//	
//	vec4 norm = vec4(texture(sampler2D(materials[ex_material_index].tex_normal_handle), ex_tex_coord ).rgb,1);
//
//	FragColor =  rma;
//}