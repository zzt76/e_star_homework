$input v_pos, v_view, v_normal, v_texcoord0

#include "../bgfx/examples/common/common.sh"

#define PI 3.14159265359
#define EPS 0.00001

uniform vec4 u_time;
uniform vec4 u_lightPos;
uniform vec4 u_viewPos;

SAMPLER2D(s_texDiffuse, 0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texAORM, 2);

vec3 fresnelSchlick(float HoV, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - HoV, 5.0);
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
    float k = r * r / 8.0;

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
	vec3 texDiffuse = texture2D(s_texDiffuse, v_texcoord0).rgb;
	vec3 texNormal = texture2D(s_texNormal, v_texcoord0).rgb;
	vec3 texAORM = texture2D(s_texAORM, v_texcoord0).rgb;
	float texAO = texAORM.r;
	float texRoughness = texAORM.g;
	// texRoughness = 0.1;
	float texMetallic = texAORM.b;
	// texMetallic = 1.0;

    vec3 viewPos = u_viewPos.xyz;
    vec3 lightPos = u_lightPos.xyz;

	vec3 lightDir = normalize(lightPos - v_pos);
	// vec3 normal = normalize(v_normal);
	vec3 normal = texNormal;
	vec3 viewDir = normalize(viewPos - v_pos);
	vec3 halfVector = normalize(lightDir + viewDir);

	// calculate related cosine
	float NoV = max(dot(viewDir, normal), 0.0);
	float NoL = max(dot(lightDir, normal), 0.0);
	float NoH = max(dot(halfVector, normal), 0.0);
	float HoV = max(dot(halfVector, viewDir), 0.0);

	// Lo
	vec3 Lo = vec3(0.0);

	// calculate Li
	vec3 lightColor = vec3(1.0);
	// float distance = length(lightPos - v_pos);
	// float attenuation = 1.0 / (distance * distance);
	// vec3 Li = lightColor * attenuation * NoL;
	vec3 Li = lightColor;

	// Fresnel
	vec3 f0 = vec3(0.04);
	f0 = mix(f0, texDiffuse, texMetallic);
	vec3 F = fresnelSchlick(HoV, f0);

	// Distribution of microfacet normal
    float NDF = normalDistributionGGX(NoH, texRoughness);

    // Geometry: microfacet occlusion relationship
    float G = occlusionMicrofacet(NoV, NoL, texRoughness);

    // calculate specualr reflection energy
    vec3 Fs = F * NDF * G / (4.0 * NoV * NoL + EPS);

    // assume there is no refraction, then Fs + Fd = 1.0
    vec3 Fd = 1.0 - Fs;
    Fd *= 1.0 - texMetallic;

    Lo += (Fd * texDiffuse / PI + Fs) * Li;

    // add ambient
    vec3 ambient = vec3(0.03) * texDiffuse * texAO;

	// final color
	gl_FragColor.xyz = Lo + ambient;
	// gl_FragColor.xyz = vec3(texMetallic, 0.0, 0.0);
    gl_FragColor.w = 1.0;
}