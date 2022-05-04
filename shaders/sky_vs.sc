$input a_position, a_texcoord0
$output v_dir, v_pos

#include "../bgfx/examples/common/common.sh"

void main()
{
    v_pos = a_position;

	gl_Position = mul(u_viewProj, vec4(a_position, 1.0) ).xyww;
}