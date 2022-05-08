/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

/* EStarHomework
 * Team 09 - Zhang Zitian
 * */

#include <iostream>
#include "common.h"
#include "bgfx_utils.h"
#include "camera.h"
#include "imgui/imgui.h"
#include "FileBrowser/ImGuiFileBrowser.h"
#include "mesh_producer.h"

namespace RenderCore {

    constexpr int SHADOW_PASS_ID = 0;
    constexpr int SCENE_PASS_ID = 1;
    constexpr int SKYBOX_PASS_ID = 2;

    struct Uniforms {
        enum {
            NumVec4 = 7
        };

        void init() {
            u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, NumVec4);
        }

        void submit() {
            bgfx::setUniform(u_params, m_params, NumVec4);
        }

        void destroy() {
            bgfx::destroy(u_params);
        }

        union {
            struct {
                struct {
                    float u_lightPos[4];
                };
                struct {
                    float u_lightColor[4];
                };
                struct {
                    float u_viewPos[4];
                };
                struct {
                    float u_roughness, u_metallic, u_exposure, u_usePBRMaps;
                };
                struct {
                    float u_diffuseColor[4];
                };
                struct {
                    float u_isFloor, u_pcfFilterSize, u_useBlinnPhong, u_usePBR;
                };
                struct {
                    float u_useDiffuseIBL, u_useSpecularIBL, u_useShadowMap, temp1;
                };
            };

            float m_params[NumVec4 * 4];
        };

