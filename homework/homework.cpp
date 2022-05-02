/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <iostream>
#include "common.h"
#include "bgfx_utils.h"
#include "camera.h"
#include "imgui/imgui.h"
#include "FileBrowser/ImGuiFileBrowser.h"
#include "meshproducer.h"

namespace RenderCore {

    static float s_texelHalf = 0.0f;

    struct PosColorTexCoord0Vertex {
        float m_x;
        float m_y;
        float m_z;
        uint32_t m_rgba;
        float m_u;
        float m_v;

        static void init() {
            ms_layout
                    .begin()
                    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                    .end();
        }

        static bgfx::VertexLayout ms_layout;
    };

    bgfx::VertexLayout PosColorTexCoord0Vertex::ms_layout;

    void screenSpaceQuad(float _textureWidth, float _textureHeight, bool _originBottomLeft = false, float _width = 1.0f,
                         float _height = 1.0f) {
        if (3 == bgfx::getAvailTransientVertexBuffer(3, PosColorTexCoord0Vertex::ms_layout)) {
            bgfx::TransientVertexBuffer vb;
            bgfx::allocTransientVertexBuffer(&vb, 3, PosColorTexCoord0Vertex::ms_layout);
            PosColorTexCoord0Vertex *vertex = (PosColorTexCoord0Vertex *) vb.data;

            const float zz = 0.0f;

            const float minx = -_width;
            const float maxx = _width;
            const float miny = 0.0f;
            const float maxy = _height * 2.0f;

            const float texelHalfW = s_texelHalf / _textureWidth;
            const float texelHalfH = s_texelHalf / _textureHeight;
            const float minu = -1.0f + texelHalfW;
            const float maxu = 1.0f + texelHalfW;

            float minv = texelHalfH;
            float maxv = 2.0f + texelHalfH;

            if (_originBottomLeft) {
                std::swap(minv, maxv);
                minv -= 1.0f;
                maxv -= 1.0f;
            }

            vertex[0].m_x = minx;
            vertex[0].m_y = miny;
            vertex[0].m_z = zz;
            vertex[0].m_rgba = 0xffffffff;
            vertex[0].m_u = minu;
            vertex[0].m_v = minv;

            vertex[1].m_x = maxx;
            vertex[1].m_y = miny;
            vertex[1].m_z = zz;
            vertex[1].m_rgba = 0xffffffff;
            vertex[1].m_u = maxu;
            vertex[1].m_v = minv;

            vertex[2].m_x = maxx;
            vertex[2].m_y = maxy;
            vertex[2].m_z = zz;
            vertex[2].m_rgba = 0xffffffff;
            vertex[2].m_u = maxu;
            vertex[2].m_v = maxv;

            bgfx::setVertexBuffer(0, &vb);
        }
    }

    struct LightProbe {
        void load(const char *_name) {
            char filePath[512];

            bx::snprintf(filePath, BX_COUNTOF(filePath), R"(../resource/env_maps/%s_lod.dds)", _name);
            m_tex = loadTexture(filePath, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);

            bx::snprintf(filePath, BX_COUNTOF(filePath), R"(../resource/env_maps/%s_irr.dds)", _name);
            m_texIrr = loadTexture(filePath, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
        }

        void destroy() {
            bgfx::destroy(m_tex);
            bgfx::destroy(m_texIrr);
        }

        bgfx::TextureHandle m_tex;
        bgfx::TextureHandle m_texIrr;
    };

    class EStarHomework : public entry::AppI {
    public:
        EStarHomework(const char *_name, const char *_description, const char *_url)
                : entry::AppI(_name, _description, _url),
                  m_pt(0),
                  m_r(true),
                  m_g(true),
                  m_b(true),
                  m_a(true) {
        }

