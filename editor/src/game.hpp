#pragma once

#include <zabato/game.hpp>

namespace zabato
{
class editor : public zabato::game
{
public:
    editor();
    editor(const editor &)            = default;
    editor(editor &&)                 = delete;
    editor &operator=(const editor &) = default;
    editor &operator=(editor &&)      = delete;
    ~editor();
};
} // namespace zabato