#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <iostream>
#include <sstream>
#include "InputSimulator.h"
#include <fstream>
#include <iostream>
#define ID_TRAY_EXIT            1001
#define ID_TRAY_SHOW_DEVICES    1002
#define ID_TRAY_CONNECT_DEVICE  1003
#define ID_TRAY_TOGGLE_DEBUG	1004 



// Global Constants & Variables
constexpr uint16_t SERVER_PORT = 12345;
constexpr UINT WM_APP_TRAYMSG = WM_APP + 1;
std::atomic<bool> g_mouseMoving(false);
std::atomic<bool> g_serverRunning(true);
std::atomic<bool> g_debugMode(true);

// Utility Functions for Mouse and Keyboard Control
void moveMouseRealtime(int deltaX, int deltaY);
void moveMouse(int deltaX, int deltaY);
void stopMouse();
void keyboard(const std::string& key);
void rightClickMouse();
void leftClickMouse();
void pressKey(const std::string& key);

// WebSocket Server Functions
void WebSocketServerFunction();

// Win32 Functions
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitTray(HWND);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	const wchar_t CLASS_NAME[] = L"TrayAppClass";
	std::ofstream logFile("log.txt"); // Open a log file
	std::streambuf* coutbuf = std::cout.rdbuf(); // Save old buf
	std::cout.rdbuf(logFile.rdbuf()); // Redirect std::cout to log.txt

	std::streambuf* cerrbuf = std::cerr.rdbuf(); // Save old buf of std::cerr
	std::cerr.rdbuf(logFile.rdbuf()); // Redirect std::cerr to log.txt as well

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hWnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"TrayApp",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL
	);

	if (hWnd == NULL)
	{
		return 0;
	}

	InitTray(hWnd);

	std::thread serverThread(WebSocketServerFunction);
	serverThread.detach();

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	std::cout.rdbuf(coutbuf);
	std::cerr.rdbuf(cerrbuf);
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_DESTROY:
		g_serverRunning = false;
		{
			NOTIFYICONDATA nid = {};
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hWnd;
			Shell_NotifyIcon(NIM_DELETE, &nid);
		}
		PostQuitMessage(0);
		return 0;

	case WM_APP_TRAYMSG:
		switch (lParam) {
		case WM_LBUTTONUP:
			MessageBox(hWnd, L"Tray icon clicked!", L"Notification", MB_OK);
			break;
		case WM_RBUTTONUP: {
			HMENU hMenu = CreatePopupMenu();
			if (hMenu) {
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Close");
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_SHOW_DEVICES, L"Show Connected Devices");
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_CONNECT_DEVICE, L"Connect New Device");
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_TOGGLE_DEBUG, L"Toggle Debug Mode"); // Added line

				POINT pt;
				GetCursorPos(&pt);
				SetForegroundWindow(hWnd);
				TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
				DestroyMenu(hMenu);
			}
			break;
		}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_TRAY_EXIT:
			DestroyWindow(hWnd);
			break;

		case ID_TRAY_SHOW_DEVICES:
			MessageBox(hWnd, L"Connected Devices:\nDevice 1\nDevice 2", L"Connected Devices", MB_OK);
			break;

		case ID_TRAY_CONNECT_DEVICE:
			MessageBox(hWnd, L"Connecting to new device...", L"Connect New Device", MB_OK);
			break;

		case ID_TRAY_TOGGLE_DEBUG:
			g_debugMode = !g_debugMode;
			MessageBox(hWnd, g_debugMode ? L"Debug mode enabled" : L"Debug mode disabled", L"Debug", MB_OK);
			break;
		}

		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}




void InitTray(HWND hWnd) {
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_APP_TRAYMSG;
	nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcscpy_s(nid.szTip, L"Tray App Example");

	Shell_NotifyIcon(NIM_ADD, &nid);
}


void moveMouseRealtime(int deltaX, int deltaY) {
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	SetCursorPos(cursorPos.x + deltaX, cursorPos.y + deltaY);
	std::cout << "Mouse moved: x=" << cursorPos.x + deltaX << ", y=" << cursorPos.y + deltaY << std::endl;
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
}

void keyboard(const std::string& key) {
	if (key.empty()) return;
	press_key(key);
}

void rightClickMouse() {
	right_click_mouse();
}

void leftClickMouse() {
	left_click_mouse();
}

void pressKey(const std::string& key) {
	press_key(key);
}

void WebSocketServerFunction() {
	using Server = websocketpp::server<websocketpp::config::asio>;
	Server websocketServer;

	// Store all connection handles
	std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections;

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
		{ "LEFT_CLICK", [](const std::string&) { left_click_mouse(); }},
		{ "RIGHT_CLICK", [](const std::string&) { right_click_mouse(); }},
		// ... Add other commands as necessary
	};

	// Open Handler
	websocketServer.set_open_handler([&connections](websocketpp::connection_hdl hdl) {
		connections.insert(hdl);
		std::cout << "Client connected!" << std::endl;
		});

	// Close Handler
	websocketServer.set_close_handler([&connections](websocketpp::connection_hdl hdl) {
		connections.erase(hdl);
		});

	// Message Handler
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

	// Print Listening Address and Port
	constexpr uint16_t SERVER_PORT = 12345;
	std::cout << "Server is listening on 0.0.0.0:" << SERVER_PORT << std::endl;

	// Initialize and Start Listening
	websocketServer.init_asio();
	websocketServer.listen(SERVER_PORT);
	websocketServer.start_accept();

	while (g_serverRunning) {
		websocketServer.run_one();
	}

	// Stop listening and Close all active connections
	websocketServer.stop_listening();
	for (const auto& hdl : connections) {
		websocketServer.close(hdl, websocketpp::close::status::going_away, "Server Shutdown");
	}
}