        void init(int32_t _argc, const char *const *_argv, uint32_t _width, uint32_t _height) override {
            Args args(_argc, _argv);

            m_width = _width;
            m_height = _height;
            m_debug = BGFX_DEBUG_TEXT;
            m_reset = BGFX_RESET_VSYNC;
            m_timeOffset = bx::getHPCounter();

            bgfx::Init init;
            // init.type = args.m_type;
            init.type = bgfx::RendererType::OpenGL;
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

            // vertex declarations
            PosColorTexCoord0Vertex::init();

            // init a cube light
            Cubes::init_cube(m_lightVbh, m_lightIbh);
            m_lightProgram = loadProgram("light_vs", "light_fs");
            u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);

            // create mesh program from shaders
            m_mesh = meshLoad(R"(../resource/pbr_stone/pbr_stone_mes.bin)");
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
            cameraSetPosition(bx::Vec3(0.0f, 0.0f, -10.0f));
            u_viewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);

            m_lightProbe.load("bolonga");
            m_skyProgram = loadProgram("sky_vs", "sky_fs");
            s_texCube = bgfx::createUniform("s_texCube", bgfx::UniformType::Sampler);
            s_texCubeIrr = bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler);


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

            m_lightProbe.destroy();
            bgfx::destroy(m_skyProgram);
            bgfx::destroy(s_texCube);
            bgfx::destroy(s_texCubeIrr);

            // Shutdown bgfx.
            bgfx::shutdown();

            cameraDestroy();

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

                ImGui::SetNextWindowPos(ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f),
                                        ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(m_width / 5.0f, m_height / 3.5f),
                                         ImGuiCond_FirstUseEver);
                ImGui::Begin("Settings", NULL, 0);

