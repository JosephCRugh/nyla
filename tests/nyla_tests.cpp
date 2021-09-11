#include "compiler.h"

#include "test_suite.h"
#include "words.h"

#include <iostream>

void test_program(const std::string& sub_project, int test_error_code, u32 extra_flags = 0) {
	nyla::compiler compiler(nyla::COMPFLAGS_FULL_COMPILATION | extra_flags);
	std::vector<std::string> src_directories;
	src_directories.push_back("resources/" + sub_project);

	compiler.set_executable_name("nyla_test_project.exe");
	compiler.compile(src_directories);

	if (!compiler.get_found_compilation_errors()) {
		int return_code = system("nyla_test_project.exe");
		check_eq(return_code, test_error_code);
	}
	nyla::g_type_table->clear_table();
	nyla::g_word_table->clear_table();
}

int main(int argc, char* argv[]) {

	test_program("Arithmetic", [](){
		s32 b = 22;
		s32 sum = 44 * 3 + 55 - 421 * b;
		s32 g = 4;
		sum /= g + 1;
		sum *= 3 - 1;
		sum = sum * sum;
		return sum;
		}());
	test_program("MoreArithmetic", []() {
		s32 a = 22;
		s32 b = a << 6;
		s32 c = b % 20;
		return c + (312312 >> 3) ^ 7;
		}());
	test_program("Casts", []() {
		s8 b = 5;
		s16 c = b + 3;
		s32 a = c + b;
		double ddd = 21.0 + c / a;
		s32 j = ddd;
		return j;
		}());
	test_program("FuncCall", 54 + 7);
	test_program("LoopSum", []() {
		s32 sum = 0;
		for (int i = 0; i < 55; i++) {
			sum += i;
		}
		return sum;
		}());
	test_program("SimpleModule", 55 + 66);
	test_program("FunctionOverloading", 77 + 88);
	test_program("SimpleArray", []() {
		s32 arr[] = { 44, 22, 832, 1 };
		s32 sum = 0;
		for (s32 i = 0; i < 4; ++i) {
			sum += arr[i];
		}
		return sum;
		}());
	test_program("MultiDimArray", 412 + 21 + 5 + 6 + 4);
	test_program("ArrayPass", 3 + 6 + 4 + 7 + 8 + 4 + 8 + 4);
	test_program("StaticModuleCall", 631 + 8);
	test_program("StringArray", []() {
		char* myString = "Hello World!";
		s32 sum = 0;
		for (s32 i = 0; i < 12; i++) {
			sum += myString[i];
		}
		return sum * 3;
		}());

	return 0;
}