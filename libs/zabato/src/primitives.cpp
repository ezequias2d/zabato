#include <zabato/math.hpp>
#include <zabato/primitives.hpp>

namespace zabato
{

namespace primitives
{

shared_ptr<mesh> create_cube()
{
    mesh *m = new mesh();
    m->init(mesh_flags::normal | mesh_flags::color | mesh_flags::tex,
            primitive_type::triangles);

    m->set_vertex_count(24);

    // Positions
    vec3<real> p[8] = {
        {-1, -1, 1},
        {1, -1, 1},
        {1, 1, 1},
        {-1, 1, 1}, // Front
        {-1, -1, -1},
        {1, -1, -1},
        {1, 1, -1},
        {-1, 1, -1} // Back
    };

    // Normals
    vec3<real> n[6] = {
        {0, 0, 1},  // Front
        {0, 0, -1}, // Back
        {-1, 0, 0}, // Left
        {1, 0, 0},  // Right
        {0, 1, 0},  // Top
        {0, -1, 0}  // Bottom
    };

    // Faces (Quad to Triangle indices handled manually)
    // 0: Front    (0, 1, 2, 3) -> Normal 0
    // 1: Back     (5, 4, 7, 6) -> Normal 1
    // 2: Left     (4, 0, 3, 7) -> Normal 2
    // 3: Right    (1, 5, 6, 2) -> Normal 3
    // 4: Top      (3, 2, 6, 7) -> Normal 4
    // 5: Bottom   (4, 5, 1, 0) -> Normal 5

    struct Face
    {
        int v[4];
        int normal_idx;
    };
    Face faces[6] = {{{0, 1, 2, 3}, 0},
                     {{5, 4, 7, 6}, 1},
                     {{4, 0, 3, 7}, 2},
                     {{1, 5, 6, 2}, 3},
                     {{3, 2, 6, 7}, 4},
                     {{4, 5, 1, 0}, 5}};

    m->set_primitive_count(12);

    int v_idx = 0;
    int t_idx = 0;

    for (int i = 0; i < 6; ++i)
    {
        vec3<real> norm = n[faces[i].normal_idx];

        for (int j = 0; j < 4; ++j)
        {
            int pos_idx = faces[i].v[j];
            m->set_position(v_idx, p[pos_idx]);
            m->set_normal(v_idx, norm);
            m->set_color(v_idx, color(1, 1, 1, 1));

            vec2<real> uv;
            if (j == 0)
                uv = {0, 0};
            else if (j == 1)
                uv = {1, 0};
            else if (j == 2)
                uv = {1, 1};
            else
                uv = {0, 1};
            m->set_texcoord(v_idx, uv);

            v_idx++;
        }

        // 2 Triangles per face (0, 1, 2) and (0, 2, 3) relative to face start
        int base = i * 4;
        m->set_primitive(
            t_idx++,
            triangle_primitive{
                {(uint16_t)base, (uint16_t)(base + 1), (uint16_t)(base + 2)}});
        m->set_primitive(
            t_idx++,
            triangle_primitive{
                {(uint16_t)base, (uint16_t)(base + 2), (uint16_t)(base + 3)}});
    }

    return shared_ptr<mesh>(m);
}

shared_ptr<mesh> create_sphere(real radius, int rings, int sectors)
{
    mesh *m = new mesh();
    m->init(mesh_flags::normal | mesh_flags::color | mesh_flags::tex,
            primitive_type::triangles);

    real R = real(1) / (real)(rings - 1);
    real S = real(1) / (real)(sectors - 1);

    m->set_vertex_count(rings * sectors);
    m->set_primitive_count((rings - 1) * (sectors - 1) * 2);

    int v_idx = 0;
    for (int r = 0; r < rings; r++)
    {
        for (int s = 0; s < sectors; s++)
        {
            const real PI  = real::pi();
            const real PI2 = real(2) * PI;
            const real PIH = real::pi() * real(0.5);
            real y         = sin(-PIH + PI * r * R);
            real x         = cos(PI2 * s * S) * sin(PI * r * R);
            real z         = sin(PI2 * s * S) * sin(PI * r * R);

            vec3<real> pos(x * radius, y * radius, z * radius);
            vec3<real> norm(x, y, z);

            m->set_position(v_idx, pos);
            m->set_normal(v_idx, norm);
            m->set_texcoord(v_idx, {real(s) * S, real(r) * R});
            m->set_color(v_idx, color::white());
            v_idx++;
        }
    }

    int t_idx = 0;
    for (int r = 0; r < rings - 1; r++)
    {
        for (int s = 0; s < sectors - 1; s++)
        {
            int cur  = r * sectors + s;
            int next = (r + 1) * sectors + s;

            m->set_primitive(
                t_idx++,
                triangle_primitive{
                    {(uint16_t)cur, (uint16_t)next, (uint16_t)(cur + 1)}});
            m->set_primitive(t_idx++,
                             triangle_primitive{{(uint16_t)(cur + 1),
                                                 (uint16_t)next,
                                                 (uint16_t)(next + 1)}});
        }
    }

    return shared_ptr<mesh>(m);
}

shared_ptr<mesh> create_plane(real size, int divisions)
{
    // Simple quad plane centered at 0, facing Y up
    mesh *m = new mesh();
    m->init(mesh_flags::normal | mesh_flags::color | mesh_flags::tex,
            primitive_type::triangles);

    // Minimal implementation: 1 quad (4 verts) if divisions=1
    // For now simple 4 vert plane
    m->set_vertex_count(4);
    m->set_primitive_count(2);

    real h = size * real(0.5);

    m->set_position(0, {-h, 0, h});
    m->set_position(1, {h, 0, h});
    m->set_position(2, {h, 0, -h});
    m->set_position(3, {-h, 0, -h});

    vec3<real> norm(0, 1, 0);
    for (int i = 0; i < 4; ++i)
        m->set_normal(i, norm);
    for (int i = 0; i < 4; ++i)
        m->set_color(i, color(1, 1, 1, 1));

    m->set_texcoord(0, {0, 0});
    m->set_texcoord(1, {1, 0});
    m->set_texcoord(2, {1, 1});
    m->set_texcoord(3, {0, 1});

    m->set_primitive(0, triangle_primitive{{0, 1, 2}});
    m->set_primitive(1, triangle_primitive{{0, 2, 3}});

    return shared_ptr<mesh>(m);
}

} // namespace primitives

} // namespace zabato
