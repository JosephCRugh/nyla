#include "compiler.h"
#include "utils.h"

#include <iostream>

const char* usage =
R"(Usage: nylac <options> !entry=<file> <source directories>
Possible Options:
  -name=<name>
      Sets the name of the generated executable
  -display.llvm.ir
      Displays generated LLVM IR
  -display.stages
      Displays the stages for the files
  -display.times
      Displays how long different stages took
)";

int main(int argc, char* argv[]) {

	nyla::compiler compiler;

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

	u32 flags = nyla::COMPFLAGS_FULL_COMPILATION;
	for (const std::string& option : options) {
		if (nyla::string_starts_with(option, std::string("name="))) {
			std::string exe_name = option.substr(option.find('=') + 1);
			compiler.set_executable_name(exe_name);
		} else if (option == "display.llvm.ir") {
			flags |= nyla::COMPFLAG_DISPLAY_LLVM_IR;
		} else if (option == "display.stages") {
			flags |= nyla::COMPFLAG_DISPLAY_STAGES;
		} else if (option == "display.times") {
			flags |= nyla::COMPFLAG_DISPLAY_TIMES;
		} else {
			std::cout << "Unknown option: " << option << '\n';
			return 1;
		}
	}
	compiler.set_flags(flags);

	std::string file_with_main;
	if (options_count < argc) {
		std::string main_arg = std::string(argv[options_count]);
		if (nyla::string_starts_with(main_arg, std::string("!entry="))) {
			file_with_main = main_arg.substr(main_arg.find('=') + 1);
		} else {
			std::cout << usage;
			return 1;
		}
		++options_count;
	} else {
		std::cout << usage;
		return 1;
	}

	for (int i = options_count; i < argc; i++) {
		src_directories.push_back(argv[i]);
	}

	if (src_directories.empty()) {
		std::cout << usage;
		return 1;
	}

	compiler.compile(src_directories, file_with_main);
	
	return 0;
}