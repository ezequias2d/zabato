#pragma once

#include <zabato/window.hpp>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <cassert>

namespace zabato
{

inline button_state to_button_state(Uint8 state, Uint8 repeat)
{
    if (state == SDL_PRESSED)
    {
        return (repeat > 0) ? button_state::repeat : button_state::press;
    }
    return button_state::release;
}

inline modifier_keys to_modifier_keys(SDL_Keymod mod)
{
    int m = 0;
    if (mod & KMOD_SHIFT)
        m |= static_cast<int>(modifier_keys::shift);
    if (mod & KMOD_CTRL)
        m |= static_cast<int>(modifier_keys::control);
    if (mod & KMOD_ALT)
        m |= static_cast<int>(modifier_keys::alt);
    if (mod & KMOD_GUI)
        m |= static_cast<int>(modifier_keys::super);
    if (mod & KMOD_CAPS)
        m |= static_cast<int>(modifier_keys::caps_lock);
    if (mod & KMOD_NUM)
        m |= static_cast<int>(modifier_keys::num_lock);
    return static_cast<modifier_keys>(m);
}

inline mouse_button to_mouse_button(Uint8 sdl_button)
{
    switch (sdl_button)
    {
    case SDL_BUTTON_LEFT:
        return mouse_button::left;
    case SDL_BUTTON_MIDDLE:
        return mouse_button::middle;
    case SDL_BUTTON_RIGHT:
        return mouse_button::right;
    default:
        return mouse_button::none;
    }
}

inline int to_sdl_mouse(mouse_button btn)
{
    switch (btn)
    {
    case mouse_button::left:
        return SDL_BUTTON_LEFT;
    case mouse_button::middle:
        return SDL_BUTTON_MIDDLE;
    case mouse_button::right:
        return SDL_BUTTON_RIGHT;
    default:
        assert(false && "should not happen");
        return -1; // Should not happen
    }
}

inline gamepad_button to_gamepad_button(int sdl_button)
{
    switch (sdl_button)
    {
    case SDL_CONTROLLER_BUTTON_A:
        return gamepad_button::ps_x;
    case SDL_CONTROLLER_BUTTON_B:
        return gamepad_button::ps_o;
    case SDL_CONTROLLER_BUTTON_Y:
        return gamepad_button::ps_t;
    case SDL_CONTROLLER_BUTTON_X:
        return gamepad_button::ps_s;
    case SDL_CONTROLLER_BUTTON_START:
        return gamepad_button::start;
    case SDL_CONTROLLER_BUTTON_BACK:
        return gamepad_button::select;
    case SDL_CONTROLLER_BUTTON_GUIDE:
        return gamepad_button::home;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        return gamepad_button::dpad_up;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        return gamepad_button::dpad_down;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        return gamepad_button::dpad_left;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        return gamepad_button::dpad_right;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        return gamepad_button::l1;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
        return gamepad_button::l3;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        return gamepad_button::r1;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        return gamepad_button::r3;
    default:
        return gamepad_button::unknown;
    }
}

inline SDL_GameControllerButton to_sdl_gamepad_button(gamepad_button btn)
{
    switch (btn)
    {
    case gamepad_button::ps_x:
        return SDL_CONTROLLER_BUTTON_A;
    case gamepad_button::ps_o:
        return SDL_CONTROLLER_BUTTON_B;
    case gamepad_button::ps_t:
        return SDL_CONTROLLER_BUTTON_Y;
    case gamepad_button::ps_s:
        return SDL_CONTROLLER_BUTTON_X;
    case gamepad_button::start:
        return SDL_CONTROLLER_BUTTON_START;
    case gamepad_button::select:
        return SDL_CONTROLLER_BUTTON_BACK;
    case gamepad_button::home:
        return SDL_CONTROLLER_BUTTON_GUIDE;
    case gamepad_button::dpad_up:
        return SDL_CONTROLLER_BUTTON_DPAD_UP;
    case gamepad_button::dpad_down:
        return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    case gamepad_button::dpad_left:
        return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    case gamepad_button::dpad_right:
        return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    case gamepad_button::l1:
        return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    case gamepad_button::l3:
        return SDL_CONTROLLER_BUTTON_LEFTSTICK;
    case gamepad_button::r1:
        return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    case gamepad_button::r3:
        return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    default:
        return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

inline SDL_GameControllerAxis to_sdl_gamepad_axis(gamepad_analog axis)
{
    switch (axis)
    {
    case gamepad_analog::left_x:
        return SDL_CONTROLLER_AXIS_LEFTX;
    case gamepad_analog::left_y:
        return SDL_CONTROLLER_AXIS_LEFTY;
    case gamepad_analog::right_x:
        return SDL_CONTROLLER_AXIS_RIGHTX;
    case gamepad_analog::right_y:
        return SDL_CONTROLLER_AXIS_RIGHTY;
    case gamepad_analog::l2:
        return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    case gamepad_analog::r2:
        return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    default:
        return SDL_CONTROLLER_AXIS_INVALID;
    }
}

inline key_code to_keycode(SDL_Scancode k)
{
    switch (k)
    {
    case SDL_SCANCODE_SPACE:
        return key_code::space;
    case SDL_SCANCODE_APOSTROPHE:
        return key_code::apostrophe;
    case SDL_SCANCODE_COMMA:
        return key_code::comma;
    case SDL_SCANCODE_MINUS:
        return key_code::minus;
    case SDL_SCANCODE_PERIOD:
        return key_code::period;
    case SDL_SCANCODE_SLASH:
        return key_code::slash;
    case SDL_SCANCODE_0:
        return key_code::num_0;
    case SDL_SCANCODE_1:
        return key_code::num_1;
    case SDL_SCANCODE_2:
        return key_code::num_2;
    case SDL_SCANCODE_3:
        return key_code::num_3;
    case SDL_SCANCODE_4:
        return key_code::num_4;
    case SDL_SCANCODE_5:
        return key_code::num_5;
    case SDL_SCANCODE_6:
        return key_code::num_6;
    case SDL_SCANCODE_7:
        return key_code::num_7;
    case SDL_SCANCODE_8:
        return key_code::num_8;
    case SDL_SCANCODE_9:
        return key_code::num_9;
    case SDL_SCANCODE_SEMICOLON:
        return key_code::semicolon;
    case SDL_SCANCODE_EQUALS:
        return key_code::equal;
    case SDL_SCANCODE_A:
        return key_code::a;
    case SDL_SCANCODE_B:
        return key_code::b;
    case SDL_SCANCODE_C:
        return key_code::c;
    case SDL_SCANCODE_D:
        return key_code::d;
    case SDL_SCANCODE_E:
        return key_code::e;
    case SDL_SCANCODE_F:
        return key_code::f;
    case SDL_SCANCODE_G:
        return key_code::g;
    case SDL_SCANCODE_H:
        return key_code::h;
    case SDL_SCANCODE_I:
        return key_code::i;
    case SDL_SCANCODE_J:
        return key_code::j;
    case SDL_SCANCODE_K:
        return key_code::k;
    case SDL_SCANCODE_L:
        return key_code::l;
    case SDL_SCANCODE_M:
        return key_code::m;
    case SDL_SCANCODE_N:
        return key_code::n;
    case SDL_SCANCODE_O:
        return key_code::o;
    case SDL_SCANCODE_P:
        return key_code::p;
    case SDL_SCANCODE_Q:
        return key_code::q;
    case SDL_SCANCODE_R:
        return key_code::r;
    case SDL_SCANCODE_S:
        return key_code::s;
    case SDL_SCANCODE_T:
        return key_code::t;
    case SDL_SCANCODE_U:
        return key_code::u;
    case SDL_SCANCODE_V:
        return key_code::v;
    case SDL_SCANCODE_W:
        return key_code::w;
    case SDL_SCANCODE_X:
        return key_code::x;
    case SDL_SCANCODE_Y:
        return key_code::y;
    case SDL_SCANCODE_Z:
        return key_code::z;
    case SDL_SCANCODE_LEFTBRACKET:
        return key_code::left_bracket;
    case SDL_SCANCODE_BACKSLASH:
        return key_code::backslash;
    case SDL_SCANCODE_RIGHTBRACKET:
        return key_code::right_bracket;
    case SDL_SCANCODE_GRAVE:
        return key_code::grave_accent;
    case SDL_SCANCODE_NONUSBACKSLASH:
        return key_code::world_1;
    case SDL_SCANCODE_NONUSHASH:
        return key_code::world_2;
    case SDL_SCANCODE_ESCAPE:
        return key_code::escape;
    case SDL_SCANCODE_RETURN:
        return key_code::enter;
    case SDL_SCANCODE_TAB:
        return key_code::tab;
    case SDL_SCANCODE_BACKSPACE:
        return key_code::backspace;
    case SDL_SCANCODE_INSERT:
        return key_code::insert;
    case SDL_SCANCODE_DELETE:
        return key_code::del;
    case SDL_SCANCODE_RIGHT:
        return key_code::right;
    case SDL_SCANCODE_LEFT:
        return key_code::left;
    case SDL_SCANCODE_DOWN:
        return key_code::down;
    case SDL_SCANCODE_UP:
        return key_code::up;
    case SDL_SCANCODE_PAGEUP:
        return key_code::page_up;
    case SDL_SCANCODE_PAGEDOWN:
        return key_code::page_down;
    case SDL_SCANCODE_HOME:
        return key_code::home;
    case SDL_SCANCODE_END:
        return key_code::end;
    case SDL_SCANCODE_CAPSLOCK:
        return key_code::caps_lock;
    case SDL_SCANCODE_SCROLLLOCK:
        return key_code::scroll_lock;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return key_code::num_lock;
    case SDL_SCANCODE_PRINTSCREEN:
        return key_code::print_screen;
    case SDL_SCANCODE_PAUSE:
        return key_code::pause;
    case SDL_SCANCODE_F1:
        return key_code::f1;
    case SDL_SCANCODE_F2:
        return key_code::f2;
    case SDL_SCANCODE_F3:
        return key_code::f3;
    case SDL_SCANCODE_F4:
        return key_code::f4;
    case SDL_SCANCODE_F5:
        return key_code::f5;
    case SDL_SCANCODE_F6:
        return key_code::f6;
    case SDL_SCANCODE_F7:
        return key_code::f7;
    case SDL_SCANCODE_F8:
        return key_code::f8;
    case SDL_SCANCODE_F9:
        return key_code::f9;
    case SDL_SCANCODE_F10:
        return key_code::f10;
    case SDL_SCANCODE_F11:
        return key_code::f11;
    case SDL_SCANCODE_F12:
        return key_code::f12;
    case SDL_SCANCODE_F13:
        return key_code::f13;
    case SDL_SCANCODE_F14:
        return key_code::f14;
    case SDL_SCANCODE_F15:
        return key_code::f15;
    case SDL_SCANCODE_F16:
        return key_code::f16;
    case SDL_SCANCODE_F17:
        return key_code::f17;
    case SDL_SCANCODE_F18:
        return key_code::f18;
    case SDL_SCANCODE_F19:
        return key_code::f19;
    case SDL_SCANCODE_F20:
        return key_code::f20;
    case SDL_SCANCODE_F21:
        return key_code::f21;
    case SDL_SCANCODE_F22:
        return key_code::f22;
    case SDL_SCANCODE_F23:
        return key_code::f23;
    case SDL_SCANCODE_F24:
        return key_code::f24;
    case SDL_SCANCODE_KP_0:
        return key_code::kp_0;
    case SDL_SCANCODE_KP_1:
        return key_code::kp_1;
    case SDL_SCANCODE_KP_2:
        return key_code::kp_2;
    case SDL_SCANCODE_KP_3:
        return key_code::kp_3;
    case SDL_SCANCODE_KP_4:
        return key_code::kp_4;
    case SDL_SCANCODE_KP_5:
        return key_code::kp_5;
    case SDL_SCANCODE_KP_6:
        return key_code::kp_6;
    case SDL_SCANCODE_KP_7:
        return key_code::kp_7;
    case SDL_SCANCODE_KP_8:
        return key_code::kp_8;
    case SDL_SCANCODE_KP_9:
        return key_code::kp_9;
    case SDL_SCANCODE_KP_DECIMAL:
        return key_code::kp_decimal;
    case SDL_SCANCODE_KP_DIVIDE:
        return key_code::kp_divide;
    case SDL_SCANCODE_KP_MULTIPLY:
        return key_code::kp_multiply;
    case SDL_SCANCODE_KP_MINUS:
        return key_code::kp_subtract;
    case SDL_SCANCODE_KP_PLUS:
        return key_code::kp_add;
    case SDL_SCANCODE_KP_ENTER:
        return key_code::kp_enter;
    case SDL_SCANCODE_KP_EQUALS:
        return key_code::kp_equal;
    case SDL_SCANCODE_LSHIFT:
        return key_code::left_shift;
    case SDL_SCANCODE_LCTRL:
        return key_code::left_control;
    case SDL_SCANCODE_LALT:
        return key_code::left_alt;
    case SDL_SCANCODE_LGUI:
        return key_code::left_super;
    case SDL_SCANCODE_RSHIFT:
        return key_code::right_shift;
    case SDL_SCANCODE_RCTRL:
        return key_code::right_control;
    case SDL_SCANCODE_RALT:
        return key_code::right_alt;
    case SDL_SCANCODE_RGUI:
        return key_code::right_super;
    case SDL_SCANCODE_MENU:
        return key_code::menu;
    default:
        return key_code::unknown;
    }
}

inline SDL_Scancode to_sdl_scancode(key_code key)
{
    switch (key)
    {
    case key_code::space:
        return SDL_SCANCODE_SPACE;
    case key_code::apostrophe:
        return SDL_SCANCODE_APOSTROPHE;
    case key_code::comma:
        return SDL_SCANCODE_COMMA;
    case key_code::minus:
        return SDL_SCANCODE_MINUS;
    case key_code::period:
        return SDL_SCANCODE_PERIOD;
    case key_code::slash:
        return SDL_SCANCODE_SLASH;
    case key_code::num_0:
        return SDL_SCANCODE_0;
    case key_code::num_1:
        return SDL_SCANCODE_1;
    case key_code::num_2:
        return SDL_SCANCODE_2;
    case key_code::num_3:
        return SDL_SCANCODE_3;
    case key_code::num_4:
        return SDL_SCANCODE_4;
    case key_code::num_5:
        return SDL_SCANCODE_5;
    case key_code::num_6:
        return SDL_SCANCODE_6;
    case key_code::num_7:
        return SDL_SCANCODE_7;
    case key_code::num_8:
        return SDL_SCANCODE_8;
    case key_code::num_9:
        return SDL_SCANCODE_9;
    case key_code::semicolon:
        return SDL_SCANCODE_SEMICOLON;
    case key_code::equal:
        return SDL_SCANCODE_EQUALS;
    case key_code::a:
        return SDL_SCANCODE_A;
    case key_code::b:
        return SDL_SCANCODE_B;
    case key_code::c:
        return SDL_SCANCODE_C;
    case key_code::d:
        return SDL_SCANCODE_D;
    case key_code::e:
        return SDL_SCANCODE_E;
    case key_code::f:
        return SDL_SCANCODE_F;
    case key_code::g:
        return SDL_SCANCODE_G;
    case key_code::h:
        return SDL_SCANCODE_H;
    case key_code::i:
        return SDL_SCANCODE_I;
    case key_code::j:
        return SDL_SCANCODE_J;
    case key_code::k:
        return SDL_SCANCODE_K;
    case key_code::l:
        return SDL_SCANCODE_L;
    case key_code::m:
        return SDL_SCANCODE_M;
    case key_code::n:
        return SDL_SCANCODE_N;
    case key_code::o:
        return SDL_SCANCODE_O;
    case key_code::p:
        return SDL_SCANCODE_P;
    case key_code::q:
        return SDL_SCANCODE_Q;
    case key_code::r:
        return SDL_SCANCODE_R;
    case key_code::s:
        return SDL_SCANCODE_S;
    case key_code::t:
        return SDL_SCANCODE_T;
    case key_code::u:
        return SDL_SCANCODE_U;
    case key_code::v:
        return SDL_SCANCODE_V;
    case key_code::w:
        return SDL_SCANCODE_W;
    case key_code::x:
        return SDL_SCANCODE_X;
    case key_code::y:
        return SDL_SCANCODE_Y;
    case key_code::z:
        return SDL_SCANCODE_Z;
    case key_code::left_bracket:
        return SDL_SCANCODE_LEFTBRACKET;
    case key_code::backslash:
        return SDL_SCANCODE_BACKSLASH;
    case key_code::right_bracket:
        return SDL_SCANCODE_RIGHTBRACKET;
    case key_code::grave_accent:
        return SDL_SCANCODE_GRAVE;
    case key_code::world_1:
        return SDL_SCANCODE_NONUSBACKSLASH;
    case key_code::world_2:
        return SDL_SCANCODE_NONUSHASH;
    case key_code::escape:
        return SDL_SCANCODE_ESCAPE;
    case key_code::enter:
        return SDL_SCANCODE_RETURN;
    case key_code::tab:
        return SDL_SCANCODE_TAB;
    case key_code::backspace:
        return SDL_SCANCODE_BACKSPACE;
    case key_code::insert:
        return SDL_SCANCODE_INSERT;
    case key_code::del:
        return SDL_SCANCODE_DELETE;
    case key_code::right:
        return SDL_SCANCODE_RIGHT;
    case key_code::left:
        return SDL_SCANCODE_LEFT;
    case key_code::down:
        return SDL_SCANCODE_DOWN;
    case key_code::up:
        return SDL_SCANCODE_UP;
    case key_code::page_up:
        return SDL_SCANCODE_PAGEUP;
    case key_code::page_down:
        return SDL_SCANCODE_PAGEDOWN;
    case key_code::home:
        return SDL_SCANCODE_HOME;
    case key_code::end:
        return SDL_SCANCODE_END;
    case key_code::caps_lock:
        return SDL_SCANCODE_CAPSLOCK;
    case key_code::scroll_lock:
        return SDL_SCANCODE_SCROLLLOCK;
    case key_code::num_lock:
        return SDL_SCANCODE_NUMLOCKCLEAR;
    case key_code::print_screen:
        return SDL_SCANCODE_PRINTSCREEN;
    case key_code::pause:
        return SDL_SCANCODE_PAUSE;
    case key_code::f1:
        return SDL_SCANCODE_F1;
    case key_code::f2:
        return SDL_SCANCODE_F2;
    case key_code::f3:
        return SDL_SCANCODE_F3;
    case key_code::f4:
        return SDL_SCANCODE_F4;
    case key_code::f5:
        return SDL_SCANCODE_F5;
    case key_code::f6:
        return SDL_SCANCODE_F6;
    case key_code::f7:
        return SDL_SCANCODE_F7;
    case key_code::f8:
        return SDL_SCANCODE_F8;
    case key_code::f9:
        return SDL_SCANCODE_F9;
    case key_code::f10:
        return SDL_SCANCODE_F10;
    case key_code::f11:
        return SDL_SCANCODE_F11;
    case key_code::f12:
        return SDL_SCANCODE_F12;
    case key_code::f13:
        return SDL_SCANCODE_F13;
    case key_code::f14:
        return SDL_SCANCODE_F14;
    case key_code::f15:
        return SDL_SCANCODE_F15;
    case key_code::f16:
        return SDL_SCANCODE_F16;
    case key_code::f17:
        return SDL_SCANCODE_F17;
    case key_code::f18:
        return SDL_SCANCODE_F18;
    case key_code::f19:
        return SDL_SCANCODE_F19;
    case key_code::f20:
        return SDL_SCANCODE_F20;
    case key_code::f21:
        return SDL_SCANCODE_F21;
    case key_code::f22:
        return SDL_SCANCODE_F22;
    case key_code::f23:
        return SDL_SCANCODE_F23;
    case key_code::f24:
        return SDL_SCANCODE_F24;
    // No F25 in SDL
    case key_code::kp_0:
        return SDL_SCANCODE_KP_0;
    case key_code::kp_1:
        return SDL_SCANCODE_KP_1;
    case key_code::kp_2:
        return SDL_SCANCODE_KP_2;
    case key_code::kp_3:
        return SDL_SCANCODE_KP_3;
    case key_code::kp_4:
        return SDL_SCANCODE_KP_4;
    case key_code::kp_5:
        return SDL_SCANCODE_KP_5;
    case key_code::kp_6:
        return SDL_SCANCODE_KP_6;
    case key_code::kp_7:
        return SDL_SCANCODE_KP_7;
    case key_code::kp_8:
        return SDL_SCANCODE_KP_8;
    case key_code::kp_9:
        return SDL_SCANCODE_KP_9;
    case key_code::kp_decimal:
        return SDL_SCANCODE_KP_DECIMAL;
    case key_code::kp_divide:
        return SDL_SCANCODE_KP_DIVIDE;
    case key_code::kp_multiply:
        return SDL_SCANCODE_KP_MULTIPLY;
    case key_code::kp_subtract:
        return SDL_SCANCODE_KP_MINUS;
    case key_code::kp_add:
        return SDL_SCANCODE_KP_PLUS;
    case key_code::kp_enter:
        return SDL_SCANCODE_KP_ENTER;
    case key_code::kp_equal:
        return SDL_SCANCODE_KP_EQUALS;
    case key_code::left_shift:
        return SDL_SCANCODE_LSHIFT;
    case key_code::left_control:
        return SDL_SCANCODE_LCTRL;
    case key_code::left_alt:
        return SDL_SCANCODE_LALT;
    case key_code::left_super:
        return SDL_SCANCODE_LGUI;
    case key_code::right_shift:
        return SDL_SCANCODE_RSHIFT;
    case key_code::right_control:
        return SDL_SCANCODE_RCTRL;
    case key_code::right_alt:
        return SDL_SCANCODE_RALT;
    case key_code::right_super:
        return SDL_SCANCODE_RGUI;
    case key_code::menu:
        return SDL_SCANCODE_MENU;
    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}
} // namespace zabato
