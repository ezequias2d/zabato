#pragma once

#include <stdint.h>
#include <zabato/input.hpp>

namespace zabato
{
struct monitor;

enum class window_flags : uint32_t
{
    none        = 0,
    transparent = (1 << 0),
    no_border   = (1 << 1),
    no_resize   = (1 << 2),
    hide_mouse  = (1 << 3),
    fullscreen  = (1 << 4),
    center      = (1 << 5),
};

inline window_flags operator|(window_flags a, window_flags b)
{
    return static_cast<window_flags>(static_cast<uint32_t>(a) |
                                     static_cast<uint32_t>(b));
}

inline window_flags operator&(window_flags a, window_flags b)
{
    return static_cast<window_flags>(static_cast<uint32_t>(a) &
                                     static_cast<uint32_t>(b));
}

/**
 * @brief Abstract interface representing a window.
 *
 * This class defines the common interface for window management, including
 * creation, input handling, and event callbacks. Implementations (e.g., SDL2,
 * GLFW) provide the concrete platform-specific behavior.
 */
class window
{
public:
    virtual ~window() = default;

    /**
     * @brief Destroys the window and releases associated resources.
     */
    virtual void destroy() = 0;

    /**
     * @brief Swaps the front and back buffers.
     * Use this method to display the rendered content.
     */
    virtual void swap_buffers() = 0;

    /**
     * @brief Checks if the window should close.
     * @return true if the window has received a close request, false otherwise.
     */
    virtual bool should_close() const = 0;

    /**
     * @brief Sets the should_close flag.
     * @param value true to signal the window to close.
     */
    virtual void set_should_close(bool value) = 0;

    /**
     * @brief Helper to set should_close to true.
     */
    virtual void close() = 0;

    /**
     * @brief Gets the size of the window in screen coordinates.
     * @return The width and height of the window.
     */
    virtual vec2<int> get_size() const = 0;

    /**
     * @brief Gets the position of the window's upper-left corner.
     * @return The x and y coordinates on the screen.
     */
    virtual vec2<int> get_position() const = 0;

    /**
     * @brief Moves the window to a new position.
     * @param new_position The new x and y coordinates.
     */
    virtual void relocate(const vec2<int> &new_position) = 0;

    /**
     * @brief Resizes the window.
     * @param new_size The new width and height.
     */
    virtual void resize(const vec2<int> &new_size) = 0;

    /**
     * @brief Gets the size of the framebuffer (pixels).
     * This may differ from window size on high-DPI displays.
     * @return The width and height in pixels.
     */
    virtual vec2<int> get_framebuffer_size() const = 0;

    /**
     * @brief Gets the current cursor position relative to the client area.
     * @return The x and y coordinates.
     */
    virtual vec2<real> get_cursor_pos() const = 0;

    /**
     * @brief Checks if a specific key is currently pressed.
     * @param key The key code to check.
     * @return true if the key is down, false otherwise.
     */
    virtual bool is_key_down(key_code key) const = 0;

    /**
     * @brief Checks if a mouse button is currently pressed.
     * @param btn The mouse button to check.
     * @return true if the button is down, false otherwise.
     */
    virtual bool is_mouse_button_down(mouse_button btn) const = 0;

    /**
     * @brief Gets the user-associated data pointer.
     * @return The user data pointer.
     */
    virtual void *get_userdata() const = 0;

    /**
     * @brief Checks if the window is currently fullscreen.
     * @return true if fullscreen, false otherwise.
     */
    virtual bool is_fullscreen() const = 0;

    /**
     * @brief Checks if the window is currently hidden.
     * @return true if hidden, false otherwise.
     */
    virtual bool is_hidden() const = 0;

    /**
     * @brief Checks if the window is currently minimized.
     * @return true if minimized, false otherwise.
     */
    virtual bool is_minimized() const = 0;

    /**
     * @brief Checks if the window is currently maximized.
     * @return true if maximized, false otherwise.
     */
    virtual bool is_maximized() const = 0;

    /**
     * @brief Sets the cursor position within the window.
     * @param pos The new cursor position.
     */
    virtual void set_cursor_pos(const vec2<real> &pos) = 0;

    /**
     * @brief Alias for relocate(). Moves the window.
     * @param new_position The new position.
     */
    virtual void move(const vec2<int> &new_position) = 0;

    /**
     * @brief Sets user-associated data.
     * @param data Pointer to the data to associate with this window.
     */
    virtual void set_userdata(void *data) = 0;

    /**
     * @brief Sets the window title.
     * @param title The new title string.
     */
    virtual void set_title(const char *title) = 0;

    /**
     * @brief Sets the window icon.
     * @param width The width of the icon.
     * @param height The height of the icon.
     * @param pixels The pixel data (RGBA).
     */
    virtual void set_icon(int width, int height, const uint32_t *pixels) = 0;

    /**
     * @brief Sets the cursor icon (shape).
     * @param icon The requested standard cursor icon.
     */
    virtual void set_cursor_icon(mouse_icon icon) = 0;

    /**
     * @brief Sets the minimum and maximum size limits of the window.
     * @param min_size The minimum allowed size.
     * @param max_size The maximum allowed size.
     */
    virtual void set_limits(const vec2<int> &min_size,
                            const vec2<int> &max_size) = 0;

    /**
     * @brief Maximizes the window.
     */
    virtual void maximize() = 0;

    /**
     * @brief Minimizes the window.
     */
    virtual void minimize() = 0;

    /**
     * @brief Restores the window from minimized/maximized state.
     */
    virtual void restore() = 0;

