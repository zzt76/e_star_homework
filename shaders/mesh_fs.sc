$input v_pos, v_view, v_normal, v_texcoord0

#include "../bgfx/examples/common/common.sh"

#define PI 3.14159265359
#define EPS 0.000001

#include "uniforms.sh"

uniform vec4 u_time;
// uniform vec4 u_lightPos;
// uniform vec4 u_viewPos;

SAMPLERCUBE(s_texCubeIrr, 0);
SAMPLERCUBE(s_texCube, 1);
SAMPLER2D(s_texDiffuse, 2);
SAMPLER2D(s_texNormal, 3);
SAMPLER2D(s_texAORM, 4);

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(v_pos);
    vec3 Q2  = dFdy(v_pos);
    vec2 st1 = dFdx(v_texcoord0);
    vec2 st2 = dFdy(v_texcoord0);

    vec3 N   = normalize(v_normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 fresnelSchlick(float cos, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlick(float cos, vec3 f0, float roughness)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos, 0.0, 1.0), 5.0) * (1.0 - roughness);
}

float normalDistributionGGX(float NoH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NoH2 = NoH * NoH;
    float btm = NoH2 * (a2 - 1.0) + 1.0;

    return a2 / ( PI * btm * btm );
}

float occlusionSchlickGGX(float NoV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NoV / ( NoV * (1.0 - k) + k);
}

float occlusionMicrofacet(float NoV, float NoL, float roughness)
{
    float ggx1 = occlusionSchlickGGX(NoV, roughness);
    float ggx2 = occlusionSchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

void main()
{
	// load textures
	// vec3 texDiffuse = texture2D(s_texDiffuse, v_texcoord0).rgb;
	// vec3 texNormal = texture2D(s_texNormal, v_texcoord0).rgb;
	// vec3 texAORM = texture2D(s_texAORM, v_texcoord0).rgb;
	// float texAO = texAORM.r;
	float texAO = 1.0;
	// float texRoughness = texAORM.g;
	// float texRoughness = 1.0;
	// float texMetallic = texAORM.b;
	// float texMetallic = 0.1;

    // load uniforms
    vec3 viewPos = u_viewPos.xyz;
    vec3 lightPos = u_lightPos.xyz;
    vec3 lightColor = u_lightColor.xyz;
	vec3 normal = normalize(v_normal);
    float roughness = u_roughness;
    float metallic = u_metallic;
    float exposure = u_exposure;
    vec3 diffuseColor = vec3(1.0);
    float ao = 1.0;
    if(u_usePBRMaps == 1.0){
        diffuseColor = texture2D(s_texDiffuse, v_texcoord0).rgb;
        normal = getNormalFromMap();
        vec3 texAORM = texture2D(s_texAORM, v_texcoord0).rgb;
        ao = texAORM.r;
        roughness = texAORM.g;
        metallic = texAORM.b;
    }

	vec3 lightDir = normalize(lightPos - v_pos);
	// vec3 normal = getNormalFromMap();
	vec3 viewDir = normalize(viewPos - v_pos);
	vec3 halfVector = normalize(lightDir + viewDir);

	// calculate related cosine
	float NoV = max(dot(normal, viewDir), 0.0);
	float NoL = max(dot(normal, lightDir), 0.0);
	float NoH = max(dot(normal, halfVector), 0.0);
	float HoV = max(dot(halfVector, viewDir), 0.0);

	// Lo
	vec3 Lo = vec3(0.0);

	// calculate Li
	float distance = length(lightPos - v_pos);
	float attenuation = 1.0 / (distance * distance);
	vec3 Li = lightColor * attenuation;
	// Li = lightColor;

	// Fresnel
	vec3 f0 = vec3(0.04);
	f0 = mix(f0, diffuseColor, metallic);
	vec3 F = fresnelSchlick(HoV, f0);

	// Distribution of microfacet normal
    float NDF = normalDistributionGGX(NoH, roughness);

    // Geometry: microfacet occlusion relationship
    float G = occlusionMicrofacet(NoV, NoL, roughness);

    // calculate specualr reflection energy
    vec3 Fs = (F * NDF * G) / (4.0 * NoV * NoL + EPS);

    // assume there is no refraction, then Fs + Fd = 1.0
    vec3 Fd = vec3(1.0) - Fs;
    Fd *= 1.0 - metallic;

    // calculate final direct lighting
    vec3 directLighting = (Fd * diffuseColor / PI + Fs) * Li * NoL;

    // calculate indirect lighting
    // ibl ambient
    vec3 kS = fresnelSchlick(NoV, f0, roughness);
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);
	vec3 irradiance = toLinear(textureCube(s_texCubeIrr, normal).rgb);
	// vec3 irradiance = vec3(0.2);
	vec3 indirectAmbient = irradiance * diffuseColor * kD;
    // vec3 indirectAmbient = irradiance * diffuseColor * texAO;

	// ibl specular
	vec3 reflectLightDir = -reflect(viewDir, normal);
	// vec3 reflectLightDir = 2.0 * NoV * normal - viewDir;
	float mip = 1.0 + 5.0 * roughness;
	reflectLightDir = fixCubeLookup(reflectLightDir, mip, 256.0);
	vec3 radiance = toLinear(textureCubeLod(s_texCube, reflectLightDir, mip).xyz);
    vec3 indirectSpecular = radiance * kS;

    vec3 indirectLighting = (indirectAmbient + indirectSpecular) * ao;

    // combine direct and indirect lighting
	vec3 color = directLighting + indirectLighting;
	// color = vec3(u_usePBRMaps);

    // gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / exposure));

	gl_FragColor.xyz = color;
	// gl_FragColor.xyz = normal;
	// gl_FragColor.xyz = vec3(metallic, 0.0, 0.0);
    gl_FragColor.w = 1.0;
}