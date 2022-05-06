$input a_position, a_color0
$output v_pos, v_color0

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_time;

void main()
{
	vec3 pos = a_position;
	gl_Position = mul(u_modelViewProj, vec4(pos, 1.0) );

	v_pos = gl_Position.xyz;
}