//                ImGui::Checkbox("Write R", &m_r);
//                ImGui::Checkbox("Write G", &m_g);
//                ImGui::Checkbox("Write B", &m_b);
//                ImGui::Checkbox("Write A", &m_a);

                ImGui::SliderFloat3("Light Pos", m_lightPos, -10, 10);

                /* ImGui File Dialog from https://github.com/gallickgunner/ImGui-Addons
                 * Under MIT license
                 * */
                if (ImGui::Button("Open Mesh"))
                    ImGui::OpenPopup("Open Mesh");
                if (file_dialog.showFileDialog("Open Mesh", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN,
                                               ImVec2(700, 310), ".bin")) {
                    std::cout << file_dialog.selected_path << std::endl;    // The absolute path to the selected file
                    m_mesh = meshLoad(file_dialog.selected_path.c_str());
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


                const bgfx::Caps *caps = bgfx::getCaps();
                // view transform 0 for sky box
                float view_matrix[16];
                bx::mtxIdentity(view_matrix);
                float proj_matrix[16];
                bx::mtxOrtho(proj_matrix, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0, caps->homogeneousDepth);
                bgfx::setViewTransform(0, view_matrix, proj_matrix);


                // view transform 1 for mesh
                cameraGetViewMtx(view_matrix);
                bx::mtxProj(proj_matrix, cameraGetFoV(), float(m_width) / float(m_height),
                            0.1f, 100.0f, caps->homogeneousDepth);
                bgfx::setViewTransform(1, view_matrix, proj_matrix);


                // Env mtx.
//                float mtxEnvView[16];
//                m_camera.envViewMtx(mtxEnvView);
//                float mtxEnvRot[16];
//                bx::mtxRotateY(mtxEnvRot, m_settings.m_envRotCurr);
//                bx::mtxMul(m_uniforms.m_mtx, mtxEnvView, mtxEnvRot); // Used for Skybox.


                // set view 0 rectangle
                bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
                // set view 0 rectangle
                bgfx::setViewRect(1, 0, 0, uint16_t(m_width), uint16_t(m_height));

                auto cam_pos = cameraGetPosition();
                m_viewPos[0] = cam_pos.x;
                m_viewPos[1] = cam_pos.y;
                m_viewPos[2] = cam_pos.z;
                bgfx::setUniform(u_viewPos, m_viewPos);


                // render sky box
                {
                    bgfx::setTexture(0, s_texCube, m_lightProbe.m_tex);
                    bgfx::setTexture(1, s_texCubeIrr, m_lightProbe.m_texIrr);
                    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
                    screenSpaceQuad((float) m_width, (float) m_height, true);
                    bgfx::submit(0, m_skyProgram);
                }

                // render light
                { // Current primitive topology
                    uint64_t state = 0
                                     | (m_r ? BGFX_STATE_WRITE_R : 0)
                                     | (m_g ? BGFX_STATE_WRITE_G : 0)
                                     | (m_b ? BGFX_STATE_WRITE_B : 0)
                                     | (m_a ? BGFX_STATE_WRITE_A : 0)
                                     | BGFX_STATE_WRITE_Z
                                     | BGFX_STATE_DEPTH_TEST_LESS
                                     | BGFX_STATE_CULL_CW
                                     | BGFX_STATE_MSAA
                                     | BGFX_STATE_PT_TRISTRIP;
                    float mtx[16];
//                    m_lightPos[0] = 10.0f * cos(time);
//                    m_lightPos[2] = 10.0f * sin(time);

                    bx::mtxTranslate(mtx, m_lightPos[0], m_lightPos[1], m_lightPos[2]);
                    bgfx::setUniform(u_lightPos, &m_lightPos);
                    // Set model matrix for rendering.
                    bgfx::setTransform(mtx);
                    // Set vertex and index buffer.
                    bgfx::setVertexBuffer(0, m_lightVbh);
                    bgfx::setIndexBuffer(m_lightIbh);
                    // Set render states.
                    bgfx::setState(state);
                    // Submit primitive for rendering to view 1.
                    bgfx::submit(1, m_lightProgram);
                }

                // render mesh
                {
                    float model_matrix[16];
                    // bx::mtxRotateXY(model_matrix, 0.0f, time * 0.37f);
                    bx::mtxTranslate(model_matrix, 0.0, 0.0, 0.0);
                    // set texture
                    bgfx::setTexture(0, s_texDiffuse, m_texDiffuse);
                    bgfx::setTexture(1, s_texNormal, m_texNormal);
                    bgfx::setTexture(2, s_texAORM, m_texAORM);
                    // draw mesh
                    meshSubmit(m_mesh, 1, m_meshProgram, model_matrix);
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

        bgfx::UniformHandle u_viewPos;
        float m_viewPos[4]{0.0, 0.0, -10.0, 1.0};

        bgfx::VertexBufferHandle m_lightVbh;
        bgfx::IndexBufferHandle m_lightIbh;
        bgfx::ProgramHandle m_lightProgram;
        float m_lightPos[4]{-5.0, 10.0, -10.0, 1.0};
        bgfx::UniformHandle u_lightPos;

        Mesh *m_mesh;
        bgfx::ProgramHandle m_meshProgram;
        bgfx::TextureHandle m_texDiffuse;
        bgfx::UniformHandle s_texDiffuse;
        bgfx::TextureHandle m_texNormal;
        bgfx::UniformHandle s_texNormal;
        bgfx::TextureHandle m_texAORM;
        bgfx::UniformHandle s_texAORM;

        LightProbe m_lightProbe;
        bgfx::ProgramHandle m_skyProgram;
        bgfx::UniformHandle s_texCube;
        bgfx::UniformHandle s_texCubeIrr;

        bgfx::UniformHandle u_time;
        int64_t m_timeOffset;
        int32_t m_pt;

        bool m_r;
        bool m_g;
        bool m_b;
        bool m_a;

        int32_t num_boxes;

        imgui_addons::ImGuiFileBrowser file_dialog;
    };

} // namespace

int _main_(int _argc, char **_argv) {
    RenderCore::EStarHomework app("e-star-homework", "", "");
    return entry::runApp(&app, _argc, _argv);
}