        bgfx::UniformHandle u_params;
    };

    struct Settings {
        Settings() {
            m_lightPos[0] = 0.0f;
            m_lightPos[1] = 10.0f;
            m_lightPos[2] = 2.0f;
            m_lightPos[3] = 1.0f;

            m_lightColor[0] = 500.0f;
            m_lightColor[1] = 500.0f;
            m_lightColor[2] = 500.0f;
            m_lightColor[3] = 1.0f;

            m_viewPos[0] = 0.0f;
            m_viewPos[1] = 5.0f;
            m_viewPos[2] = -20.0f;
            m_viewPos[3] = 1.0f;

            m_roughness = 0.5f;
            m_metallic = 0.1f;
            m_exposure = 2.2f;
            m_usePbrMaps = false;

            m_diffuseColor[0] = 1.0f;
            m_diffuseColor[1] = 1.0f;
            m_diffuseColor[2] = 1.0f;
            m_diffuseColor[3] = 1.0f;

            m_isFloor = false;
            m_pcfFilterSize = 5.0;
            m_useBlinnPhong = false;
            m_usePBR = true;

            m_useShadowMap = true;
            m_useDiffuseIBL = true;
            m_useSpecularIBL = true;

            m_meshPos[0] = 0.0f;
            m_meshPos[1] = 5.0f;
            m_meshPos[2] = 0.0f;
            m_meshPos[3] = 1.0f;

            // not passed to uniform
            m_visPbrStone = true;
            m_visSkyBox = true;
        }

        float m_lightPos[4];
        float m_viewPos[4];
        float m_lightColor[4];
        float m_roughness;
        float m_metallic;
        float m_exposure;
        bool m_visPbrStone;
        float m_diffuseColor[4];
        bool m_isFloor;
        bool m_usePbrMaps;
        float m_pcfFilterSize;
        bool m_useBlinnPhong;
        bool m_usePBR;
        bool m_useDiffuseIBL;
        bool m_useSpecularIBL;

        // not used in uniform
        bool m_useShadowMap;
        bool m_visSkyBox;
        float m_meshPos[4];
    };

    class EStarHomework : public entry::AppI {
    public:
        EStarHomework(const char *_name, const char *_description, const char *_url)
                : entry::AppI(_name, _description, _url),
                  m_pt(0) {}

        void init(int32_t _argc, const char *const *_argv, uint32_t _width, uint32_t _height) override {
            Args args(_argc, _argv);

            m_width = _width;
            m_height = _height;
            m_debug = BGFX_DEBUG_TEXT;
            m_reset = BGFX_RESET_VSYNC;
            m_timeOffset = bx::getHPCounter();

            bgfx::Init init;
            // init.type = args.m_type;
            // init.type = bgfx::RendererType::OpenGL;
            init.type = bgfx::RendererType::Direct3D11;
            init.vendorId = args.m_pciId;
            init.resolution.width = m_width;
            init.resolution.height = m_height;
            init.resolution.reset = m_reset;
            bgfx::init(init);

            // Enable debug text
            bgfx::setDebug(m_debug);

            // Set view 0 clear state
            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

            u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

            // init shadow map
            m_shadowMapSize = 512;
            s_shadowMap = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);
            u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);

            // When using GL clip space depth range [-1, 1] and packing depth into color buffer, we need to
            // adjust the depth range to be [0, 1] for writing to the color buffer
            u_depthScaleOffset = bgfx::createUniform("u_depthScaleOffset", bgfx::UniformType::Vec4);

            // Get renderer capabilities info.
            const bgfx::Caps *caps = bgfx::getCaps();

            float depthScaleOffset[4] = {1.0f, 0.0f, 0.0f, 0.0f};
            if (caps->homogeneousDepth) {
                depthScaleOffset[0] = 0.5f;
                depthScaleOffset[1] = 0.5f;
            }
            bgfx::setUniform(u_depthScaleOffset, depthScaleOffset);

            bgfx::touch(0);

            // init a plane
            Triangle::init_plane(m_planeVbh, m_planeIbh);

            // init shadow map program
            m_shadowProgram = loadProgram("shadow_vs", "shadow_fs");
            m_shadowMapFB = BGFX_INVALID_HANDLE;

            // init a cube light
            Triangle::init_cube(m_lightVbh, m_lightIbh);
            m_lightProgram = loadProgram("light_vs", "light_fs");
            // u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);

            // create mesh program from shaders
            m_pbrStone = meshLoad("../resource/pbr_stone/pbr_stone_mesh.bin");

            m_meshName = "../resource/basic_meshes/bunny.bin";
            m_mesh = meshLoad(m_meshName.c_str());
            if (!m_mesh) {
                std::cout << "mesh not load!" << std::endl;
                shutdown();
            }
            m_meshProgram = loadProgram("mesh_vs", "mesh_fs");
            // load texture and create texture sampler uniform
            m_texDiffuse = loadTexture(R"(../resource/pbr_stone/pbr_stone_base_color.dds)");
            s_texDiffuse = bgfx::createUniform("s_texDiffuse", bgfx::UniformType::Sampler);
            m_texNormal = loadTexture(R"(../resource/pbr_stone/pbr_stone_normal.dds)");
            s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
            m_texAORM = loadTexture(R"(../resource/pbr_stone/pbr_stone_aorm.dds)");
            s_texAORM = bgfx::createUniform("s_texAORM", bgfx::UniformType::Sampler);

            cameraCreate();
            cameraSetPosition(bx::Vec3(m_settings.m_viewPos[0], m_settings.m_viewPos[1], m_settings.m_viewPos[2]));

            m_skyBoxMesh = meshLoad(R"(../resource/basic_meshes/cube.bin)");
            m_skyBoxProgram = loadProgram("sky_vs", "sky_fs");
            m_texCube = loadTexture(R"(../resource/env_maps/kyoto_lod.dds)");
            s_texCube = bgfx::createUniform("s_texCube", bgfx::UniformType::Sampler);
            m_texCubeIrr = loadTexture(R"(../resource/env_maps/kyoto_irr.dds)");
            s_texCubeIrr = bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler);

            // some other meshes
            m_hollowCube = meshLoad(R"(../resource/basic_meshes/hollowcube.bin)");
            // load m_state
            m_state[0] = meshStateCreate();
            m_state[0]->m_state = 0;
            m_state[0]->m_program = m_shadowProgram;
            m_state[0]->m_viewId = SHADOW_PASS_ID;
            m_state[0]->m_numTextures = 0;

            m_state[1] = meshStateCreate();
            m_state[1]->m_state = 0
                                  | BGFX_STATE_WRITE_RGB
                                  | BGFX_STATE_WRITE_A
                                  | BGFX_STATE_WRITE_Z
                                  | BGFX_STATE_DEPTH_TEST_LESS
                                  | BGFX_STATE_CULL_CCW
                                  | BGFX_STATE_MSAA;
            m_state[1]->m_program = m_meshProgram;
            m_state[1]->m_viewId = SCENE_PASS_ID;
            m_state[1]->m_numTextures = 3;
            m_state[1]->m_textures[0].m_flags = 0;
            m_state[1]->m_textures[0].m_stage = 0;
            m_state[1]->m_textures[0].m_sampler = s_texCube;
            m_state[1]->m_textures[0].m_texture = m_texCube;
            m_state[1]->m_textures[1].m_flags = 0;
            m_state[1]->m_textures[1].m_stage = 1;
            m_state[1]->m_textures[1].m_sampler = s_texCubeIrr;
            m_state[1]->m_textures[1].m_texture = m_texCubeIrr;
            m_state[1]->m_textures[2].m_flags = UINT32_MAX;
            m_state[1]->m_textures[2].m_stage = 2;
            m_state[1]->m_textures[2].m_sampler = s_shadowMap;
            m_state[1]->m_textures[2].m_texture = m_shadowMap;

            // load settings and uniforms
            m_uniforms.init();

            imguiCreate();
        }

        virtual int shutdown() override {
            imguiDestroy();

            bgfx::destroy(m_lightVbh);
            bgfx::destroy(m_lightIbh);
            bgfx::destroy(m_lightProgram);

            meshUnload(m_mesh);
            bgfx::destroy(m_meshProgram);
            bgfx::destroy(m_texDiffuse);
            bgfx::destroy(s_texDiffuse);
            bgfx::destroy(m_texNormal);
            bgfx::destroy(s_texNormal);
            bgfx::destroy(m_texAORM);
            bgfx::destroy(s_texAORM);
            bgfx::destroy(u_time);

            meshUnload(m_skyBoxMesh);
            bgfx::destroy(m_skyBoxProgram);
            bgfx::destroy(m_texCube);
            bgfx::destroy(m_texCubeIrr);
            bgfx::destroy(s_texCube);
            bgfx::destroy(s_texCubeIrr);

            m_uniforms.destroy();
            meshUnload(m_hollowCube);
            meshStateDestroy(m_state[0]);
            meshStateDestroy(m_state[1]);
            bgfx::destroy(m_planeVbh);
            bgfx::destroy(m_planeIbh);
            bgfx::destroy(m_shadowMap);
            bgfx::destroy(s_shadowMap);
            bgfx::destroy(u_lightMtx);
            bgfx::destroy(u_depthScaleOffset);
            bgfx::destroy(m_shadowProgram);
            bgfx::destroy(m_shadowMapFB);

            cameraDestroy();

            // Shutdown bgfx.
            bgfx::shutdown();

            return 0;
        }

        bool update() override {
            if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) {
                imguiBeginFrame(m_mouseState.m_mx,
                                m_mouseState.m_my,
                                (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
                                | (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
                                | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0),
                                m_mouseState.m_mz, uint16_t(m_width), uint16_t(m_height)
                );

                showExampleDialog(this);


                ImGui::SetNextWindowPos(ImVec2(m_width - m_width / 4.0f - 8.0f, 10.0f),
                                        ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(m_width / 4.0f, m_height * 0.9f),
                                         ImGuiCond_FirstUseEver);
                ImGui::Begin("Settings", NULL, 0);

                ImGui::SliderFloat3("Light Pos", m_settings.m_lightPos, -50, 50);
                ImGui::SliderFloat3("Light Color", m_settings.m_lightColor, 0, 1000);
                ImGui::SliderFloat("Exposure", &m_settings.m_exposure, 1, 10);
                ImGui::Separator();
                ImGui::Text("Shadow Map:");
                ImGui::Checkbox("Use Shadow Map", &m_settings.m_useShadowMap);
                ImGui::SliderFloat("PCF Filter Size", &m_settings.m_pcfFilterSize, 1, 20);

                /* ImGui File Dialog from https://github.com/gallickgunner/ImGui-Addons
                 * Under MIT license
                 * */
                ImGui::Separator();
                ImGui::Text("Light Model:");
                ImGui::Checkbox("Diffuse IBL", &m_settings.m_useDiffuseIBL);
                ImGui::SameLine();
                ImGui::Checkbox("Specular IBL", &m_settings.m_useSpecularIBL);
                ImGui::Checkbox("Vis Skybox", &m_settings.m_visSkyBox);
                ImGui::Text("If not use IBL, use hardcoded ambient");
                if (ImGui::Checkbox("Blinn Phong", &m_settings.m_useBlinnPhong)) {
                    m_settings.m_usePBR = false;
                    m_settings.m_lightColor[0] = 1.0f;
                    m_settings.m_lightColor[1] = 1.0f;
                    m_settings.m_lightColor[2] = 1.0f;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("PBR", &m_settings.m_usePBR)) {
                    m_settings.m_useBlinnPhong = false;
                    m_settings.m_lightColor[0] = 500.0f;
                    m_settings.m_lightColor[1] = 500.0f;
                    m_settings.m_lightColor[2] = 500.0f;
                    ImGui::SetNextItemOpen(true);
                }

                if (ImGui::TreeNode("Meshes BRDF Params")) {
                    if (ImGui::Button("Open Mesh"))
                        ImGui::OpenPopup("Open Mesh");
                    if (file_dialog.showFileDialog("Open Mesh", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN,
                                                   ImVec2(700, 310), ".bin")) {
                        std::cout << file_dialog.selected_path
                                  << std::endl;    // The absolute path to the selected file
                        m_meshName = file_dialog.selected_path;
                        m_mesh = meshLoad(m_meshName.c_str());
                    }

                    ImGui::ColorEdit3("Diffuse Color", m_settings.m_diffuseColor);
                    ImGui::SliderFloat3("Mesh Pos", m_settings.m_meshPos, -50.0, 50.0);
                    ImGui::SliderFloat("Roughness", &m_settings.m_roughness, 0.0, 1);
                    ImGui::SliderFloat("Metallic", &m_settings.m_metallic, 0.0, 1);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("PBR Stone")) {
                    ImGui::Checkbox("Visualize PBR Stone", &m_settings.m_visPbrStone);
                    if (ImGui::Button("Load Diffuse"))
                        ImGui::OpenPopup("Load Diffuse");
                    if (file_dialog.showFileDialog("Load Diffuse", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN,
                                                   ImVec2(700, 310), ".dds")) {
                        m_texDiffuse = loadTexture(file_dialog.selected_path.c_str());
                    }
                    if (ImGui::Button("Load Normal"))
                        ImGui::OpenPopup("Load Normal");
                    if (file_dialog.showFileDialog("Load Normal", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN,
                                                   ImVec2(700, 310), ".dds")) {
                        m_texNormal = loadTexture(file_dialog.selected_path.c_str());
                    }
                    if (ImGui::Button("Load AORM"))
                        ImGui::OpenPopup("Load AORM");
                    if (file_dialog.showFileDialog("Load AORM", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN,
                                                   ImVec2(700, 310), ".dds")) {
                        m_texAORM = loadTexture(file_dialog.selected_path.c_str());
                    }
                    ImGui::TreePop();
                }

                ImGui::End();

                imguiEndFrame();

                // get delta time
                int64_t now = bx::getHPCounter();
                static int64_t last = now;
                const int64_t frameTime = now - last;
                last = now;
                const auto freq = double(bx::getHPFrequency());
                const auto time = (float) ((now - m_timeOffset) / double(bx::getHPFrequency()));
                bgfx::setUniform(u_time, &time);
                const auto deltaTime = float(frameTime / freq);
                cameraUpdate(deltaTime, m_mouseState);

                // shadow map settings
                if (!bgfx::isValid(m_shadowMapFB)) {
                    bgfx::TextureHandle shadowMapTexture = BGFX_INVALID_HANDLE;

                    m_shadowProgram = loadProgram("shadow_vs", "shadow_fs");

                    bgfx::TextureHandle fbtextures[] =
                            {
                                    bgfx::createTexture2D(
                                            m_shadowMapSize, m_shadowMapSize, false, 1, bgfx::TextureFormat::BGRA8,
                                            BGFX_TEXTURE_RT
                                    ),
                                    bgfx::createTexture2D(
                                            m_shadowMapSize, m_shadowMapSize, false, 1, bgfx::TextureFormat::D16,
                                            BGFX_TEXTURE_RT_WRITE_ONLY
                                    ),
                            };

                    shadowMapTexture = fbtextures[0];
                    m_shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);

                    m_state[0]->m_program = m_shadowProgram;
                    m_state[0]->m_state = 0
                                          | BGFX_STATE_WRITE_A
                                          | BGFX_STATE_WRITE_Z
                                          | BGFX_STATE_DEPTH_TEST_LESS
                                          | BGFX_STATE_CULL_CCW
                                          | BGFX_STATE_MSAA;

                    m_state[1]->m_program = m_meshProgram;
                    m_state[1]->m_textures[2].m_texture = shadowMapTexture;
                    m_shadowMap = shadowMapTexture;
                    // bgfx::setTexture(0, s_shadowMap, shadowMapTexture);
                }

                // load camera position to settings
                auto camPos = cameraGetPosition();
                m_settings.m_viewPos[0] = camPos.x;
                m_settings.m_viewPos[1] = camPos.y;
                m_settings.m_viewPos[2] = camPos.z;

                // load settings to uniforms
                m_uniforms.u_roughness = m_settings.m_roughness;
                m_uniforms.u_metallic = m_settings.m_metallic;
                m_uniforms.u_exposure = m_settings.m_exposure;
                m_uniforms.u_isFloor = m_settings.m_isFloor;
                m_uniforms.u_pcfFilterSize = m_settings.m_pcfFilterSize;
                m_uniforms.u_useBlinnPhong = m_settings.m_useBlinnPhong;
                m_uniforms.u_usePBR = m_settings.m_usePBR;
                m_uniforms.u_useDiffuseIBL = m_settings.m_useDiffuseIBL;
                m_uniforms.u_useSpecularIBL = m_settings.m_useSpecularIBL;
                m_uniforms.u_useShadowMap = m_settings.m_useShadowMap;
                bx::memCopy(m_uniforms.u_lightPos, m_settings.m_lightPos, 4 * sizeof(float));
                bx::memCopy(m_uniforms.u_lightColor, m_settings.m_lightColor, 4 * sizeof(float));
                bx::memCopy(m_uniforms.u_viewPos, m_settings.m_viewPos, 4 * sizeof(float));
                bx::memCopy(m_uniforms.u_diffuseColor, m_settings.m_diffuseColor, 4 * sizeof(float));

                // define matrices
                float lightView[16];
                float lightProj[16];

                const bx::Vec3 at = {0.0f, 0.0f, 0.0f};
                const bx::Vec3 eye = {m_settings.m_lightPos[0], m_settings.m_lightPos[1], m_settings.m_lightPos[2]};
                bx::mtxLookAt(lightView, eye, at);

                const bgfx::Caps *caps = bgfx::getCaps();
                const float area = 30.0f;
                bx::mtxOrtho(lightProj, -area, area, -area, area, -100.0f, 100.0f, 0.0f, caps->homogeneousDepth);

                // cross-platform texture coordinate procession
                float mtxShadow[16];
                float lightMtx[16];

                const float sy = caps->originBottomLeft ? 0.5f : -0.5f;
                const float sz = caps->homogeneousDepth ? 0.5f : 1.0f;
                const float tz = caps->homogeneousDepth ? 0.5f : 0.0f;
                const float mtxCrop[16] =
                        {
                                0.5f, 0.0f, 0.0f, 0.0f,
                                0.0f, sy, 0.0f, 0.0f,
                                0.0f, 0.0f, sz, 0.0f,
                                0.5f, 0.5f, tz, 1.0f,
                        };

                float mtxTmp[16];
                bx::mtxMul(mtxTmp, lightProj, mtxCrop);
                bx::mtxMul(mtxShadow, lightView, mtxTmp);

                // set shadow map pass
                bgfx::setViewRect(SHADOW_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
                bgfx::setViewFrameBuffer(SHADOW_PASS_ID, m_shadowMapFB);
                bgfx::setViewTransform(SHADOW_PASS_ID, lightView, lightProj);
                bgfx::setViewClear(SHADOW_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

                // set scene pass
                bgfx::setViewRect(SCENE_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
                bgfx::setViewClear(SCENE_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
                // view and proj matrix for
                float viewMatrix[16];
                float projMatrix[16];
                cameraGetViewMtx(viewMatrix);
                bx::mtxProj(projMatrix, cameraGetFoV(), float(m_width) / float(m_height),
                            0.1f, 100.0f, caps->homogeneousDepth);
                // view transform 0 for mesh
                bgfx::setViewTransform(SCENE_PASS_ID, viewMatrix, projMatrix);

                // set skybox pass
                bgfx::setViewRect(SKYBOX_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
                bgfx::setViewClear(SKYBOX_PASS_ID, 0, 0x303030ff, 1.0f, 0);

                // render pbr stone
                if (m_settings.m_visPbrStone) {
                    m_uniforms.u_usePBRMaps = true;
                    float modelStone[16];
                    // bx::mtxRotateXY(modelStone, 0.0f, time * 0.37f);
                    bx::mtxSRT(modelStone, 0.3f, 0.3f, 0.3f, 0.0f, 0.0f, 0.0f, 5.0f, 3.0f, 5.0f
                    );

                    // shadow pass
                    bx::mtxMul(lightMtx, modelStone, mtxShadow);
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    meshSubmit(m_pbrStone, &m_state[0], 1, modelStone);

                    // scene pass
                    uint64_t state = 0
                                     | BGFX_STATE_WRITE_RGB
                                     | BGFX_STATE_WRITE_Z
                                     | BGFX_STATE_DEPTH_TEST_LESS
                                     | BGFX_STATE_MSAA;
                    // set texture
                    bgfx::setTexture(0, s_texCube, m_texCube);
                    bgfx::setTexture(1, s_texCubeIrr, m_texCubeIrr);
                    bgfx::setTexture(2, s_shadowMap, m_shadowMap, UINT32_MAX);
                    bgfx::setTexture(3, s_texDiffuse, m_texDiffuse);
                    bgfx::setTexture(4, s_texNormal, m_texNormal);
                    bgfx::setTexture(5, s_texAORM, m_texAORM);
                    // submit uniforms
                    m_uniforms.submit();
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    // draw mesh
                    meshSubmit(m_pbrStone, SCENE_PASS_ID, m_meshProgram, modelStone, state);
                    // set: not use pbr maps for other meshes
                    m_uniforms.u_usePBRMaps = false;
                }

                // render floor
                {
                    float mtxFloor[16];
                    bx::mtxSRT(mtxFloor, 30.0f, 30.0f, 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
                    );
                    bx::mtxMul(lightMtx, mtxFloor, mtxShadow);

                    // shadow pass
                    bgfx::setIndexBuffer(m_planeIbh);
                    bgfx::setVertexBuffer(0, m_planeVbh);
                    bgfx::setState(m_state[SHADOW_PASS_ID]->m_state);
                    bgfx::setTransform(mtxFloor);
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    bgfx::submit(SHADOW_PASS_ID, m_shadowProgram);

                    // scene pass
                    bgfx::setIndexBuffer(m_planeIbh);
                    bgfx::setVertexBuffer(0, m_planeVbh);
                    bgfx::setState(m_state[SCENE_PASS_ID]->m_state);
                    bgfx::setTransform(mtxFloor);
                    bgfx::setTexture(0, s_texCube, m_texCube);
                    bgfx::setTexture(1, s_texCubeIrr, m_texCubeIrr);
                    bgfx::setTexture(2, s_shadowMap, m_shadowMap);
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    m_settings.m_isFloor = true;
                    m_uniforms.u_isFloor = m_settings.m_isFloor;
                    m_uniforms.submit();
                    bgfx::submit(SCENE_PASS_ID, m_meshProgram);
                }

                m_settings.m_isFloor = false;
                m_uniforms.u_isFloor = m_settings.m_isFloor;

                // render light
                { // Current primitive topology
                    uint64_t state = 0
                                     | BGFX_STATE_WRITE_RGB
                                     | BGFX_STATE_WRITE_Z
                                     | BGFX_STATE_DEPTH_TEST_LESS
                                     | BGFX_STATE_CULL_CW
                                     | BGFX_STATE_MSAA
                                     | BGFX_STATE_PT_TRISTRIP;
                    float modelLight[16];
                    m_uniforms.submit();
                    bx::mtxSRT(modelLight,
                               0.5f, 0.5f, 0.5f,
                               0.0f, 0.0f, 0.0f,
                               m_settings.m_lightPos[0],
                               m_settings.m_lightPos[1],
                               m_settings.m_lightPos[2]);
                    // Set model matrix for rendering.
                    bgfx::setTransform(modelLight);
                    // Set vertex and index buffer.
                    bgfx::setVertexBuffer(SCENE_PASS_ID, m_lightVbh);
                    bgfx::setIndexBuffer(m_lightIbh);
                    // Set render states.
                    bgfx::setState(state);
                    // Submit primitive for rendering to view 0.
                    bgfx::submit(SCENE_PASS_ID, m_lightProgram);
                }

                // render hollow cube
                {
                    float modelHollowCube[16];
                    bx::mtxSRT(modelHollowCube, 1.0f, 1.0f, 1.0f, 0.0f, 1.56f - time, 0.0f, -20.0f, 5.0f, 20.0f
                    );
                    // shadow pass
                    bx::mtxMul(lightMtx, modelHollowCube, mtxShadow);
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    meshSubmit(m_hollowCube, &m_state[0], 1, modelHollowCube);

                    // scene pass
                    // submit uniforms
                    m_uniforms.submit();
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    // draw mesh
                    meshSubmit(m_hollowCube, &m_state[1], 1, modelHollowCube);
                }

                // render mesh
                {
                    float modelMesh[16];
                    // bx::mtxRotateXY(model_matrix, 0.0f, time * 0.37f);
                    bx::mtxTranslate(modelMesh, m_settings.m_meshPos[0], m_settings.m_meshPos[1],
                                     m_settings.m_meshPos[2]);

                    // shadow pass
                    bx::mtxMul(lightMtx, modelMesh, mtxShadow);
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    meshSubmit(m_mesh, &m_state[0], 1, modelMesh);

                    // scene pass
                    // submit uniforms
                    m_uniforms.submit();
                    bgfx::setUniform(u_lightMtx, lightMtx);
                    // draw mesh
                    meshSubmit(m_mesh, &m_state[1], 1, modelMesh);
                }

                // render sky box
                if (m_settings.m_visSkyBox) {
                    bgfx::setTexture(0, s_texCube, m_texCube);
                    bgfx::setTexture(1, s_texCubeIrr, m_texCubeIrr);
                    uint64_t state = 0
                                     | BGFX_STATE_WRITE_RGB
                                     | BGFX_STATE_DEPTH_TEST_LEQUAL;
                    float view_sky[16];
                    cameraGetViewMtx(view_sky);
                    view_sky[3] = 0;
                    view_sky[7] = 0;
                    view_sky[11] = 0;
                    view_sky[12] = 0;
                    view_sky[13] = 0;
                    view_sky[14] = 0;
                    float proj_sky[16];
                    bx::mtxProj(proj_sky, cameraGetFoV(), float(m_width) / float(m_height),
                                0.1f, 100.0f, caps->homogeneousDepth);
                    bgfx::setViewTransform(SKYBOX_PASS_ID, view_sky, proj_sky);
                    float model_sky[16];
                    bx::mtxIdentity(model_sky);
                    meshSubmit(m_skyBoxMesh, SKYBOX_PASS_ID, m_skyBoxProgram, model_sky, state);
                }

                // Advance to next frame. Rendering thread will be kicked to
                // process submitted rendering primitives.
                bgfx::frame();

                return true;
            }

            return false;
        }

        entry::MouseState m_mouseState;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_debug;
        uint32_t m_reset;

        bgfx::VertexBufferHandle m_lightVbh;
        bgfx::IndexBufferHandle m_lightIbh;
        bgfx::ProgramHandle m_lightProgram;

        Mesh *m_pbrStone;

        Mesh *m_mesh;
        std::string m_meshName;
        bgfx::ProgramHandle m_meshProgram;
        bgfx::TextureHandle m_texDiffuse;
        bgfx::UniformHandle s_texDiffuse;
        bgfx::TextureHandle m_texNormal;
        bgfx::UniformHandle s_texNormal;
        bgfx::TextureHandle m_texAORM;
        bgfx::UniformHandle s_texAORM;

        Mesh *m_skyBoxMesh;
        bgfx::ProgramHandle m_skyBoxProgram;
        bgfx::TextureHandle m_texCube;
        bgfx::TextureHandle m_texCubeIrr;
        bgfx::UniformHandle s_texCube;
        bgfx::UniformHandle s_texCubeIrr;

        bgfx::UniformHandle u_time;
        int64_t m_timeOffset;
        int32_t m_pt;

        // shadow map related
        Mesh *m_hollowCube;
        MeshState *m_state[2];
        bgfx::VertexBufferHandle m_planeVbh;
        bgfx::IndexBufferHandle m_planeIbh;
        uint16_t m_shadowMapSize;
        bgfx::TextureHandle m_shadowMap;
        bgfx::UniformHandle s_shadowMap;
        bgfx::UniformHandle u_lightMtx;
        bgfx::UniformHandle u_depthScaleOffset;
        bgfx::ProgramHandle m_shadowProgram;
        bool m_shadowSamplerSupported;
        bgfx::FrameBufferHandle m_shadowMapFB;

        // settings
        Settings m_settings;
        Uniforms m_uniforms;

        imgui_addons::ImGuiFileBrowser file_dialog;
    };

} // namespace

int _main_(int _argc, char **_argv) {
    RenderCore::EStarHomework app("e-star-homework", "", "");
    return entry::runApp(&app, _argc, _argv);
}

