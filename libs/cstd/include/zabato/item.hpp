#pragma once
#include <zabato/list.hpp>

namespace zabato
{
class list_node;

class item
{
public:
private:
    list_node link;

    friend class list<item, &item::link>;

public:
    using list = class list<item, &item::link>;
};
} // namespace zabato