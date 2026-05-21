#include "test_framework.hpp"
#include <zabato/math.hpp>
#include <zabato/real.hpp>

using namespace zabato;

TEST(vec2_default_constructor)
{
    vec2<real> v;
    ASSERT_EQ(v.x, 0.0f);
    ASSERT_EQ(v.y, 0.0f);
}

TEST(vec2_value_constructor)
{
    vec2<real> v(5.0f);
    ASSERT_EQ(v.x, 5.0f);
    ASSERT_EQ(v.y, 5.0f);
}

TEST(vec2_component_constructor)
{
    vec2<real> v(1.0f, 2.0f);
    ASSERT_EQ(v.x, 1.0f);
    ASSERT_EQ(v.y, 2.0f);
}

TEST(vec2_addition)
{
    vec2<real> a(1.0f, 2.0f);
    vec2<real> b(3.0f, 4.0f);
    vec2<real> c = a + b;
    ASSERT_EQ(c.x, 4.0f);
    ASSERT_EQ(c.y, 6.0f);
}

TEST(vec2_subtraction)
{
    vec2<real> a(5.0f, 7.0f);
    vec2<real> b(2.0f, 3.0f);
    vec2<real> c = a - b;
    ASSERT_EQ(c.x, 3.0f);
    ASSERT_EQ(c.y, 4.0f);
}

TEST(vec2_scalar_multiply)
{
    vec2<real> v(2.0f, 3.0f);
    vec2<real> r = v * 4.0f;
    ASSERT_EQ(r.x, 8.0f);
    ASSERT_EQ(r.y, 12.0f);
}

TEST(vec2_swizzle)
{
    vec2<real> v(1.0f, 2.0f);
    vec2<real> yx = v.yx();
    ASSERT_EQ(yx.x, 2.0f);
    ASSERT_EQ(yx.y, 1.0f);
}

TEST(vec3_default_constructor)
{
    vec3<real> v;
    ASSERT_EQ(v.x, 0.0f);
    ASSERT_EQ(v.y, 0.0f);
    ASSERT_EQ(v.z, 0.0f);
}

TEST(vec3_component_constructor)
{
    vec3<real> v(1.0f, 2.0f, 3.0f);
    ASSERT_EQ(v.x, 1.0f);
    ASSERT_EQ(v.y, 2.0f);
    ASSERT_EQ(v.z, 3.0f);
}

TEST(vec3_addition)
{
    vec3<real> a(1.0f, 2.0f, 3.0f);
    vec3<real> b(4.0f, 5.0f, 6.0f);
    vec3<real> c = a + b;
    ASSERT_EQ(c.x, 5.0f);
    ASSERT_EQ(c.y, 7.0f);
    ASSERT_EQ(c.z, 9.0f);
}

TEST(vec3_dot_product)
{
    vec3<real> a(1.0f, 2.0f, 3.0f);
    vec3<real> b(4.0f, 5.0f, 6.0f);
    real dot = a.x * b.x + a.y * b.y + a.z * b.z;
    ASSERT_NEAR(dot, 32.0f, 0.001f);
}

TEST(vec3_cross_product)
{
    vec3<real> a(1.0f, 0.0f, 0.0f);
    vec3<real> b(0.0f, 1.0f, 0.0f);
    vec3<real> c = cross(a, b);
    ASSERT_NEAR(c.x, 0.0f, 0.001f);
    ASSERT_NEAR(c.y, 0.0f, 0.001f);
    ASSERT_NEAR(c.z, 1.0f, 0.001f);
}

TEST(vec3_length)
{
    vec3<real> v(3.0f, 4.0f, 0.0f);
    ASSERT_NEAR(length(v), 5.0f, 0.001f);
}

TEST(vec3_normalize)
{
    vec3<real> v(3.0f, 0.0f, 4.0f);
    vec3<real> n = normalize(v);
    ASSERT_NEAR(length(n), 1.0f, 0.001f);
}

TEST(mat4_identity)
{
    mat4<real> m = mat4<real>::identity();
    ASSERT_NEAR(m[0][0], 1.0f, 0.001f);
    ASSERT_NEAR(m[1][1], 1.0f, 0.001f);
    ASSERT_NEAR(m[2][2], 1.0f, 0.001f);
    ASSERT_NEAR(m[3][3], 1.0f, 0.001f);
    ASSERT_NEAR(m[0][1], 0.0f, 0.001f);
    ASSERT_NEAR(m[1][0], 0.0f, 0.001f);
}

TEST(mat4_translation)
{
    mat4<real> m = mat4_translation(vec3<real>(1.0f, 2.0f, 3.0f));
    ASSERT_NEAR(m.m03, 1.0f, 0.001f);
    ASSERT_NEAR(m.m13, 2.0f, 0.001f);
    ASSERT_NEAR(m.m23, 3.0f, 0.001f);
}

TEST(mat4_multiply_identity)
{
    mat4<real> m      = mat4<real>::identity();
    mat4<real> result = m * m;
    ASSERT_NEAR(result[0][0], 1.0f, 0.001f);
    ASSERT_NEAR(result[1][1], 1.0f, 0.001f);
    ASSERT_NEAR(result[2][2], 1.0f, 0.001f);
    ASSERT_NEAR(result[3][3], 1.0f, 0.001f);
}

TEST(mat4_transform_vector)
{
    mat4<real> m = mat4_translation(vec3<real>(5.0f, 0.0f, 0.0f));
    vec4<real> v(1.0f, 2.0f, 3.0f, 1.0);
    vec4<real> result = m * v;
    ASSERT_NEAR(result.x, 6.0f, 0.001f);
    ASSERT_NEAR(result.y, 2.0f, 0.001f);
    ASSERT_NEAR(result.z, 3.0f, 0.001f);
    ASSERT_NEAR(result.w, 1.0f, 0.001f);
}

RUN_TESTS()
