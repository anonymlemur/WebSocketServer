#include <iostream>
#include <thread>
#include <atomic>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
#include <Windows.h>

std::condition_variable cv;
std::mutex mutex;
std::atomic<bool> mouse_moving(false); // Variable to track mouse movement
std::thread move_thread;               // Thread for mouse movement

void move_mouse_realtime(int delta_x, int delta_y)
{
	// Get the current mouse position
	int current_x, current_y;
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	current_x = cursorPos.x;
	current_y = cursorPos.y;

	// Perform real-time mouse movement
	SetCursorPos(current_x + delta_x, current_y + delta_y);

	// Print the new mouse position (optional)
	std::cout << "Mouse moved: x=" << current_x + delta_x << ", y=" << current_y + delta_y << std::endl;

	// Notify that mouse movement is done
	mouse_moving = false;
}

void move_mouse(int delta_x, int delta_y)
{
	// Set the flag to indicate mouse movement
	mouse_moving = true;

	// Start the real-time mouse movement in a separate thread
	move_thread = std::thread(move_mouse_realtime, delta_x, delta_y);
	move_thread.detach(); // Detach the thread to let it run independently
}

void stop_mouse()
{
	// Set the flag to false to stop mouse movement
	mouse_moving = false;

	// Notify the move_mouse function to stop immediately
	cv.notify_one();

	// Wait for the move_thread to finish before returning
	if (move_thread.joinable())
	{
		move_thread.join();
	}
}

void right_click_mouse()
{
	std::cout << "Right clicking mouse" << std::endl;

	// Prepare the INPUT structure for the mouse click
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; // Press the left mouse button

	// Send the mouse down event
	SendInput(1, &input, sizeof(INPUT));

	// Release the left mouse button
	input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	SendInput(1, &input, sizeof(INPUT));
}

void left_click_mouse()
{
	std::cout << "Left clicking mouse" << std::endl;

	// Prepare the INPUT structure for the mouse click
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; // Press the left mouse button

	// Send the mouse down event
	SendInput(1, &input, sizeof(INPUT));

	// Release the left mouse button
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(INPUT));
	;
}

void test()
{
	std::cout << "test" << std::endl;
}

// define keybord function that takes a string and presses the corresponding key
void keyboard(std::string key)
{
	if (key.empty())
	{
		return; // Do nothing if the key string is empty
	}

	// Convert the string to a single character (assuming only one character is passed)
	char c = key[0];

	// Prepare the INPUT structure for the key press
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = 0;                      // We'll use scan code instead of a virtual key
	input.ki.wScan = static_cast<WORD>(c); // Convert the character to scan code
	input.ki.dwFlags = KEYEVENTF_SCANCODE; // Use scan code instead of virtual key

	// Send the key press event
	SendInput(1, &input, sizeof(INPUT));

	// Release the key
	input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
}



// Function to process and dispatch commands
void process_command(const std::map<std::string, std::function<void(const std::string&)>>& command_mapping, const std::string& data)
{
	// Split the data and get the command and parameters 
	size_t pos = data.find(":");
	std::string command = data.substr(0, pos);
	std::string params = data.substr(pos + 1);

	// Call the corresponding function based on the command
	auto it = command_mapping.find(command);
	if (it != command_mapping.end())
	{
		it->second(params); // Call the function with the parameters
	}
}

int main()
{
	std::cout << "Server is listening on 0.0.0.0:12345" << std::endl;

	// Create the WebSocket server
	using server = websocketpp::server<websocketpp::config::asio>;
	server websocket_server;

	// Define the command-function map
	std::map<std::string, std::function<void(const std::string&)>> command_mapping;

	// Add command-function pairs to the map using lambda functions
	command_mapping.insert({ "MOVE", [](const std::string& params)
							{
			// Parse parameters (comma-separated delta_x and delta_y)
			std::istringstream iss(params);
			int delta_x, delta_y;
			char delimiter;
			iss >> delta_x >> delimiter >> delta_y;
			if (delimiter == ',')
			{
				move_mouse(delta_x, delta_y);
			}
			else
			{
				std::cerr << "Invalid MOVE command parameters" << std::endl;
			}
		} });

	command_mapping.insert({ "KEYBOARD", [](const std::string& params)
							{
			// Check if parameters are not empty
			if (!params.empty()) {
				// Pass the parameter to the keyboard function
				keyboard(params);
			}
			else {
				std::cerr << "Invalid KEYBOARD command parameters" << std::endl;
			} } });

	command_mapping.insert({ "STOP_MOVE", [](const std::string&)
							{ stop_mouse(); } });

	command_mapping.insert({ "LEFT_CLICK", [](const std::string&)
							{ left_click_mouse(); } });

	command_mapping.insert({ "RIGHT_CLICK", [](const std::string&)
							{ right_click_mouse(); } });

	command_mapping.insert({ "TEST", [](const std::string&)
							{ test(); } });

	// ... (Other command mappings)

	// Set up the WebSocket server handlers
	websocket_server.set_message_handler([&command_mapping](websocketpp::connection_hdl hdl, server::message_ptr msg)
		{
			std::string data = msg->get_payload();
			std::cout << "Received: " << data << std::endl;

			// Process and dispatch the command
			process_command(command_mapping, data); });

	// Start the WebSocket server on port 12345
	websocket_server.init_asio();
	websocket_server.listen(12345);
	websocket_server.start_accept();

	// Start the main event loop
	websocket_server.run();

	return 0;
}
