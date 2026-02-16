#pragma once

#include <zabato/base_object.hpp>
#include <zabato/delegate.hpp>
#include <zabato/object.hpp>

namespace zabato::editor
{

class property_grid
{
public:
    static void
    render_object(base_object *obj,
                  const rtti &type,
                  class asset_database *db                 = nullptr,
                  delegate<void(const string &)> on_locate = nullptr);
};

} // namespace zabato::editor
