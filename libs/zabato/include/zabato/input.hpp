#pragma once
#include <stdint.h>
#include <zabato/math.hpp>
#include <zabato/real.hpp>

namespace zabato
{
enum class button_state
{
    release,
    press,
    repeat
};
enum class connect_event
{
    disconnected,
    connected
};
enum class mouse_button
{
    none,
    left,
    middle,
    right
};

enum class lock_state : uint32_t
{
    caps_lock = (1 << 0),
    num_lock  = (1 << 1),
};

enum class gamepad_button
{
    unknown,
    ps_x,
    ps_o,
    ps_t,
    ps_s,
    start,
    select,
    home,
    dpad_up,
    dpad_down,
    dpad_left,
    dpad_right,
    l1,
    l2,
    l3,
    r1,
    r2,
    r3,
    max,
    xbox_a = ps_x,
    xbox_b = ps_o,
    xbox_y = ps_t,
    xbox_x = ps_s,
};

enum class gamepad_analog
{
    left_x,
    left_y,
    right_x,
    right_y,
    l2,
    r2,
    max
};

enum class key_code
{
    unknown,

    /* Printable keys */
    space,
    apostrophe, /* ' */
    comma,      /* , */
    minus,      /* - */
    period,     /* . */
    slash,      /* / */
    num_0,
    num_1,
    num_2,
    num_3,
    num_4,
    num_5,
    num_6,
    num_7,
    num_8,
    num_9,
    semicolon, /* ; */
    equal,     /* = */
    a,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,
    left_bracket,  /* [ */
    backslash,     /* \ */
    right_bracket, /* ] */
    grave_accent,  /* ` */
    world_1,       /* non-US #1 */
    world_2,       /* non-US #2 */

    /* Function keys */
    escape,
    enter,
    tab,
    backspace,
    insert,
    del,
    right,
    left,
    down,
    up,
    page_up,
    page_down,
    home,
    end,
    caps_lock,
    scroll_lock,
    num_lock,
    print_screen,
    pause,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    f13,
    f14,
    f15,
    f16,
    f17,
    f18,
    f19,
    f20,
    f21,
    f22,
    f23,
    f24,
    kp_0,
    kp_1,
    kp_2,
    kp_3,
    kp_4,
    kp_5,
    kp_6,
    kp_7,
    kp_8,
    kp_9,
    kp_decimal,
    kp_divide,
    kp_multiply,
    kp_subtract,
    kp_add,
    kp_enter,
    kp_equal,
    left_shift,
    left_control,
    left_alt,
    left_super,
    right_shift,
    right_control,
    right_alt,
    right_super,
    menu,

    last = menu
};

enum class modifier_keys : uint32_t
{
    none      = 0,
    shift     = (1 << 0),
    control   = (1 << 1),
    alt       = (1 << 2),
    super     = (1 << 3),
    caps_lock = (1 << 4),
    num_lock  = (1 << 5),
};

inline modifier_keys operator|(modifier_keys a, modifier_keys b)
{
    return static_cast<modifier_keys>(static_cast<uint32_t>(a) |
                                      static_cast<uint32_t>(b));
}
inline modifier_keys operator&(modifier_keys a, modifier_keys b)
{
    return static_cast<modifier_keys>(static_cast<uint32_t>(a) &
                                      static_cast<uint32_t>(b));
}

enum class mouse_icon
{
    arrow,
    ibeam,
    crosshair,
    pointing_hand,
    resize_left_right,
    resize_up_down,
    resize_all,
    resize_left_up_right_down,
    resize_left_down_right_up,
    not_allowed,
};

struct gamepad_state
{
    button_state buttons[static_cast<int>(gamepad_button::max)];
    real analogs[static_cast<int>(gamepad_analog::max)];
};

const char *to_string(key_code key);
const char *to_string(mouse_button button);
const char *to_string(mouse_icon icon);
const char *to_string(button_state state);
const char *to_string(connect_event event);
const char *to_string(modifier_keys mods);
const char *to_string(gamepad_button button);
const char *to_string(gamepad_analog analog);
const char *to_string(gamepad_state state);
} // namespace zabato