#include "files_util.h"


#include <locale>
#include <codecvt>
#include <fstream>

#include <iostream>

#ifdef _WIN32
#include<Windows.h>
#endif

bool nyla::for_files(const std::wstring& path, std::function<void(std::string)> callback) {
#ifdef _WIN32
	WIN32_FIND_DATA data;
	HANDLE f_handle = FindFirstFile(path.c_str(), &data);
	if (f_handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	do {
		using ct = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<ct, wchar_t> wconvertor;
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::string file_path = wconvertor.to_bytes(data.cFileName);
			callback(file_path);
		}
	} while (FindNextFile(f_handle, &data) != 0);

	FindClose(f_handle);
#endif
	return true;
}

bool nyla::read_file(const std::string& fpath, c8*& data, ulen& size) {
	std::ifstream in(fpath, std::ios::binary | std::ios::in);
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
