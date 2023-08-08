// InputSimulator.h
#pragma once
#include <string>
// Function to simulate key press
void press_key(const std::string& keys);

// Functions to simulate mouse actions
void move_mouse(int delta_x, int delta_y);
void stop_mouse();
void right_click_mouse();
void left_click_mouse();
