#include "InputSimulator.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <map>
#include <functional>
#include <sstream>
#include <condition_variable>
#include <mutex>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#ifdef _WIN32
#include <Windows.h>
#else
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#endif


// ----- Constants & Global Variables -----
constexpr uint16_t SERVER_PORT = 12345;
std::atomic<bool> g_mouseMoving(false);

// ----- Utility Functions -----
#ifdef _WIN32

void moveMouseRealtime(int deltaX, int deltaY) {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    SetCursorPos(cursorPos.x + deltaX, cursorPos.y + deltaY);
    std::cout << "Mouse moved: x=" << cursorPos.x + deltaX << ", y=" << cursorPos.y + deltaY << std::endl;
}

#else

void moveMouseRealtime(int deltaX, int deltaY) {
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Failed to open X display." << std::endl;
        return;
    }
    Window root = DefaultRootWindow(display);
    XTestFakeMotionEvent(display, -1, deltaX, deltaY, 0);
    XFlush(display);
    XCloseDisplay(display);
    std::cout << "Mouse moved: delta_x=" << deltaX << ", delta_y=" << deltaY << std::endl;
}
#endif
void rightClickMouse() {
    right_click_mouse();
}

void leftClickMouse() {
    // Implement left-clicking for Windows here...
    left_click_mouse();
}

void pressKey(const std::string& key) {
    press_key(key);
}

void moveMouse(int deltaX, int deltaY) {
    g_mouseMoving = true;
    std::thread([=]() {
        moveMouseRealtime(deltaX, deltaY);
        g_mouseMoving = false;
        }).detach();
}

void stopMouse() {
    g_mouseMoving = false;
    // Add any other necessary cleanup for the moveMouse function.
}

void keyboard(const std::string& key) {
    if (key.empty()) return;
    pressKey(key);
}


// ----- Main Server Logic -----
int main() {
    using Server = websocketpp::server<websocketpp::config::asio>;
    Server websocketServer;

    // Define command-function mapping
    using CommandFunction = std::function<void(const std::string&)>;
    std::map<std::string, CommandFunction> commandMapping = {
        { "MOVE", [](const std::string& params) {
            int deltaX, deltaY;
            char delimiter;
            std::istringstream iss(params);
            iss >> deltaX >> delimiter >> deltaY;
            if (delimiter == ',') moveMouse(deltaX, deltaY);
            else std::cerr << "Invalid MOVE command parameters" << std::endl;
        }},
        { "KEYBOARD", [](const std::string& params) {
            if (!params.empty()) keyboard(params);
            else std::cerr << "Invalid KEYBOARD command parameters" << std::endl;
        }},
        { "STOP_MOVE", [](const std::string&) { stopMouse(); }},
        { "LEFT_CLICK", [](const std::string&) { leftClickMouse(); }},
        { "RIGHT_CLICK", [](const std::string&) { rightClickMouse(); }},
        // ... Add other commands as necessary
    };

    // Setup WebSocket server handlers
    websocketServer.set_message_handler([&commandMapping](websocketpp::connection_hdl hdl, Server::message_ptr msg) {
        std::string data = msg->get_payload();
        size_t pos = data.find(":");
        if (pos == std::string::npos) {
            std::cerr << "Invalid command format received: " << data << std::endl;
            return;
        }
        std::string command = data.substr(0, pos);
        std::string params = data.substr(pos + 1);
        auto it = commandMapping.find(command);
        if (it != commandMapping.end()) it->second(params);
        else std::cerr << "Unknown command: " << command << std::endl;
        });

    std::cout << "Server is listening on 0.0.0.0:" << SERVER_PORT << std::endl;
    websocketServer.init_asio();
    websocketServer.listen(SERVER_PORT);
    websocketServer.start_accept();
    websocketServer.run();

    return 0;
}
