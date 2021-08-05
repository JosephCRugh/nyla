#pragma once

#include "types_ext.h"
#include "token.h"
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
		TYPE_CHAR16,
		TYPE_STRING,
		TYPE_ERROR,

		TYPE_PTR_BYTE,
		TYPE_PTR_SHORT,
		TYPE_PTR_INT,
		TYPE_PTR_LONG,
		TYPE_PTR_FLOAT,
		TYPE_PTR_DOUBLE,
		TYPE_PTR_BOOL,
		TYPE_PTR_CHAR16,

	};

	struct type {
		type_tag tag;
		nyla::token* st = nullptr;
		nyla::token* et = nullptr;
		u32 ptr_depth = 0;
		
		type() {}
		type(type_tag _tag) : tag(_tag) {}
		type(type_tag _tag, u32 _ptr_depth)
			: tag(_tag), ptr_depth(_ptr_depth){}

		static nyla::type* get_byte();
		static nyla::type* get_short();
		static nyla::type* get_int();
		static nyla::type* get_long();
		static nyla::type* get_float();
		static nyla::type* get_double();
		static nyla::type* get_bool();
		static nyla::type* get_void();
		static nyla::type* get_string();
		static nyla::type* get_char16();
		static nyla::type* get_error();

		static nyla::type* get_ptr_byte(u32 depth);
		static nyla::type* get_ptr_short(u32 depth);
		static nyla::type* get_ptr_int(u32 depth);
		static nyla::type* get_ptr_long(u32 depth);
		static nyla::type* get_ptr_float(u32 depth);
		static nyla::type* get_ptr_double(u32 depth);
		static nyla::type* get_ptr_bool(u32 depth);
		static nyla::type* get_ptr_char16(u32 depth);

		nyla::type* as_ptr(u32 depth);

		friend std::ostream& operator<<(std::ostream& os, const nyla::type& type) {
			switch (type.tag) {
			case TYPE_PTR_BYTE:
			case TYPE_BYTE:     os << "byte"; break;
			case TYPE_PTR_SHORT:
			case TYPE_SHORT:    os << "short"; break;
			case TYPE_PTR_INT:
			case TYPE_INT:      os << "int"; break;
			case TYPE_PTR_LONG:
			case TYPE_LONG:     os << "long"; break;
			case TYPE_PTR_FLOAT:
			case TYPE_FLOAT:    os << "float"; break;
			case TYPE_PTR_DOUBLE:
			case TYPE_DOUBLE:   os << "double"; break;
			case TYPE_PTR_BOOL:
			case TYPE_BOOL:     os << "bool"; break;
			case TYPE_VOID:     os << "void"; break;
			case TYPE_STRING:   os << "String"; break;
			case TYPE_PTR_CHAR16:
			case TYPE_CHAR16:   os << "char16"; break;
			}
			for (u32 i = 0; i < type.ptr_depth; i++) {
				os << "*";
			}
			return os;
		}

		bool operator!=(const nyla::type& o) {
			return !(*this == o);
		}

		bool operator==(const nyla::type& o) {
			return tag == o.tag &&
				   ptr_depth == o.ptr_depth;
		}

		u32 get_mem_size();

		bool is_int();

		bool is_number();

		bool is_signed();

		bool is_float();

		bool is_ptr();

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
		extern nyla::type* string_type;
		extern nyla::type* char16_type;
		extern nyla::type* error_type;
	}

}