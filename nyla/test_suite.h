#pragma once

#include "types_ext.h"
#include "console_colors.h"

#include <iostream>

#define test_header_length      46
#define test_half_header_length (test_header_length >> 1)

void print_check_heading(const char* msg) {
	set_console_color(test_color_yellow);
	std::cout << msg;
	set_console_color(test_color_default);
}

void print_success_state(bool tof) {
	if (tof) {
		set_console_color(test_color_green);
		std::cout << " (OK)";
	} else {
		set_console_color(test_color_red);
		std::cout << " (FAIL)";
	}
	set_console_color(test_color_default);
}

template<typename T, typename V>
bool check_eq(T a, V b, c_string extra_info, typename std::enable_if<!std::is_pointer<T>::value>::type* = 0) {
	print_check_heading("check_eq: ");
	if (strlen(extra_info) > 0)
		std::cout << "|" << extra_info << "| ";
	std::cout << a << " == " << b;
	bool tof = a == b;
	print_success_state(tof);
	std::cout << std::endl;
	return tof;
}

template<typename T, typename V>
bool check_eq(T a, V b, c_string extra_info, typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
	print_check_heading("check_eq: ");
	if (strlen(extra_info) > 0)
		std::cout << "|" << extra_info << "| ";
	std::cout << a << " == " << b;
	bool tof = *a == *b;
	print_success_state(tof);
	std::cout << std::endl;
	return tof;
}

template<typename T, typename V>
bool check_neq(T a, V b, c_string extra_info, typename std::enable_if<!std::is_pointer<T>::value>::type* = 0) {
	print_check_heading("check_neq: ");
	if (strlen(extra_info) > 0)
		std::cout << "|" << extra_info << "| ";
	std::cout << a << " != " << b;
	bool tof = a != b;
	print_success_state(tof);
	std::cout << std::endl;
	return tof;
}

template<typename T, typename V>
bool check_neq(T a, V b, c_string extra_info, typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
	print_check_heading("check_neq: ");
	if (strlen(extra_info) > 0)
		std::cout << "|" << extra_info << "| ";
	std::cout << *a << " != " << *b;
	bool tof = *a != *b;
	print_success_state(tof);
	std::cout << std::endl;
	return tof;
}

template<typename T, typename V>
bool check_eq(T a, V b) {
	return check_eq<T, V>(a, b, "");
}

template<typename T, typename V>
bool check_neq(T a, V b) {
	return check_neq<T, V>(a, b, "");
}

bool check_tof(bool tof, c_string extra_info) {
	print_check_heading("check_tof: ");
	if (strlen(extra_info) > 0)
		std::cout << "|" << extra_info << "| ";
	std::cout << (tof ? "true" : "false");
	print_success_state(tof);
	std::cout << std::endl;
	return tof;
}


void test(c_string name) {
	ulen name_len = strlen(name);
	if (name_len % 2 != 0) {
		std::string side = std::string(test_half_header_length - ((name_len + 1) >> 1), '-');
		std::cout << side << "  " << name << " " << side;
	}
	else {
		std::string side = std::string(test_half_header_length - (name_len >> 1), '-');
		std::cout << side << " " << name << " " << side;
	}
	std::cout << std::endl;
}
