#include <zabato/string_tree.hpp>

namespace zabato
{

string_tree::string_tree() {}

string_tree::string_tree(const string &text) : m_text(text) {}

string_tree::~string_tree()
{
    for (size_t i = 0; i < m_children.size(); ++i)
    {
        delete m_children[i];
    }
    m_children.clear();
}

void string_tree::set_text(const string &text) { m_text = text; }

const string &string_tree::get_text() const { return m_text; }

void string_tree::add_child(string_tree *child)
{
    if (child)
    {
        m_children.push_back(child);
    }
}

int string_tree::get_child_count() const { return (int)m_children.size(); }

string_tree *string_tree::get_child(int index)
{
    if (index >= 0 && index < (int)m_children.size())
    {
        return m_children[index];
    }
    return nullptr;
}

const string_tree *string_tree::get_child(int index) const
{
    if (index >= 0 && index < (int)m_children.size())
    {
        return m_children[index];
    }
    return nullptr;
}

} // namespace zabato
