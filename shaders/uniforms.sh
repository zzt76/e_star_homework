uniform vec4 u_params[5];

#define u_lightPos u_params[0]
#define u_lightColor u_params[1]
#define u_viewPos u_params[2]
#define u_roughness u_params[3].x
#define u_metallic u_params[3].y
#define u_exposure u_params[3].z
#define u_usePBRMaps u_params[3].w
#define u_diffuseColor u_params[4]