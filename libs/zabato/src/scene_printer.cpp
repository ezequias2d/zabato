#include <zabato/scene_printer.hpp>
#include <stdio.h>

namespace zabato
{

scene_printer::scene_printer(const char *filename)
    : m_filename(filename), m_file(nullptr)
{
}

scene_printer::~scene_printer() {}

void scene_printer::print(const string_tree *tree)
{
    if (!tree || !m_filename)
        return;

    m_file = fopen(m_filename, "wt");
    if (!m_file)
        return;

    print_recursive(tree, 0);

    fclose(m_file);
    m_file = nullptr;
}

void scene_printer::print_recursive(const string_tree *tree, int indentation)
{
    print_indentation(indentation);
    fprintf(m_file, "%s\n", tree->get_text().c_str());

    for (int i = 0; i < tree->get_child_count(); ++i)
    {
        print_recursive(tree->get_child(i), indentation + 4);
    }
}

void scene_printer::print_indentation(int indentation)
{
    for (int i = 0; i < indentation; ++i)
    {
        fputc(' ', m_file);
    }
}

} // namespace zabato
