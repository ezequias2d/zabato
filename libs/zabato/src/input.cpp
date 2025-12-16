#include <zabato/input.hpp>

namespace zabato
{

const char *to_string(key_code key)
{
    switch (key)
    {
    case key_code::unknown:
        return "<unknown>";
    case key_code::space:
        return "<space>";
    case key_code::apostrophe:
        return "'";
    case key_code::comma:
        return ",";
    case key_code::minus:
        return "-";
    case key_code::period:
        return ".";
    case key_code::slash:
        return "/";
    case key_code::num_0:
        return "<num 0>";
    case key_code::num_1:
        return "<num 1>";
    case key_code::num_2:
        return "<num 2>";
    case key_code::num_3:
        return "<num 3>";
    case key_code::num_4:
        return "<num 4>";
    case key_code::num_5:
        return "<num 5>";
    case key_code::num_6:
        return "<num 6>";
    case key_code::num_7:
        return "<num 7>";
    case key_code::num_8:
        return "<num 8>";
    case key_code::num_9:
        return "<num 9>";
    case key_code::semicolon:
        return ";";
    case key_code::equal:
        return "=";
    case key_code::a:
        return "a";
    case key_code::b:
        return "b";
    case key_code::c:
        return "c";
    case key_code::d:
        return "d";
    case key_code::e:
        return "e";
    case key_code::f:
        return "f";
    case key_code::g:
        return "g";
    case key_code::h:
        return "h";
    case key_code::i:
        return "i";
    case key_code::j:
        return "j";
    case key_code::k:
        return "k";
    case key_code::l:
        return "l";
    case key_code::m:
        return "m";
    case key_code::n:
        return "n";
    case key_code::o:
        return "o";
    case key_code::p:
        return "p";
    case key_code::q:
        return "q";
    case key_code::r:
        return "r";
    case key_code::s:
        return "s";
    case key_code::t:
        return "t";
    case key_code::u:
        return "u";
    case key_code::v:
        return "v";
    case key_code::w:
        return "w";
    case key_code::x:
        return "x";
    case key_code::y:
        return "y";
    case key_code::z:
        return "z";
    case key_code::left_bracket:
        return "[";
    case key_code::backslash:
        return "\\";
    case key_code::right_bracket:
        return "]";
    case key_code::grave_accent:
        return "`";
    case key_code::world_1:
        return "<world 1>";
    case key_code::world_2:
        return "<world 2>";
    case key_code::escape:
        return "<escape>";
    case key_code::enter:
        return "<enter>";
    case key_code::tab:
        return "<tab>";
    case key_code::backspace:
        return "<backspace>";
    case key_code::insert:
        return "<insert>";
    case key_code::del:
        return "<delete>";
    case key_code::right:
        return "<right>";
    case key_code::left:
        return "<left>";
    case key_code::down:
        return "<down>";
    case key_code::up:
        return "<up>";
    case key_code::page_up:
        return "<page up>";
    case key_code::page_down:
        return "<page down>";
    case key_code::home:
        return "<home>";
    case key_code::end:
        return "<end>";
    case key_code::caps_lock:
        return "<caps lock>";
    case key_code::scroll_lock:
        return "<scroll lock>";
    case key_code::num_lock:
        return "<num lock>";
    case key_code::print_screen:
        return "<print screen>";
    case key_code::pause:
        return "<pause>";
    case key_code::f1:
        return "<f1>";
    case key_code::f2:
        return "<f2>";
    case key_code::f3:
        return "<f3>";
    case key_code::f4:
        return "<f4>";
    case key_code::f5:
        return "<f5>";
    case key_code::f6:
        return "<f6>";
    case key_code::f7:
        return "<f7>";
    case key_code::f8:
        return "<f8>";
    case key_code::f9:
        return "<f9>";
    case key_code::f10:
        return "<f10>";
    case key_code::f11:
        return "<f11>";
    case key_code::f12:
        return "<f12>";
    case key_code::f13:
        return "<f13>";
    case key_code::f14:
        return "<f14>";
    case key_code::f15:
        return "<f15>";
    case key_code::f16:
        return "<f16>";
    case key_code::f17:
        return "<f17>";
    case key_code::f18:
        return "<f18>";
    case key_code::f19:
        return "<f19>";
    case key_code::f20:
        return "<f20>";
    case key_code::f21:
        return "<f21>";
    case key_code::f22:
        return "<f22>";
    case key_code::f23:
        return "<f23>";
    case key_code::f24:
        return "<f24>";
    case key_code::f25:
        return "<f25>";
    case key_code::kp_0:
        return "<kp 0>";
    case key_code::kp_1:
        return "<kp 1>";
    case key_code::kp_2:
        return "<kp 2>";
    case key_code::kp_3:
        return "<kp 3>";
    case key_code::kp_4:
        return "<kp 4>";
    case key_code::kp_5:
        return "<kp 5>";
    case key_code::kp_6:
        return "<kp 6>";
    case key_code::kp_7:
        return "<kp 7>";
    case key_code::kp_8:
        return "<kp 8>";
    case key_code::kp_9:
        return "<kp 9>";
    case key_code::kp_decimal:
        return "<kp .>";
    case key_code::kp_divide:
        return "<kp />";
    case key_code::kp_multiply:
        return "<kp *>";
    case key_code::kp_subtract:
        return "<kp ->";
    case key_code::kp_add:
        return "<kp +>";
    case key_code::kp_enter:
        return "<kp enter>";
    case key_code::kp_equal:
        return "<kp =>";
    case key_code::left_shift:
        return "<left shift>";
    case key_code::left_control:
        return "<left control>";
    case key_code::left_alt:
        return "<left alt>";
    case key_code::left_super:
        return "<left super>";
    case key_code::right_shift:
        return "<right shift>";
    case key_code::right_control:
        return "<right control>";
    case key_code::right_alt:
        return "<right alt>";
    case key_code::right_super:
        return "<right super>";
    case key_code::menu:
        return "<menu>";
    default:
        return "<invalid>";
    }
}

const char *to_string(mouse_button button)
{
    switch (button)
    {
    case mouse_button::none:
        return "<none>";
    case mouse_button::left:
        return "<left>";
    case mouse_button::middle:
        return "<middle>";
    case mouse_button::right:
        return "<right>";
    default:
        return "<invalid>";
    }
}
const char *to_string(mouse_icon icon)
{
    switch (icon)
    {
    case mouse_icon::arrow:
        return "<arrow>";
    case mouse_icon::ibeam:
        return "<ibeam>";
    case mouse_icon::crosshair:
        return "<crosshair>";
    case mouse_icon::pointing_hand:
        return "<pointing hand>";
    case mouse_icon::resize_left_right:
        return "<resize left/right>";
    case mouse_icon::resize_up_down:
        return "<resize up/down>";
    case mouse_icon::resize_all:
        return "<resize all>";
    case mouse_icon::resize_left_up_right_down:
        return "<resize left/up/right/down>";
    case mouse_icon::resize_left_down_right_up:
        return "<resize left/down/right/up>";
    case mouse_icon::not_allowed:
        return "<not allowed>";
    default:
        return "<invalid>";
    }
}

const char *to_string(button_state state)
{
    switch (state)
    {
    case button_state::release:
        return "<release>";
    case button_state::press:
        return "<press>";
    case button_state::repeat:
        return "<repeat>";
    default:
        return "<invalid>";
    }
}

const char *to_string(connect_event event)
{
    switch (event)
    {
    case connect_event::disconnected:
        return "<disconnected>";
    case connect_event::connected:
        return "<connected>";
    default:
        return "<invalid>";
    }
}

const char *to_string(gamepad_button button)
{
    switch (button)
    {
    case gamepad_button::unknown:
        return "<unknown>";
    case gamepad_button::ps_x:
        return "<ps x|xbox a>";
    case gamepad_button::ps_o:
        return "<ps o|xbox b>";
    case gamepad_button::ps_t:
        return "<ps t|xbox y>";
    case gamepad_button::ps_s:
        return "<ps s|xbox x>";
    case gamepad_button::start:
        return "<start>";
    case gamepad_button::select:
        return "<select>";
    case gamepad_button::home:
        return "<home>";
    case gamepad_button::dpad_up:
        return "<dpad up>";
    case gamepad_button::dpad_down:
        return "<dpad down>";
    case gamepad_button::dpad_left:
        return "<dpad left>";
    case gamepad_button::dpad_right:
        return "<dpad right>";
    case gamepad_button::l1:
        return "<L1>";
    case gamepad_button::l2:
        return "<L2>";
    case gamepad_button::l3:
        return "<L3>";
    case gamepad_button::r1:
        return "<R1>";
    case gamepad_button::r2:
        return "<R2>";
    case gamepad_button::r3:
        return "<R3>";
    default:
        return "<invalid>";
    }
}

const char *to_string(gamepad_analog analog)
{
    switch (analog)
    {
    case gamepad_analog::left_x:
        return "<left x>";
    case gamepad_analog::left_y:
        return "<left y>";
    case gamepad_analog::right_x:
        return "<right x>";
    case gamepad_analog::right_y:
        return "<right y>";
    case gamepad_analog::l2:
        return "<analog L2>";
    case gamepad_analog::r2:
        return "<analog R2>";
    default:
        return "<invalid>";
    }
}

} // namespace zabato