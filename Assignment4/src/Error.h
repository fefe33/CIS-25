#pragma once
#include <iostream>

void fError(const std::string& message) {
	cout << "\033[41m" << message << "\033[0m\n"
}
