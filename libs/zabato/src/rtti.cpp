#include <zabato/reflection.hpp>
#include <zabato/rtti.hpp>

namespace zabato
{

rtti::~rtti()
{
    if (m_reflection)
        delete m_reflection;
}

reflection &rtti::ensure_reflection() const
{
    if (!m_reflection)
    {
        m_reflection = new reflection();
        if (m_setup)
            m_setup(*m_reflection);
    }
    return *m_reflection;
}

} // namespace zabato
