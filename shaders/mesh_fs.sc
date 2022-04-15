$input v_pos, v_view, v_normal, v_color0

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_time;
uniform vec4 u_lightPos;

void main()
{
	vec3 lightDir = normalize(u_lightPos.xyz - v_pos);
	vec3 normal = normalize(v_normal);
	vec3 viewDir = normalize(-v_view);
	vec3 halfVector = lightDir + viewDir;

	vec3 lightColor = vec3(1.0);

	float ambientFactor = 0.3;
	vec3 ambient = ambientFactor * lightColor;

	vec3 diffuse = max(dot(lightDir, normal), 0.0) * lightColor;

    float shininess = 16.0;
    float specularStrength = 0.5;
    vec3 specular = specularStrength * pow(max(dot(halfVector, normal),0.0), shininess) * lightColor;

	gl_FragColor.xyz = (ambient + diffuse + specular) * v_color0.xyz;
	gl_FragColor.w = 1.0;
}