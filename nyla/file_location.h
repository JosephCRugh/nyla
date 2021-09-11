#ifndef NYLA_FILE_LOCATION
#define NYLA_FILE_LOCATION

namespace nyla {
	/*
	 * Structure for representing
	 * loaded .nyla files.
	 */
	struct file_location {
		// "src/std/a.nyla"   << full path relative to the compiler
		std::string system_path;
		// "std/a"            << Path relative to source folder with .nyla stripped
		std::string internal_path;
	};
}

#endif