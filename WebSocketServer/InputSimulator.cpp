#include "InputSimulator.h"
#ifdef _WIN32
#include <Windows.h>
#include <iostream>
// Windows-specific implementations

void press_key(const std::string& keys)
{
    std::cout << "Pressing keys (Windows): " << keys << std::endl;

    for (char c : keys) {
        // Convert character to virtual key code
        SHORT vk = VkKeyScan(c);

        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;

        SendInput(1, &input, sizeof(INPUT));       // Key down
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));       // Key up
    }
}

void right_click_mouse()
{
    std::cout << "Right clicking mouse (Windows)" << std::endl;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; // Press the right mouse button

    SendInput(1, &input, sizeof(INPUT));      // Send the mouse down event

    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;   // Release the right mouse button
    SendInput(1, &input, sizeof(INPUT));      // Send the mouse up event
}

void left_click_mouse()
{
    std::cout << "Left clicking mouse (Windows)" << std::endl;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;  // Press the left mouse button

    SendInput(1, &input, sizeof(INPUT));      // Send the mouse down event

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;    // Release the left mouse button
    SendInput(1, &input, sizeof(INPUT));      // Send the mouse up event
}


// ... other Windows-specific function implementations

#else
// Linux-specific implementations

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>

void press_key(const std::string& keys)
{
    std::cout << "Pressing keys (Linux): " << keys << std::endl;

    Display* display = XOpenDisplay(NULL);

    for (char c : keys) {
        // Convert character to keycode
        KeyCode kc = XKeysymToKeycode(display, XStringToKeysym(&c));

        XTestFakeKeyEvent(display, kc, True, 0);   // Key down
        XTestFakeKeyEvent(display, kc, False, 0);  // Key up
    }

    XFlush(display);
    XCloseDisplay(display);
}

void right_click_mouse()
{
    std::cout << "Right clicking mouse (Linux)" << std::endl;

    Display* display = XOpenDisplay(NULL);  // Open the default display

    XTestFakeButtonEvent(display, 3, True, 0);   // Press the right mouse button
    XTestFakeButtonEvent(display, 3, False, 0);  // Release the right mouse button

    XFlush(display);    // Ensure all commands are sent to the X server
    XCloseDisplay(display);
}

void left_click_mouse()
{
    std::cout << "Left clicking mouse (Linux)" << std::endl;

    Display* display = XOpenDisplay(NULL);  // Open the default display

    XTestFakeButtonEvent(display, 1, True, 0);   // Press the left mouse button
    XTestFakeButtonEvent(display, 1, False, 0);  // Release the left mouse button

    XFlush(display);    // Ensure all commands are sent to the X server
    XCloseDisplay(display);
}


// ... other Linux-specific function implementations

#endif
