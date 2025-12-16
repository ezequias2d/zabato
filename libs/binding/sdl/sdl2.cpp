#include "SDL_gamecontroller.h"
#include "sdl2_keymap.hpp"
#include "zabato/window.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <zabato/hash_map.hpp>
#include <zabato/sdl2.hpp>

namespace zabato
{

static hash_map<Uint32, Sdl2Window *> g_window_map;
static hash_map<int, SDL_GameController *> g_controllers;
static gamepad_callback g_gamepad_cb = nullptr;
static int g_display_indices[32];
static monitor *g_monitor_pointers[32];

void dispatch_gamepad_event(SDL_Event *event)
{
    if (event->type == SDL_CONTROLLERDEVICEADDED)
    {
        int joy_index = event->cdevice.which;
        if (!SDL_IsGameController(joy_index))
        {
            return;
        }
        SDL_GameController *controller = SDL_GameControllerOpen(joy_index);
        if (controller)
        {
            g_controllers.add_or_set(joy_index, controller);
            if (g_gamepad_cb)
            {
                g_gamepad_cb(joy_index, connect_event::connected);
            }
        }
    }
    else if (event->type == SDL_CONTROLLERDEVICEREMOVED)
    {
        int joy_index = event->cdevice.which;
        SDL_GameController *controller =
            SDL_GameControllerFromInstanceID(joy_index);
        if (!controller)
            return;

        // Find the joystick index associated with this controller
        int jid = -1;
        for (auto const &[index, ctl, _] : g_controllers)
        {
            if (ctl == controller)
            {
                jid = index;
                break;
            }
        }

        if (jid != -1)
        {
            g_controllers.erase(jid);
            if (g_gamepad_cb)
                g_gamepad_cb(jid, connect_event::disconnected);
        }
        SDL_GameControllerClose(controller);
    }
}

void dispatch_event(SDL_Event *event)
{
    Sdl2Window *win = nullptr;
    Uint32 windowID = 0;

    switch (event->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        windowID = event->key.windowID;
        break;
    case SDL_TEXTINPUT:
        windowID = event->text.windowID;
        break;
    case SDL_MOUSEMOTION:
        windowID = event->motion.windowID;
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        windowID = event->button.windowID;
        break;
    case SDL_MOUSEWHEEL:
        windowID = event->wheel.windowID;
        break;
    case SDL_DROPFILE:
    case SDL_DROPTEXT:
    case SDL_DROPBEGIN:
    case SDL_DROPCOMPLETE:
        windowID = event->drop.windowID;
        break;
    case SDL_WINDOWEVENT:
        windowID = event->window.windowID;
        break;
    case SDL_QUIT:
        for (auto const &[id, w, _] : g_window_map)
        {
            if (w)
                w->set_should_close(true);
        }
        return;

    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
        dispatch_gamepad_event(event);
        return;
    }

    // Find the window instance and forward the event
    if (windowID != 0)
    {
        Sdl2Window *win = nullptr;
        if (g_window_map.try_get_value(windowID, win))
        {
            assert(win);
            win->handle_event(event);
        }
    }
}

Sdl2Window::Sdl2Window(int x,
                       int y,
                       int width,
                       int height,
                       const char *title,
                       window_flags flags)
{
    Uint32 sdl_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

    if ((flags & window_flags::no_border) != window_flags::none)
        sdl_flags |= SDL_WINDOW_BORDERLESS;
    if ((flags & window_flags::no_resize) == window_flags::none)
        sdl_flags |= SDL_WINDOW_RESIZABLE;
    if ((flags & window_flags::fullscreen) != window_flags::none)
        sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    /*
    TODO: port to SDL3 to use SDL_WINDOW_TRANSPARENT wit
    window_flags::transparent
    */
    assert((flags & window_flags::transparent) == window_flags::none);

    int win_x = x;
    int win_y = y;
    if ((flags & window_flags::center) != window_flags::none)
    {
        win_x = SDL_WINDOWPOS_CENTERED;
        win_y = SDL_WINDOWPOS_CENTERED;
    }

    // Create the window
    m_handle = SDL_CreateWindow(title, win_x, win_y, width, height, sdl_flags);
    if (!m_handle)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Create the OpenGL context
    m_context = SDL_GL_CreateContext(m_handle);
    if (!m_context)
    {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError()
                  << std::endl;
        SDL_DestroyWindow(m_handle);
        exit(EXIT_FAILURE);
    }

    // Register window in global map
    m_windowID = SDL_GetWindowID(m_handle);
    g_window_map.add_or_set(m_windowID, this);

    if ((flags & window_flags::hide_mouse) != window_flags::none)
        hide_cursor();

    // Enable text input events for char_callback
    SDL_StartTextInput();

    // Create standard system cursors
    m_cursors[static_cast<int>(mouse_icon::arrow)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    m_cursors[static_cast<int>(mouse_icon::ibeam)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    m_cursors[static_cast<int>(mouse_icon::crosshair)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    m_cursors[static_cast<int>(mouse_icon::pointing_hand)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    m_cursors[static_cast<int>(mouse_icon::resize_left_right)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    m_cursors[static_cast<int>(mouse_icon::resize_up_down)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    m_cursors[static_cast<int>(mouse_icon::resize_all)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    m_cursors[static_cast<int>(mouse_icon::resize_left_up_right_down)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    m_cursors[static_cast<int>(mouse_icon::resize_left_down_right_up)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    m_cursors[static_cast<int>(mouse_icon::not_allowed)] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
}

Sdl2Window::~Sdl2Window() { destroy(); }

void Sdl2Window::handle_event(SDL_Event *event)
{
    switch (event->type)
    {
    // Window Events
    case SDL_WINDOWEVENT:
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_CLOSE:
            m_should_close = true;
            break;
        case SDL_WINDOWEVENT_ENTER:
            for (auto cb : m_cursor_enter_cbs)
                cb(this, 1);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            for (auto cb : m_cursor_enter_cbs)
                cb(this, 0);
            break;
        }
        break;

    // Keyboard Events
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        for (auto cb : m_key_cbs)
            cb(this,
               to_keycode(event->key.keysym.scancode),
               event->key.keysym.scancode, // Pass raw scancode
               to_button_state(event->key.state, event->key.repeat),
               to_modifier_keys((SDL_Keymod)event->key.keysym.mod));
        break;

    // Text Input Event
    case SDL_TEXTINPUT:
        for (auto cb : m_text_input_cbs)
        {
            // SDL_TEXTINPUT provides UTF-8 string
            if (event->text.text[0] != '\0')
                cb(this, event->text.text);
        }
        break;

    // Mouse Events
    case SDL_MOUSEMOTION:
        for (auto cb : m_cursor_pos_cbs)
            cb(this,
               static_cast<real>(event->motion.x),
               static_cast<real>(event->motion.y));
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        for (auto cb : m_mouse_button_cbs)
        {
            cb(this,
               to_mouse_button(event->button.button),
               to_button_state(event->button.state, 0),
               to_modifier_keys(SDL_GetModState()));
        }
        break;
    case SDL_MOUSEWHEEL:
        for (auto cb : m_scroll_cbs)
            cb(this,
               static_cast<real>(event->wheel.x),
               static_cast<real>(event->wheel.y));
        break;

    // Drag and Drop Events
    case SDL_DROPFILE:
        for (auto cb : m_drop_cbs)
        {
            const char *paths[] = {event->drop.file};
            cb(this, 1, paths);
        }
        // Must free the path string
        SDL_free(event->drop.file);
        break;
    }
}

void Sdl2Window::destroy()
{
    // Free clipboard text buffer
    if (m_clipboard_text_ptr)
        SDL_free(m_clipboard_text_ptr);

    // Stop text input
    SDL_StopTextInput();

    // Free cursors
    for (int i = 0; i < 10; ++i)
    {
        if (m_cursors[i])
            SDL_FreeCursor(m_cursors[i]);
    }

    // Unregister from global map
    if (m_windowID != 0)
    {
        g_window_map.erase(m_windowID);
    }

    // Destroy context and window
    if (m_context)
        SDL_GL_DeleteContext(m_context);
    if (m_handle)
        SDL_DestroyWindow(m_handle);
}

void Sdl2Window::swap_buffers() { SDL_GL_SwapWindow(m_handle); }

bool Sdl2Window::should_close() const { return m_should_close; }

void Sdl2Window::set_should_close(bool value) { m_should_close = value; }

void Sdl2Window::close() { m_should_close = true; }

vec2<int> Sdl2Window::get_size() const
{
    int w, h;
    SDL_GetWindowSize(m_handle, &w, &h);
    return {w, h};
}

vec2<int> Sdl2Window::get_position() const
{
    int x, y;
    SDL_GetWindowPosition(m_handle, &x, &y);
    return {x, y};
}

void Sdl2Window::relocate(const vec2<int> &new_position)
{
    SDL_SetWindowPosition(m_handle, new_position.x, new_position.y);
}

void Sdl2Window::resize(const vec2<int> &new_size)
{
    SDL_SetWindowSize(m_handle, new_size.x, new_size.y);
}

vec2<int> Sdl2Window::get_framebuffer_size() const
{
    int w, h;
    SDL_GL_GetDrawableSize(m_handle, &w, &h);
    return {w, h};
}

vec2<real> Sdl2Window::get_cursor_pos() const
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    return {real(x), real(y)};
}

bool Sdl2Window::is_key_down(key_code key) const
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    return state[to_sdl_scancode(key)] == 1;
}

bool Sdl2Window::is_mouse_button_down(mouse_button btn) const
{
    Uint32 state = SDL_GetMouseState(NULL, NULL);
    return (state & SDL_BUTTON(to_sdl_mouse(btn))) != 0;
}

void *Sdl2Window::get_userdata() const { return m_userdata; }

void Sdl2Window::set_userdata(void *data) { m_userdata = data; }

bool Sdl2Window::is_fullscreen() const
{
    return (SDL_GetWindowFlags(m_handle) &
            (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

bool Sdl2Window::is_hidden() const
{
    return (SDL_GetWindowFlags(m_handle) & SDL_WINDOW_HIDDEN) != 0;
}

bool Sdl2Window::is_minimized() const
{
    return (SDL_GetWindowFlags(m_handle) & SDL_WINDOW_MINIMIZED) != 0;
}

bool Sdl2Window::is_maximized() const
{
    return (SDL_GetWindowFlags(m_handle) & SDL_WINDOW_MAXIMIZED) != 0;
}

void Sdl2Window::set_cursor_pos(const vec2<real> &pos)
{
    SDL_WarpMouseInWindow(m_handle, float(pos.x), float(pos.y));
}

void Sdl2Window::move(const vec2<int> &new_position)
{
    SDL_SetWindowPosition(m_handle, new_position.x, new_position.y);
}

void Sdl2Window::set_title(const char *title)
{
    SDL_SetWindowTitle(m_handle, title);
}

void Sdl2Window::set_icon(int width, int height, const uint32_t *pixels)
{
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    SDL_Surface *icon = SDL_CreateRGBSurfaceFrom((void *)pixels,
                                                 width,
                                                 height,
                                                 32,
                                                 width * 4,
                                                 rmask,
                                                 gmask,
                                                 bmask,
                                                 amask);

    if (icon)
    {
        SDL_SetWindowIcon(m_handle, icon);
        SDL_FreeSurface(icon); // SDL_SetWindowIcon makes a copy
    }
}

void Sdl2Window::set_cursor_icon(mouse_icon icon)
{
    SDL_Cursor *cursor = m_cursors[static_cast<int>(icon)];
    if (!cursor)
    {
        // Fallback to arrow if creation failed
        cursor = m_cursors[static_cast<int>(mouse_icon::arrow)];
    }
    SDL_SetCursor(cursor);
}

void Sdl2Window::set_limits(const vec2<int> &min_size,
                            const vec2<int> &max_size)
{
    SDL_SetWindowMinimumSize(m_handle, min_size.x, min_size.y);
    SDL_SetWindowMaximumSize(m_handle, max_size.x, max_size.y);
}

void Sdl2Window::maximize() { SDL_MaximizeWindow(m_handle); }

void Sdl2Window::minimize() { SDL_MinimizeWindow(m_handle); }

void Sdl2Window::restore() { SDL_RestoreWindow(m_handle); }

void Sdl2Window::hide() { SDL_HideWindow(m_handle); }

void Sdl2Window::show() { SDL_ShowWindow(m_handle); }

void Sdl2Window::hold_cursor() { SDL_SetRelativeMouseMode(SDL_TRUE); }

void Sdl2Window::release_cursor() { SDL_SetRelativeMouseMode(SDL_FALSE); }

void Sdl2Window::hide_cursor() { SDL_ShowCursor(SDL_DISABLE); }

monitor *Sdl2Window::get_monitor()
{
    int index = SDL_GetWindowDisplayIndex(m_handle);
    if (index < 0)
        return nullptr;
    // Return the index cast to the opaque monitor pointer
    return reinterpret_cast<monitor *>((intptr_t)index);
}

void Sdl2Window::move_to_monitor(monitor *mon)
{
    int displayIndex = (intptr_t)mon;

    // This implements GLFW's glfwSetWindowMonitor behavior:
    // It makes the window fullscreen on the specified monitor.
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(displayIndex, &mode);
    SDL_SetWindowDisplayMode(m_handle, &mode);
    SDL_SetWindowFullscreen(m_handle, SDL_WINDOW_FULLSCREEN);
}

const char *Sdl2Window::read_clipboard()
{
    // Free the old buffer if it exists
    if (m_clipboard_text_ptr)
    {
        SDL_free(m_clipboard_text_ptr);
        m_clipboard_text_ptr = nullptr;
    }
    m_clipboard_text_ptr = SDL_GetClipboardText();
    return m_clipboard_text_ptr;
}

void Sdl2Window::write_clipboard(const char *text)
{
    SDL_SetClipboardText(text);
}

void Sdl2Window::start_text_input() { SDL_StartTextInput(); }

void Sdl2Window::stop_text_input() { SDL_StopTextInput(); }

template <typename T> static void remove_callback_helper(vector<T> &vec, T cb)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (vec[i] == cb)
        {
            // Shift elements to preserve order
            for (size_t j = i; j < vec.size() - 1; ++j)
            {
                vec[j] = vec[j + 1];
            }
            vec.pop_back();
            i--;
        }
    }
}

void Sdl2Window::add_key_callback(key_callback cb) { m_key_cbs.push_back(cb); }
void Sdl2Window::remove_key_callback(key_callback cb)
{
    remove_callback_helper(m_key_cbs, cb);
}

void Sdl2Window::add_text_input_callback(text_input_callback cb)
{
    m_text_input_cbs.push_back(cb);
}
void Sdl2Window::remove_text_input_callback(text_input_callback cb)
{
    remove_callback_helper(m_text_input_cbs, cb);
}

void Sdl2Window::add_cursor_pos_callback(cursor_pos_callback cb)
{
    m_cursor_pos_cbs.push_back(cb);
}
void Sdl2Window::remove_cursor_pos_callback(cursor_pos_callback cb)
{
    remove_callback_helper(m_cursor_pos_cbs, cb);
}

void Sdl2Window::add_cursor_enter_callback(cursor_enter_callback cb)
{
    m_cursor_enter_cbs.push_back(cb);
}
void Sdl2Window::remove_cursor_enter_callback(cursor_enter_callback cb)
{
    remove_callback_helper(m_cursor_enter_cbs, cb);
}

void Sdl2Window::add_mouse_button_callback(mouse_button_callback cb)
{
    m_mouse_button_cbs.push_back(cb);
}
void Sdl2Window::remove_mouse_button_callback(mouse_button_callback cb)
{
    remove_callback_helper(m_mouse_button_cbs, cb);
}

void Sdl2Window::add_scroll_callback(scroll_callback cb)
{
    m_scroll_cbs.push_back(cb);
}
void Sdl2Window::remove_scroll_callback(scroll_callback cb)
{
    remove_callback_helper(m_scroll_cbs, cb);
}

void Sdl2Window::add_drop_callback(drop_callback cb)
{
    m_drop_cbs.push_back(cb);
}
void Sdl2Window::remove_drop_callback(drop_callback cb)
{
    remove_callback_helper(m_drop_cbs, cb);
}

bool init_window_system()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK |
                 SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    return true;
}

void terminate_window_system()
{
    // Close all game controllers
    for (auto const &[id, controller, _] : g_controllers)
    {
        if (controller)
            SDL_GameControllerClose(controller);
    }

    g_controllers.clear();

    SDL_Quit();
}

window *create_window(int x,
                      int y,
                      int width,
                      int height,
                      const char *title,
                      window_flags flags)
{
    return new Sdl2Window(x, y, width, height, title, flags);
}

void poll_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        dispatch_event(&event);
}

void wait_events()
{
    SDL_Event event;
    if (SDL_WaitEvent(&event))
    {
        dispatch_event(&event);
        poll_events();
    }
}

void wait_events_timeout(double timeout)
{
    SDL_Event event;
    if (SDL_WaitEventTimeout(&event, (int)(timeout * 1000.0)))
    {
        dispatch_event(&event);

        // After dispatching, poll for any other pending events
        poll_events();
    }
}

uint64_t get_time() { return SDL_GetTicks64(); }

const char *get_key_name(key_code key)
{
    SDL_Scancode scancode = to_sdl_scancode(key);
    SDL_Keycode keycode   = SDL_GetKeyFromScancode(scancode);
    return SDL_GetKeyName(keycode);
}

monitor **get_monitors(size_t *count)
{
    int num_displays = SDL_GetNumVideoDisplays();
    if (num_displays < 0)
        num_displays = 0;
    if (num_displays > 32)
        num_displays = 32;

    *count = num_displays;
    for (int i = 0; i < num_displays; ++i)
        g_monitor_pointers[i] = reinterpret_cast<monitor *>((intptr_t)i);

    return g_monitor_pointers;
}

bool get_gamepad_state(int jid, gamepad_state *state)
{
    SDL_GameController *controller = nullptr;
    if (!g_controllers.try_get_value(jid, controller))
        return false;

    // Convert buttons
    for (int i = 0; i <= (int)gamepad_button::r3; ++i)
    {
        state->buttons[i] = to_button_state(
            SDL_GameControllerGetButton(
                controller,
                to_sdl_gamepad_button(static_cast<gamepad_button>(i))),
            0); // No repeat for gamepad buttons
    }

    // Convert axes
    for (int i = 0; i <= (int)gamepad_analog::r2; ++i)
    {
        Sint16 axis_val = SDL_GameControllerGetAxis(
            controller, to_sdl_gamepad_axis(static_cast<gamepad_analog>(i)));

        // Normalize from [-32768, 32767] to [-1.0, 1.0]
        if (axis_val < 0)
            state->analogs[i] =
                static_cast<real>(axis_val) * (real(1.0) / real(32768.0));
        else
            state->analogs[i] =
                static_cast<real>(axis_val) * (real(1.0) / real(32767.0));
    }

    return true;
}

const char *get_gamepad_name(int jid)
{
    SDL_GameController *controller = nullptr;
    if (g_controllers.try_get_value(jid, controller))
        return SDL_GameControllerName(controller);
    return SDL_GameControllerNameForIndex(jid);
}

void set_gamepad_callback(gamepad_callback cb) { g_gamepad_cb = cb; }

gl_proc get_proc_address(const char *procname)
{
    return reinterpret_cast<gl_proc>(SDL_GL_GetProcAddress(procname));
}

void make_context_current(window *win)
{
    Sdl2Window *sdl_win = static_cast<Sdl2Window *>(win);
    SDL_GL_MakeCurrent(sdl_win->get_handle(), sdl_win->m_context);
}

} // namespace zabato
