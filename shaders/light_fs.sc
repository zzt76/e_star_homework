$input v_pos, v_color0

#include "../bgfx/examples/common/common.sh"
#include "uniforms.sh"

uniform vec4 u_time;

void main()
{
    // visualize the rgb portions of the light color
    float sum = u_lightColor.x + u_lightColor.y + u_lightColor.z;
    float red = u_lightColor.x / sum;
    float green = u_lightColor.y / sum;
    float blue = u_lightColor.z / sum;
    vec3 color = vec3(red, green, blue) * 3.0f;
	gl_FragColor = vec4(color, 1.0f);
}