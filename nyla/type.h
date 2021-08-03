#pragma once

#include "types_ext.h"
#include <iostream>

namespace nyla {

	enum type_tag {

		TYPE_BYTE,
		TYPE_SHORT,
		TYPE_INT,
		TYPE_LONG,
		TYPE_FLOAT,
		TYPE_DOUBLE,
		TYPE_BOOL,
		TYPE_VOID,
		TYPE_ERROR

	};

	struct type {
		type_tag tag;
		
		type() {}
		type(type_tag _tag) : tag(_tag) {}

		static nyla::type* get_byte();
		static nyla::type* get_short();
		static nyla::type* get_int();
		static nyla::type* get_long();
		static nyla::type* get_float();
		static nyla::type* get_double();
		static nyla::type* get_bool();
		static nyla::type* get_void();
		static nyla::type* get_error();

		friend std::ostream& operator<<(std::ostream& os, const nyla::type& type) {
			switch (type.tag) {
			case TYPE_BYTE: os << "byte"; break;
			case TYPE_SHORT: os << "short"; break;
			case TYPE_INT: os << "int"; break;
			case TYPE_LONG: os << "long"; break;
			case TYPE_FLOAT: os << "flaot"; break;
			case TYPE_DOUBLE: os << "double"; break;
			case TYPE_BOOL: os << "bool"; break;
			case TYPE_VOID: os << "void"; break;
			}
			return os;
		}

		bool operator!=(const nyla::type* o) {
			return !(*this == o);
		}

		bool operator==(const nyla::type* o) {
			return tag == o->tag;
		}

		u32 get_mem_size();

		bool is_int();

		bool is_number();

		bool is_signed();

		bool is_float();

	};

	namespace reuse_types {
		extern nyla::type* byte_type;
		extern nyla::type* short_type;
		extern nyla::type* int_type;
		extern nyla::type* long_type;
		extern nyla::type* float_type;
		extern nyla::type* double_type;
		extern nyla::type* bool_type;
		extern nyla::type* void_type;
		extern nyla::type* error_type;
	}

}