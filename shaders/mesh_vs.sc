$input a_position, a_normal, a_texcoord0
$output v_pos, v_view, v_normal, v_texcoord0

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_time;

void main()
{
	vec3 pos = a_position;

	vec3 normal = a_normal.xyz * 2.0 - 1.0;

	gl_Position = mul(u_modelViewProj, vec4(pos, 1.0) );

	v_pos = mul(u_model[0], vec4(pos, 1.0) ).xyz;

    v_normal = mul(u_model[0], vec4(normal, 0.0) ).xyz;

    v_texcoord0 = vec2(a_texcoord0.x, 1.0 - a_texcoord0.y);
}