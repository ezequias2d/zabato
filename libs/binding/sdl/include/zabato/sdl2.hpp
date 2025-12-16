#pragma once

#include <SDL2/SDL.h>
#include <zabato/vector.hpp>
#include <zabato/window.hpp>

namespace zabato
{

mouse_button to_mouse_button(Uint8 sdl_button);
key_code to_keycode(SDL_Scancode scancode);
button_state to_button_state(Uint8 state, Uint8 repeat);
modifier_keys to_modifier_keys(SDL_Keymod mod);
gamepad_button to_gamepad_button(int sdl_button);

int to_sdl_mouse(mouse_button btn);
SDL_Scancode to_sdl_scancode(key_code key);
SDL_GameControllerButton to_sdl_gamepad_button(gamepad_button btn);
SDL_GameControllerAxis to_sdl_gamepad_axis(gamepad_analog axis);

class Sdl2Window : public window
{
public:
    Sdl2Window(int x,
               int y,
               int width,
               int height,
               const char *title,
               window_flags flags);
    virtual ~Sdl2Window() override;

    void destroy() override;

    void swap_buffers() override;
    bool should_close() const override;
    void set_should_close(bool value) override;
    void close() override;

    vec2<int> get_size() const override;
    vec2<int> get_position() const override;
    void relocate(const vec2<int> &new_position) override;
    void resize(const vec2<int> &new_size) override;
    vec2<int> get_framebuffer_size() const override;
    vec2<real> get_cursor_pos() const override;
    bool is_key_down(key_code key) const override;
    bool is_mouse_button_down(mouse_button btn) const override;
    void *get_userdata() const override;
    bool is_fullscreen() const override;
    bool is_hidden() const override;
    bool is_minimized() const override;
    bool is_maximized() const override;

    void set_cursor_pos(const vec2<real> &pos) override;
    void move(const vec2<int> &new_position) override;
    void set_userdata(void *data) override;
    void set_title(const char *title) override;
    void set_icon(int width, int height, const uint32_t *pixels) override;
    void set_cursor_icon(mouse_icon icon) override;
    void set_limits(const vec2<int> &min_size,
                    const vec2<int> &max_size) override;

    void maximize() override;
    void minimize() override;
    void restore() override;
    void hide() override;
    void show() override;
    void hold_cursor() override;
    void release_cursor() override;
    void hide_cursor() override;

    monitor *get_monitor() override;
    void move_to_monitor(monitor *mon) override;

    const char *read_clipboard() override;
    void write_clipboard(const char *text) override;

    void start_text_input() override;
    void stop_text_input() override;

    void add_key_callback(key_callback cb) override;
    void remove_key_callback(key_callback cb) override;

    void add_text_input_callback(text_input_callback cb) override;
    void remove_text_input_callback(text_input_callback cb) override;

    void add_cursor_pos_callback(cursor_pos_callback cb) override;
    void remove_cursor_pos_callback(cursor_pos_callback cb) override;

    void add_cursor_enter_callback(cursor_enter_callback cb) override;
    void remove_cursor_enter_callback(cursor_enter_callback cb) override;

    void add_mouse_button_callback(mouse_button_callback cb) override;
    void remove_mouse_button_callback(mouse_button_callback cb) override;

    void add_scroll_callback(scroll_callback cb) override;
    void remove_scroll_callback(scroll_callback cb) override;

    void add_drop_callback(drop_callback cb) override;
    void remove_drop_callback(drop_callback cb) override;

    SDL_Window *get_handle() { return m_handle; }
    void handle_event(SDL_Event *event);

private:
    SDL_Window *m_handle       = nullptr;
    SDL_GLContext m_context    = nullptr;
    SDL_Cursor *m_cursors[10]  = {nullptr};
    void *m_userdata           = nullptr;
    Uint32 m_windowID          = 0;
    bool m_should_close        = false;
    char *m_clipboard_text_ptr = nullptr;

    vector<key_callback> m_key_cbs;
    vector<text_input_callback> m_text_input_cbs;
    vector<cursor_pos_callback> m_cursor_pos_cbs;
    vector<cursor_enter_callback> m_cursor_enter_cbs;
    vector<mouse_button_callback> m_mouse_button_cbs;
    vector<scroll_callback> m_scroll_cbs;
    vector<drop_callback> m_drop_cbs;

    friend void make_context_current(window *win);
};

} // namespace zabato
