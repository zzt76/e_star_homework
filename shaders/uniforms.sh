uniform vec4 u_params[7];

#define u_lightPos u_params[0]
#define u_lightColor u_params[1]
#define u_viewPos u_params[2]
#define u_roughness u_params[3].x
#define u_metallic u_params[3].y
#define u_exposure u_params[3].z
#define u_usePbrMaps u_params[3].w
#define u_diffuseColor u_params[4]

#define u_isFloor u_params[5].x
#define u_pcfFilterSize u_params[5].y
#define u_useBlinnPhong u_params[5].z
#define u_usePBR u_params[5].w

#define u_useDiffuseIBL u_params[6].x
#define u_useSpecularIBL u_params[6].y
#define u_useShadowMap u_params[6].z

