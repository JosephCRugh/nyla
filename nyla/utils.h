#ifndef NYLA_UTILS_H
#define NYLA_UTILS_H

#include "types_ext.h"

#include <string>
#include <tuple>
#include <vector>

namespace nyla {

	constexpr u32 console_color_default = 0x7;
	constexpr u32 console_color_red     = 0xC;
	constexpr u32 console_color_green   = 0x2;
	constexpr u32 console_color_yellow  = 0x6;

	struct search_file {
		std::string path;
		bool        is_directory;
	};

	// Get the operating system's running current time
	// in milliseconds (time since epoch)
	u64 get_time_in_milliseconds();

	// Sets the color for the console.
	void set_console_color(int color_id);

	// Get the files (including directory files) under the directory path.
	std::tuple<std::vector<search_file>, bool> get_directory_files(const std::string& path);

	// Splits the string into segments by the deilimiter
	std::vector<std::wstring> split(const std::wstring& str, wchar_t delimiter);

	// Splits the string into segments by the deilimiter
	std::vector<std::string> split(const std::string& str, char delimiter);

	// Replaces occurrences of the 'from' string with the 'to' string in the source
	std::string replace(const std::string& source, const std::string& from, const std::string& to);

	// Wide character string to non-wide character string
	std::string wstring_to_string(const std::wstring& wstr);

	// Non-wide character string to wide character string
	std::wstring string_to_wstring(const std::string& str);

	// Read a file into a character buffer 'data'
	bool read_file(const std::string& path, c8*& data, ulen& size);

	// Checks if the string ends with another string
	template<typename T>
	bool string_ends_with(const T& str, const T& ending) {
		if (str.length() < ending.length()) {
			return false;
		}
		return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
	}

	// Checks if the string starts with another string
	template<typename T>
	bool string_starts_with(const T& str, const T& start) {
		if (str.length() < start.length()) {
			return false;
		}
		return str.compare(0, start.length(), start) == 0;
	}
}

#endif