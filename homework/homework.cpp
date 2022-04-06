/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"

namespace {

    struct PosColorVertex {
        // Vertex Position
        float m_x;
        float m_y;
        float m_z;
        // Vertex Color
        uint32_t m_abgr;

        static void init() {
            ms_layout
                    .begin()
                    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
                    .end();
        }

        static bgfx::VertexLayout ms_layout;
    };

    bgfx::VertexLayout PosColorVertex::ms_layout;

    static PosColorVertex s_cubeVertices[] =
            {
                    {-1.0f, 1.0f,  1.0f,  0xff000000},
                    {1.0f,  1.0f,  1.0f,  0xff0000ff},
                    {-1.0f, -1.0f, 1.0f,  0xff00ff00},
                    {1.0f,  -1.0f, 1.0f,  0xff00ffff},
                    {-1.0f, 1.0f,  -1.0f, 0xffff0000},
                    {1.0f,  1.0f,  -1.0f, 0xffff00ff},
                    {-1.0f, -1.0f, -1.0f, 0xffffff00},
                    {1.0f,  -1.0f, -1.0f, 0xffffffff},
            };

    static const uint16_t s_cubeTriList[] =
            {
                    0, 1, 2, // 0
                    1, 3, 2,
                    4, 6, 5, // 2
                    5, 6, 7,
                    0, 2, 4, // 4
                    4, 2, 6,
                    1, 5, 3, // 6
                    5, 7, 3,
                    0, 4, 1, // 8
                    4, 5, 1,
                    2, 3, 6, // 10
                    6, 3, 7,
            };

    static const uint16_t s_cubeTriStrip[] =
            {
                    0, 1, 2,
                    3,
                    7,
                    1,
                    5,
                    0,
                    4,
                    2,
                    6,
                    7,
                    4,
                    5,
            };

    static const uint16_t s_cubeLineList[] =
            {
                    0, 1,
                    0, 2,
                    0, 4,
                    1, 3,
                    1, 5,
                    2, 3,
                    2, 6,
                    3, 7,
                    4, 5,
                    4, 6,
                    5, 7,
                    6, 7,
            };

    static const uint16_t s_cubeLineStrip[] =
            {
                    0, 2, 3, 1, 5, 7, 6, 4,
                    0, 2, 6, 4, 5, 7, 3, 1,
                    0,
            };

    static const uint16_t s_cubePoints[] =
            {
                    0, 1, 2, 3, 4, 5, 6, 7
            };

    static const char *s_ptNames[]
            {
                    "Triangle List",
                    "Triangle Strip",
                    "Lines",
                    "Line Strip",
                    "Points",
            };

    static const uint64_t s_ptState[]
            {
                    UINT64_C(0),
                    BGFX_STATE_PT_TRISTRIP,
                    BGFX_STATE_PT_LINES,
                    BGFX_STATE_PT_LINESTRIP,
                    BGFX_STATE_PT_POINTS,
            };
    BX_STATIC_ASSERT(BX_COUNTOF(s_ptState) == BX_COUNTOF(s_ptNames));

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
            init.type = args.m_type;
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
            PosColorVertex::init();

            // Create static vertex buffer
            m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)),
                                             PosColorVertex::ms_layout);

            // 4 different index buffer for rendering
            // Create static index buffer for triangle list rendering
            m_ibh[0] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

            // Create static index buffer for triangle strip rendering
            m_ibh[1] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriStrip, sizeof(s_cubeTriStrip)));

            // Create static index buffer for line list rendering
            m_ibh[2] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeLineList, sizeof(s_cubeLineList)));

            // Create static index buffer for line strip rendering
            m_ibh[3] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeLineStrip, sizeof(s_cubeLineStrip)));

            // Create static index buffer for point list rendering, only render points
            m_ibh[4] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubePoints, sizeof(s_cubePoints)));
            

            // Create program from shaders
            m_program = loadProgram("vs_cubes", "fs_cubes");

            imguiCreate();
        }

        virtual int shutdown() override {
            imguiDestroy();

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

                imguiEndFrame();

                // Set view 0 default viewport.
                bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

                // This dummy draw call is here to make sure that view 0 is cleared
                // if no other draw calls are submitted to view 0.
                bgfx::touch(0);

                // Use debug font to print information about this example.
                bgfx::dbgTextClear();
                bgfx::dbgTextImage(
                        bx::max<uint16_t>(uint16_t(m_width / 2 / 8), 20) - 20,
                        bx::max<uint16_t>(uint16_t(m_height / 2 / 16), 6) - 6, 40, 12, s_logo, 160
                );

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
        bgfx::IndexBufferHandle m_ibh[BX_COUNTOF(s_ptState)];
        bgfx::ProgramHandle m_program;
        int32_t m_pt;

        bool m_r;
        bool m_g;
        bool m_b;
        bool m_a;
    };

} // namespace

int _main_(int _argc, char **_argv) {
    EStarHomework app("e-star-homework", "", "");
    return entry::runApp(&app, _argc, _argv);
}

