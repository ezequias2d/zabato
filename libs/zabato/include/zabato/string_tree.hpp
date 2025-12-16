#pragma once

#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

class string_tree
{
public:
    string_tree();
    string_tree(const string &text);
    ~string_tree();

    void set_text(const string &text);
    const string &get_text() const;

    void add_child(string_tree *child);
    int get_child_count() const;
    string_tree *get_child(int index);
    const string_tree *get_child(int index) const;

private:
    string m_text;
    vector<string_tree *> m_children;
};

} // namespace zabato
