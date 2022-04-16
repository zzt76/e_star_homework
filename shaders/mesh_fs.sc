$input v_pos, v_view, v_normal, v_texcoord0

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_time;
uniform vec4 u_lightPos;
uniform vec4 u_viewPos;

SAMPLER2D(s_texDiffuse, 0);

void main()
{
    vec3 viewPos = u_viewPos.xyz;
    vec3 lightPos = u_lightPos.xyz;

	vec3 lightDir = normalize(lightPos - v_pos);
	vec3 normal = normalize(v_normal);
	vec3 viewDir = normalize(viewPos - v_pos);
	vec3 halfVector = normalize(lightDir + viewDir);

	vec3 lightColor = vec3(1.0);

	float ambientFactor = 0.1;
	vec3 ambient = ambientFactor * lightColor;

	vec3 diffuse = max(dot(lightDir, normal), 0.0) * lightColor;

    float shininess = 16.0;
    float specularStrength = 0.5;
    vec3 specular = specularStrength * pow(max(dot(halfVector, normal),0.0), shininess) * lightColor;

    vec4 pixel_color = texture2D(s_texDiffuse, v_texcoord0);
    vec3 illumination = ambient + diffuse + specular;
	gl_FragColor.xyz = illumination * pixel_color.xyz;
	// gl_FragColor.xyz = pixel_color.xyz;
	gl_FragColor.w = 1.0;
}