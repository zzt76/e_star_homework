$input v_dir, v_pos

#include "../bgfx/examples/common/common.sh"

SAMPLERCUBE(s_texCube, 0);
SAMPLERCUBE(s_texCubeIrr, 1);

void main()
{
    vec3 env_color = textureCubeLod(s_texCube, v_pos, 0).rgb;

    env_color = env_color / (env_color + vec3_splat(1.0));
    env_color = pow(env_color, vec3_splat(1.0/2.2));

    gl_FragColor = vec4(env_color, 1.0);
}