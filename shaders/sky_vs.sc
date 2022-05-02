$input a_position, a_texcoord0
$output v_dir

#include "../bgfx/examples/common/common.sh"

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );

	float fov = radians(45.0);
	float height = tan(fov*0.5);
	float aspect = height*(u_viewRect.z / u_viewRect.w);
	vec2 tex = (2.0*a_texcoord0-1.0) * vec2(aspect, height);

	mat4 mtx;
	/*
	mtx[0] = u_mtx0;
	mtx[1] = u_mtx1;
	mtx[2] = u_mtx2;
	mtx[3] = u_mtx3;
	*/
	mtx[0] = vec4(1.0,0.0,0.0,0.0);
	mtx[1] = vec4(0.0,1.0,0.0,0.0);
	mtx[2] = vec4(0.0,0.0,1.0,0.0);
	mtx[3] = vec4(0.0,0.0,0.0,1.0);

	v_dir = instMul(u_view, vec4(tex, 1.0, 0.0) ).xyz;
}