    /**
     * @brief Hides the window.
     */
    virtual void hide() = 0;

    /**
     * @brief Shows the window.
     */
    virtual void show() = 0;

    /**
     * @brief Locks the cursor to the window (relative mode).
     */
    virtual void hold_cursor() = 0;

    /**
     * @brief Releases the cursor from relative mode.
     */
    virtual void release_cursor() = 0;

    /**
     * @brief Hides the cursor.
     */
    virtual void hide_cursor() = 0;

    /**
     * @brief Gets the monitor this window is currently on.
     * @return Pointer to the monitor, or nullptr if not available.
     */
    virtual monitor *get_monitor() = 0;

    /**
     * @brief Moves the window to a specific monitor (and usually sets
     * fullscreen).
     * @param mon The target monitor.
     */
    virtual void move_to_monitor(monitor *mon) = 0;

    /**
     * @brief Reads text from the system clipboard.
     * @return The clipboard text content as a string.
     */
    virtual const char *read_clipboard() = 0;

    /**
     * @brief Writes text to the system clipboard.
     * @param text The text to write.
     */
    virtual void write_clipboard(const char *text) = 0;

    /**
     * @brief Starts accepting text input events.
     * Use this when focusing a text field.
     */
    virtual void start_text_input() = 0;

    /**
     * @brief Stops accepting text input events.
     * Use this when a text field loses focus.
     */
    virtual void stop_text_input() = 0;

    using key_callback =
        void (*)(window *, key_code, int, button_state, modifier_keys);
    using text_input_callback   = void (*)(window *, const char *);
    using cursor_pos_callback   = void (*)(window *, real, real);
    using cursor_move_callback  = void (*)(window *, real, real, real, real);
    using cursor_enter_callback = void (*)(window *, bool);
    using mouse_button_callback = void (*)(window *,
                                           mouse_button,
                                           button_state,
                                           modifier_keys);
    using scroll_callback       = void (*)(window *, real, real);
    using drop_callback         = void (*)(window *, int, const char **);

    virtual void add_key_callback(key_callback cb)                      = 0;
    virtual void remove_key_callback(key_callback cb)                   = 0;
    virtual void add_text_input_callback(text_input_callback cb)        = 0;
    virtual void remove_text_input_callback(text_input_callback cb)     = 0;
    virtual void add_cursor_move_callback(cursor_move_callback cb)      = 0;
    virtual void remove_cursor_move_callback(cursor_move_callback cb)   = 0;
    virtual void add_cursor_enter_callback(cursor_enter_callback cb)    = 0;
    virtual void remove_cursor_enter_callback(cursor_enter_callback cb) = 0;
    virtual void add_mouse_button_callback(mouse_button_callback cb)    = 0;
    virtual void remove_mouse_button_callback(mouse_button_callback cb) = 0;
    virtual void add_scroll_callback(scroll_callback cb)                = 0;
    virtual void remove_scroll_callback(scroll_callback cb)             = 0;
    virtual void add_drop_callback(drop_callback cb)                    = 0;
    virtual void remove_drop_callback(drop_callback cb)                 = 0;
};

/**
 * @brief Initializes the windowing system (e.g., SDL2).
 * @return true on success, false on failure.
 */
bool init_window_system();

/**
 * @brief Terminates the windowing system.
 */
void terminate_window_system();

/**
 * @brief Creates a new window.
 *
 * @param x The x-coordinate (or SDL_WINDOWPOS_CENTERED/UNDEFINED logic).
 * @param y The y-coordinate.
 * @param width The width of the window.
 * @param height The height of the window.
 * @param title The window title.
 * @param flags Window creation flags.
 * @return A pointer to the created window, or nullptr on failure.
 */
window *create_window(int x,
                      int y,
                      int width,
                      int height,
                      const char *title,
                      window_flags flags = window_flags::none);

/**
 * @brief Polls for pending events.
 * This should be called in the main loop.
 */
void poll_events();

/**
 * @brief Waits until an event is received.
 * Blocks the thread.
 */
void wait_events();

/**
 * @brief Waits for an event with a timeout.
 * @param timeout The timeout in seconds.
 */
void wait_events_timeout(real timeout);

/**
 * @brief Gets the time since initialization.
 * @return Time in milliseconds.
 */
uint64_t get_time();

/**
 * @brief Gets the string name of a key code.
 * @param key The key code.
 * @return The key name.
 */
const char *get_key_name(key_code key);

/**
 * @brief Gets a list of available monitors.
 * @param count Output parameter for the number of monitors.
 * @return Array of monitor pointers.
 */
monitor **get_monitors(size_t *count);

/**
 * @brief Gets the current state of a gamepad.
 * @param jid The joystick ID.
 * @param state Output parameter for the state.
 * @return true if successful, false otherwise.
 */
bool get_gamepad_state(int jid, gamepad_state *state);

/**
 * @brief Gets the name of a gamepad.
 * @param jid The joystick ID.
 * @return The name string.
 */
const char *get_gamepad_name(int jid);

using gamepad_callback = void (*)(int, connect_event);
/**
 * @brief Sets the global gamepad connection callback.
 * @param cb The callback function.
 */
void set_gamepad_callback(gamepad_callback cb);

// OpenGL specific functions
using gl_proc = void (*)();

/**
 * @brief Gets the address of an OpenGL function.
 * @param procname The function name.
 * @return The function pointer.
 */
gl_proc get_proc_address(const char *procname);

/**
 * @brief Makes the window's OpenGL context current.
 * @param win The window.
 */
void make_context_current(window *win);

}; // namespace zabato