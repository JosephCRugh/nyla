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

		TYPE_NULL

	};

	struct aexpr;

	struct type {
		type_tag     tag;
		nyla::type*  elem_type;
		// Arrays have sizes for their dimensions
		nyla::aexpr* dim_size;
		u32          arr_depth = 0; // how many [] subscripts exist
		u32          ptr_depth = 0; // how many * subscripts exist

		nyla::token* st = nullptr;
		nyla::token* et = nullptr;

		type() {}
		type(type_tag _tag) : tag(_tag) {}
		type(type_tag _tag, nyla::type* _elem_type)
			: tag(_tag), elem_type(_elem_type) {}
		type(type_tag _tag, nyla::type* _elem_type, nyla::aexpr* _dim_size)
			: tag(_tag), elem_type(_elem_type), dim_size(_dim_size) {}

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
		static nyla::type* get_mixed();
		static nyla::type* get_error();
		static nyla::type* get_null();

		static nyla::type* get_arr(nyla::type* elem_type, nyla::aexpr* dim_size);

		static nyla::type* get_ptr(nyla::type* elem_type);

		void calculate_arr_depth();

		void calculate_ptr_depth();

		nyla::type* get_array_at_depth(u32 req_depth, u32 depth = 0);

		nyla::type* get_array_base_type();

		nyla::type* get_ptr_base_type();
		
		void set_array_base_type(nyla::type* type);

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
		extern nyla::type* mixed_type;
		extern nyla::type* null_type;
	}

}