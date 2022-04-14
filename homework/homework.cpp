/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include "meshproducer.h"

namespace RenderCore {

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
            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0
            );

            // Create vertex stream declaration
            Cubes::PosColorVertex::init();
            Cubes::init(m_vbh, m_ibh);

            m_mesh = meshLoad("meshes/bunny.bin");

            // Create program from shaders
            m_program = loadProgram("vs_cubes", "fs_cubes");

            m_timeOffset = bx::getHPCounter();

            num_boxes = 10;

            imguiCreate();
        }

        virtual int shutdown() override {
            imguiDestroy();

            for (uint32_t ii = 0; ii < BX_COUNTOF(m_ibh); ++ii) {
                bgfx::destroy(m_ibh[ii]);
            }
            meshUnload(m_mesh);

            bgfx::destroy(m_vbh);
            bgfx::destroy(m_program);

            // Shutdown bgfx.
            bgfx::shutdown();

            return 0;
        }

        bool update() override {
            if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) {
                imguiBeginFrame(m_mouseState.m_mx, m_mouseState.m_my,
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

                ImGui::Checkbox("Write R", &m_r);
                ImGui::Checkbox("Write G", &m_g);
                ImGui::Checkbox("Write B", &m_b);
                ImGui::Checkbox("Write A", &m_a);

                ImGui::SliderInt("Numbers", &num_boxes, 1, 100);

                ImGui::Text("Primitive topology:");
                ImGui::Combo("##topology", (int *) &m_pt, Cubes::s_ptNames, BX_COUNTOF(Cubes::s_ptNames));

                ImGui::End();

                imguiEndFrame();

                float time = (float) ((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()));

                constexpr bx::Vec3 look_at{0.0f, 0.0f, 0.0f};
                constexpr bx::Vec3 camera{0.0f, 0.0f, -35.0f};

                {
                    float view_matrix[16];
                    bx::mtxLookAt(view_matrix, camera, look_at);

                    float proj_matrix[16];
                    bx::mtxProj(proj_matrix, 60.0f, float(m_width) / float(m_height),
                                0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
                    bgfx::setViewTransform(0, view_matrix, proj_matrix);

                    // Set view 0 default viewport.
                    bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
                }

                // This dummy draw call is here to make sure that view 0 is cleared
                // if no other draw calls are submitted to view 0.
                bgfx::touch(0);

                // Current primitive topology
                bgfx::IndexBufferHandle current_ibh = m_ibh[m_pt];
                uint64_t state = 0
                                 | (m_r ? BGFX_STATE_WRITE_R : 0)
                                 | (m_g ? BGFX_STATE_WRITE_G : 0)
                                 | (m_b ? BGFX_STATE_WRITE_B : 0)
                                 | (m_a ? BGFX_STATE_WRITE_A : 0)
                                 | BGFX_STATE_WRITE_Z
                                 | BGFX_STATE_DEPTH_TEST_LESS
                                 | BGFX_STATE_CULL_CW
                                 | BGFX_STATE_MSAA
                                 | Cubes::s_ptState[m_pt];

                // Submit 11x11 cubes
                for (int yy = 0; yy < num_boxes; ++yy) {
                    for (int xx = 0; xx < num_boxes; ++xx) {
                        float model_matrix[16];
                        // Rotate the boxes according to time
                        bx::mtxRotateXY(model_matrix, time + xx * 0.21f, time + yy * 0.37f);
                        // Translate the boxes
                        model_matrix[12] = -15.0f + float(xx) * 3.0f;
                        model_matrix[13] = -15.0f + float(yy) * 3.0f;
                        model_matrix[14] = 0.0f;

                        // Set the model matrix
                        bgfx::setTransform(model_matrix);

                        // Set vertex and index buffer
                        bgfx::setVertexBuffer(0, m_vbh);
                        bgfx::setIndexBuffer(current_ibh);

                        // Set render state
                        bgfx::setState(state);

                        // Submit primitive for rendering to view 0
                        // Program created from shaders
                        bgfx::submit(0, m_program);
                    }
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

        bgfx::VertexBufferHandle m_vbh;
        bgfx::IndexBufferHandle m_ibh[BX_COUNTOF(Cubes::s_ptState)];
        Mesh *m_mesh;
        bgfx::ProgramHandle m_program;
        int64_t m_timeOffset;
        int32_t m_pt;

        bool m_r;
        bool m_g;
        bool m_b;
        bool m_a;

        int32_t num_boxes;
    };

} // namespace

int _main_(int _argc, char **_argv) {
    RenderCore::EStarHomework app("e-star-homework", "", "");
    return entry::runApp(&app, _argc, _argv);
}

