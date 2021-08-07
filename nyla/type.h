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

		TYPE_UBYTE,
		TYPE_USHORT,
		TYPE_UINT,
		TYPE_ULONG,

		TYPE_FLOAT,
		TYPE_DOUBLE,
		TYPE_BOOL,
		TYPE_VOID,
		TYPE_CHAR16,
		TYPE_STRING,
		TYPE_ERROR,

		TYPE_MIXED, // Mixed type is needed
		            // because array initialization
					// with {} can have mixed element
					// types at parse prior to analysis
					
		TYPE_PTR,
		TYPE_ARR,

	};

	struct aexpr;

	struct type {
		type_tag tag;
		type_tag elem_tag;
		
		u32                       ptr_depth = 0;
		std::vector<nyla::aexpr*> array_depths;

		nyla::token* st = nullptr;
		nyla::token* et = nullptr;

		type() {}
		type(type_tag _tag) : tag(_tag) {}
		type(type_tag _tag, u32 _ptr_depth)
			: tag(_tag), ptr_depth(_ptr_depth) {}
		type(type_tag _tag, type_tag _elem_tag, u32 _ptr_depth)
			: tag(_tag), elem_tag(_elem_tag), ptr_depth(_ptr_depth) {}

		static nyla::type* get_byte();
		static nyla::type* get_short();
		static nyla::type* get_int();
		static nyla::type* get_long();
		static nyla::type* get_ubyte();
		static nyla::type* get_ushort();
		static nyla::type* get_uint();
		static nyla::type* get_ulong();
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

		static nyla::type* get_arr_byte(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_short(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_int(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_long(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_float(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_double(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_bool(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_char16(std::vector<nyla::aexpr*>& array_depths);
		static nyla::type* get_arr_mixed(std::vector<nyla::aexpr*>& array_depths);

		static nyla::type* make_arr(type_tag tag, std::vector<nyla::aexpr*>& array_depths);

		nyla::type* as_ptr(u32 depth);

		nyla::type* as_arr(std::vector<nyla::aexpr*>& array_depths);

		nyla::type* get_element_type();
		
		friend std::ostream& operator<<(std::ostream& os, const nyla::type& type);

		void print_elem(std::ostream& os, type_tag elem_tag) const;

		bool operator!=(const nyla::type& o);

		bool operator==(const nyla::type& o);

		u32 get_mem_size();

		bool is_int();

		bool is_number();

		bool is_signed();

		bool is_float();

		bool is_ptr();

		bool is_arr();

	};

	namespace reuse_types {
		extern nyla::type* byte_type;
		extern nyla::type* short_type;
		extern nyla::type* int_type;
		extern nyla::type* long_type;
		extern nyla::type* ubyte_type;
		extern nyla::type* ushort_type;
		extern nyla::type* uint_type;
		extern nyla::type* ulong_type;
		extern nyla::type* float_type;
		extern nyla::type* double_type;
		extern nyla::type* bool_type;
		extern nyla::type* void_type;
		extern nyla::type* string_type;
		extern nyla::type* char16_type;
		extern nyla::type* error_type;
	}

}