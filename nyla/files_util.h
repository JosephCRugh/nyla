
#include "types_ext.h"

#include <functional>
#include <string.h>
#include <xstring>

namespace nyla {

	/// <summary>
	/// Reads the files within a directory and provides
	/// a callback function to the file names
	/// </summary>
	/// <param name="path">The path to the directory to read</param>
	/// <param name="callback">A callback that provides the files in the directory</param>
	/// <returns>Successful read of directory path</returns>
	bool for_files(const std::wstring& path, std::function<void(std::string)> callback);
	bool read_file(const std::string& fpath, c8*& data, ulen& size);
}
