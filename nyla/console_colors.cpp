#include "console_colors.h"

#ifdef _WIN32
#include <Windows.h>
#endif

void set_console_color(int color) {
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
}