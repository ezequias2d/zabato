#pragma once

#include <zabato/string_tree.hpp>

namespace zabato
{

class scene_printer
{
public:
    scene_printer(const char *filename);
    ~scene_printer();

    void print(const string_tree *tree);

private:
    void print_recursive(const string_tree *tree, int indentation);
    void print_indentation(int indentation);

    const char *m_filename;
    FILE *m_file;
};

} // namespace zabato
