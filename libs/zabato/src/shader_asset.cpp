#include <zabato/gpu.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/shader_asset.hpp>

namespace zabato
{

const rtti shader_asset::TYPE("zabato.shader_asset", &resource::TYPE, nullptr);

shader_asset::shader_asset()  = default;
shader_asset::~shader_asset() = default;

void shader_asset::set_source(const string_view &source)
{
    m_source = source;
    // Invalidate program?
    if (m_program)
    {
        m_program->destroy();
        m_program = nullptr;
    }
}

bool shader_asset::is_compiled() const { return m_program != nullptr; }

class program *shader_asset::get_program() const { return m_program.get(); }

bool shader_asset::compile(script_system &ss,
                           gpu *g,
                           const string_view &backend_arg)
{
    if (m_source.empty())
        return false;

    string_view backend = backend_arg;

    char temp_name[32];
    snprintf(temp_name, sizeof(temp_name), "shader@%p", (void *)this);

    if (!ss.compile_zshader(m_source, temp_name, m_result, backend))
    {
        return false;
    }

    if (g)
    {
        auto vs = g->create_shader(shader_type::vertex, m_result.glsl_vertex);
        if (!vs)
            return false;
        auto fs =
            g->create_shader(shader_type::fragment, m_result.glsl_fragment);
        if (!fs)
        {
            vs->destroy();
            return false;
        }

        m_program = shared_ptr<program>(g->create_program(vs, fs));
        if (!m_program)
        {
            vs->destroy();
            fs->destroy();
            return false;
        }

        if (!m_program->link())
        {
            m_program = nullptr;
            vs->destroy();
            fs->destroy();
            return false;
        }

        vs->destroy();
        fs->destroy();
    }

    return true;
}

} // namespace zabato
