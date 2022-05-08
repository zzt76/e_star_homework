//
// Created by 37660 on 2022/4/14.
//

#include "common.h"
#include "bgfx_utils.h"

#ifndef ESTARHOMEWORK_MESHPRODUCER_H
#define ESTARHOMEWORK_MESHPRODUCER_H

namespace RenderCore::Triangle {
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
                    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
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

    static void create_cubes_vertex_buffer(bgfx::VertexBufferHandle &m_vbh) {
        // Create static vertex buffer
        m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)),
                                         PosColorVertex::ms_layout);
    }

    static void create_cubes_index_buffer_array(bgfx::IndexBufferHandle *m_ibhs) {
        // 4 different index buffer for rendering
        // Create static index buffer for triangle list rendering
        m_ibhs[0] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

        // Create static index buffer for triangle strip rendering
        m_ibhs[1] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriStrip, sizeof(s_cubeTriStrip)));

        // Create static index buffer for line list rendering
        m_ibhs[2] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeLineList, sizeof(s_cubeLineList)));

        // Create static index buffer for line strip rendering
        m_ibhs[3] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeLineStrip, sizeof(s_cubeLineStrip)));

        // Create static index buffer for point list rendering, only render points
        m_ibhs[4] = bgfx::createIndexBuffer(bgfx::makeRef(s_cubePoints, sizeof(s_cubePoints)));
    }

    static void create_cubes_index_buffer(bgfx::IndexBufferHandle &m_ibh) {
        // Create static index buffer for triangle strip rendering
        m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriStrip, sizeof(s_cubeTriStrip)));
    }

    static void init_cube(bgfx::VertexBufferHandle &m_vbh, bgfx::IndexBufferHandle &m_ibh) {
        PosColorVertex::init();
        create_cubes_vertex_buffer(m_vbh);
        create_cubes_index_buffer(m_ibh);
    }

    // platform vertices and indices
    struct PosNormalVertex {
        float m_x;
        float m_y;
        float m_z;
        uint32_t m_normal;

        static void init() {
            ms_layout
                    .begin()
                    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true)
                    .end();
        };

        static bgfx::VertexLayout ms_layout;
    };

    bgfx::VertexLayout PosNormalVertex::ms_layout;

    static PosNormalVertex s_hplaneVertices[] =
            {
                    {-1.0f, 0.0f, 1.0f,  encodeNormalRgba8(0.0f, 1.0f, 0.0f)},
                    {1.0f,  0.0f, 1.0f,  encodeNormalRgba8(0.0f, 1.0f, 0.0f)},
                    {-1.0f, 0.0f, -1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f)},
                    {1.0f,  0.0f, -1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f)},
            };

    static const uint16_t s_planeIndices[] =
            {
                    0, 1, 2,
                    1, 3, 2,
            };

    static void create_plane_vertex_buffer(bgfx::VertexBufferHandle &m_vbh) {
        // Create static index buffer for triangle strip rendering
        m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(s_hplaneVertices, sizeof(s_hplaneVertices)),
                                         PosNormalVertex::ms_layout);
    }

    static void create_plane_index_buffer(bgfx::IndexBufferHandle &m_ibh) {
        // Create static index buffer for triangle strip rendering
        m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_planeIndices, sizeof(s_planeIndices)));
    }

    static void init_plane(bgfx::VertexBufferHandle &m_vbh, bgfx::IndexBufferHandle &m_ibh) {
        PosNormalVertex::init();
        create_plane_vertex_buffer(m_vbh);
        create_plane_index_buffer(m_ibh);
    }

}


#endif //ESTARHOMEWORK_MESHPRODUCER_H
