#include "compiler.h"
#include "utils.h"

#include <iostream>

const char* usage =
R"(Usage: nylac <options> <source directories>
Possible Options:
  -name=<name>
      Sets the name of the generated executable
)";

int main(int argc, char* argv[]) {

	nyla::compiler compiler(nyla::COMPFLAGS_FULL_COMPILATION);

	std::vector<std::string> src_directories;
	std::vector<std::string> options;
	int options_count = 1;
	while (options_count < argc) {
		if (argv[options_count][0] == '-') {
			options.push_back(std::string(argv[options_count]).substr(1));
			++options_count;
		} else {
			break;
		}
	}

	for (const std::string& option : options) {
		if (nyla::string_starts_with(option, std::string("name="))) {
			std::string exe_name = option.substr(option.find('=') + 1);
			compiler.set_executable_name(exe_name);
		} else {
			std::cout << "Unknown option: " << option << '\n';
			return 1;
		}
	}

	for (int i = options_count; i < argc; i++) {
		src_directories.push_back(argv[i]);
	}

	if (src_directories.empty()) {
		std::cout << usage;
		return 1;
	}

	compiler.compile(src_directories);
	
	return 0;
}