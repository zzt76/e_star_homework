$input v_pos, v_normal, v_texcoord0, v_shadowcoord

#include "../bgfx/examples/common/common.sh"

#define PI 3.14159265359
#define PI2 6.283185307179586
#define EPS 0.000001
#define NUM_SAMPLES 100
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10
#define BIAS 0.02

#include "uniforms.sh"

uniform vec4 u_time;

SAMPLERCUBE(s_texCube, 0);
SAMPLERCUBE(s_texCubeIrr, 1);
SAMPLER2D(s_shadowMap, 2);
SAMPLER2D(s_texDiffuse, 3);
SAMPLER2D(s_texNormal, 4);
SAMPLER2D(s_texAORM, 5);

vec3 getNormalFromMap(vec3 pos, vec3 normal, sampler2D texNormal, vec2 texcoord)
{
    vec3 tangentNormal = texture2D(texNormal, texcoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(pos);
    vec3 Q2  = dFdy(pos);
    vec2 st1 = dFdx(texcoord);
    vec2 st2 = dFdy(texcoord);

    vec3 N   = normalize(normal);
    vec3 T  = normalize(Q1*st2.y - Q2*st1.y);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(mul(TBN, tangentNormal));
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

float hardShadow(sampler2D _sampler, vec4 _shadowCoord, float _bias)
{
	vec3 texCoord = _shadowCoord.xyz/_shadowCoord.w;
	return step(texCoord.z-_bias, unpackRgbaToFloat(texture2D(_sampler, texCoord.xy) ) );
}

float rand_1to1(float x) {
  // -1 -1
  return fract(sin(x)*10000.0);
}

float rand_2to1(vec2 uv) {
  // 0 - 1
	const float a = 12.9898, b = 78.233, c = 43758.5453;
	float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

vec2 poissonDisk[NUM_SAMPLES];

void poissonDiskSamples(vec2 randomSeed) {

  float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
  float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

  float angle = rand_2to1( randomSeed ) * PI2;
  float radius = INV_NUM_SAMPLES;
  float radiusStep = radius;

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
    radius += radiusStep;
    angle += ANGLE_STEP;
  }
}

float PCF(sampler2D shadowMap, vec4 coords, float filterSize) {

  vec3 sp_coords = coords.xyz;
  float sp_depth = sp_coords.z;
  poissonDiskSamples(sp_coords.xy);
  //uniformDiskSamples(sp_coords.xy);
  float filter = 0.0;
  vec2 textureResolution = vec2_splat(512.0);

  for(int i=0; i < PCF_NUM_SAMPLES; i++){
    vec2 sample_coords = sp_coords.xy + poissonDisk[i] * filterSize / textureResolution;
    float sample_depth = unpackRgbaToFloat(texture2D(shadowMap, sample_coords));
    if(sp_depth <= (sample_depth + BIAS)){
      filter += 1.0;
    }
  }
  filter /= float(NUM_SAMPLES);

  return filter;
}

void main()
{
    // load uniforms
    vec3 viewPos = u_viewPos.xyz;
    vec3 lightPos = u_lightPos.xyz;
    vec3 lightColor = u_lightColor.xyz;
	vec3 normal = normalize(v_normal);

    float roughness = u_roughness;
    float metallic = u_metallic;
    float exposure = u_exposure;
    vec3 diffuseColor = u_diffuseColor.xyz;
    float ao = 1.0;

    if(u_usePbrMaps != 0.0){
        diffuseColor = texture2D(s_texDiffuse, v_texcoord0).rgb;
        normal = getNormalFromMap(v_pos, v_normal, s_texNormal, v_texcoord0);
        vec3 texAORM = texture2D(s_texAORM, v_texcoord0).rgb;
        ao = texAORM.r;
        roughness = texAORM.g;
        metallic = texAORM.b;
    }

    if(u_isFloor != 0.0){
        roughness = 1.0;
        metallic = 0.0;
        exposure = 2.2;
        diffuseColor = vec3_splat(1.0);
        ao = 1.0;
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

	// calculate Li
	float distance = length(lightPos - v_pos);
	float attenuation = 1.0 / (distance * distance);
	vec3 Li = lightColor * attenuation;

	// define direct and indirect lighting
	vec3 directLighting = vec3_splat(0.0);
	vec3 indirectLighting = vec3_splat(0.0);

    // Fresnel
    vec3 f0 = vec3_splat(0.04);
    f0 = mix(f0, diffuseColor, metallic);
    vec3 F = fresnelSchlick(HoV, f0);

	if(u_useBlinnPhong != 0.0){
        // diffuse
        vec3 diffuse = NoL * lightColor * diffuseColor;

        // specular
        float specularStrength = 0.5;
        vec3 specular = pow(HoV, 32) * specularStrength * lightColor * diffuseColor;

        directLighting = diffuse + specular;
	}

    if(u_usePBR != 0.0){
        // Distribution of microfacet normal
        float NDF = normalDistributionGGX(NoH, roughness);

        // Geometry: microfacet occlusion relationship
        float G = occlusionMicrofacet(NoV, NoL, roughness);

        // calculate specualr reflection energy
        vec3 Fs = (F * NDF * G) / (4.0 * NoV * NoL + EPS);

        // assume there is no refraction, then Fs + Fd = 1.0
        vec3 Fd = vec3_splat(1.0) - Fs;
        Fd *= 1.0 - metallic;

        // calculate final direct lighting
        directLighting = (Fd * diffuseColor / PI + Fs) * Li * NoL;
    }

    if(u_useDiffuseIBL == 0.0){
        // ambient
        float ambientStrength = 0.1;
        indirectLighting += ambientStrength * normalize(lightColor) * diffuseColor;
    }
    else{
        // ibl ambient
        vec3 kS = fresnelSchlick(NoV, f0, roughness);
        vec3 kD = vec3_splat(1.0) - kS;
        kD *= (1.0 - metallic);
        vec3 irradiance = toLinear(textureCube(s_texCubeIrr, normal).rgb);
        // vec3 irradiance = vec3_splat(0.2);
        vec3 indirectAmbient = irradiance * diffuseColor * kD;
        // vec3 indirectAmbient = irradiance * diffuseColor * texAO;

        indirectLighting += indirectAmbient * ao;
    }

    if(u_useSpecularIBL != 0.0){
        vec3 kS = fresnelSchlick(NoV, f0, roughness);
        // ibl specular
        vec3 reflectLightDir = -reflect(viewDir, normal);
        // vec3 reflectLightDir = 2.0 * NoV * normal - viewDir;
        float mip = 1.0 + 5.0 * roughness;
        reflectLightDir = fixCubeLookup(reflectLightDir, mip, 256.0);
        vec3 radiance = toLinear(textureCubeLod(s_texCube, reflectLightDir, mip).xyz);
        vec3 indirectSpecular = radiance * kS;
        indirectLighting += indirectSpecular * ao;
    }

	// apply shadow map
	float visibility = 1.0;
	if(u_useShadowMap != 0.0){
	    visibility = PCF(s_shadowMap, v_shadowcoord, u_pcfFilterSize);
    }

    // combine direct and indirect lighting
    // vec3 color = directLighting + indirectLighting;
    vec3 color = directLighting * visibility + indirectLighting;
    // color = vec3(u_usePBRMaps);

    // gamma correction
    color = color / (color + vec3_splat    (1.0));
    color = pow(color, vec3_splat(1.0 / exposure));

	gl_FragColor.xyz = color;
	// gl_FragColor.xyz = normal;
	// gl_FragColor.xyz = vec3(metallic, 0.0, 0.0);
    gl_FragColor.w = 1.0;
}