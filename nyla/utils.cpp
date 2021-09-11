#include "utils.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <locale>
#include <codecvt>

#include <fstream>

#include <chrono>

u64 nyla::get_time_in_milliseconds() {
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
	using std::chrono::seconds;
	using std::chrono::system_clock;

	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void nyla::set_console_color(int color_id) {
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color_id);
#endif
}

std::tuple<std::vector<nyla::search_file>, bool> nyla::get_directory_files(const std::string& path) {
	std::vector<search_file> files;

#ifdef _WIN32
	std::string glob_path = path + "/*";
	WIN32_FIND_DATAA data;
	//LPWIN32_FIND_DATAA
	HANDLE f_handle = FindFirstFileA(glob_path.c_str(), &data);
	if (f_handle == INVALID_HANDLE_VALUE) {
		return std::tuple<std::vector<search_file>, bool>{ files, false };
	}
	do {
		if (!((data.cFileName[0] == '.' && data.cFileName[1] == '\0')
			|| data.cFileName[0] == '.' && data.cFileName[1] == '.' && data.cFileName[2] == '\0'
			)) {
			files.push_back(search_file{
			std::string(data.cFileName),
			(bool)(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				});
		}
	} while (FindNextFileA(f_handle, &data) != 0);

	FindClose(f_handle);
#endif

	return std::tuple<std::vector<search_file>, bool>{ files, true };
}

std::vector<std::wstring> nyla::split(const std::wstring& str, wchar_t delimiter) {
	std::vector<std::wstring> segments;
	std::wstring segment;
	for (wchar_t ch : str) {
		if (ch != delimiter) {
			segment += ch;
		} else {
			segments.push_back(segment);
			segment.clear();
		}
	}
	if (!segment.empty()) segments.push_back(segment);
	return segments;
}

std::vector<std::string> nyla::split(const std::string& str, char delimiter) {
	std::vector<std::string> segments;
	std::string segment;
	for (char ch : str) {
		if (ch != delimiter) {
			segment += ch;
		} else {
			segments.push_back(segment);
			segment.clear();
		}
	}
	if (!segment.empty()) segments.push_back(segment);
	return segments;
}

std::string nyla::replace(const std::string& source, const std::string& from, const std::string& to) {
	std::string result;
	result.reserve(source.length());
	ulen last_pos = 0;
	ulen find_pos;

	while ((find_pos = source.find(from, last_pos)) != std::string::npos) {
		result.append(source, last_pos, find_pos - last_pos);
		result.append(to);
		last_pos = find_pos + from.length();
	}

	result.append(source, last_pos, source.length() - last_pos);

	return result;
}

std::string nyla::wstring_to_string(const std::wstring& wstr) {
	using ct = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<ct, wchar_t> wconvertor;
	return wconvertor.to_bytes(wstr);
}

std::wstring nyla::string_to_wstring(const std::string& str) {
	using ct = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<ct, wchar_t> wconvertor;
	return wconvertor.from_bytes(str);
}

bool nyla::read_file(const std::string& path, c8*& data, ulen& size) {
	std::ifstream in(path, std::ios::binary | std::ios::in);
	if (!in.good()) {
		return false;
	}
	in.seekg(0, std::ios::end);
	size = in.tellg();
	data = new c8[size];
	in.seekg(0, std::ios::beg);
	in.read(data, size);
	return true;
}

