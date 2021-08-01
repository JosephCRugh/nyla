
#include "types_ext.h"

#include <functional>
#include <string.h>
#include <xstring>

namespace nyla {
	bool for_files(const std::wstring& path, std::function<void(std::string)> callback);
	void read_file(const std::string& fpath, c8*& data);
}
