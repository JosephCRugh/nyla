#ifndef NYLA_TYPE_H
#define NYLA_TYPE_H

#include "types_ext.h"

#include <assert.h>
#include <unordered_map>

namespace nyla {

	// Maximum subscripts for * and []
	static constexpr u32 MAX_SUBSCRIPTS = 8;

	enum type_tag {

		// Integers
		TYPE_BYTE = 0,
		TYPE_SHORT,
		TYPE_INT,
		TYPE_LONG,

		TYPE_UBYTE,
		TYPE_USHORT,
		TYPE_UINT,
		TYPE_ULONG,

		// Characters
		TYPE_CHAR8,
		TYPE_CHAR16,
		TYPE_CHAR32,

		// Floats
		TYPE_FLOAT,
		TYPE_DOUBLE,

		// Other
		TYPE_BOOL,
		TYPE_VOID,
		TYPE_ERROR,
		TYPE_NULL,

		TYPE_PTR,
		TYPE_ARR,

		TYPE_MIXED, // Mixed type is needed
					// because array initialization
					// with {} can have mixed element
					// types during parsing

		TYPE_MODULE,

		// Forward declared modules are
		// temporary types used until the
		// module type can be resolved and
		// replace it
		TYPE_FD_MODULE,

		TYPE_STRING,

	};

	struct aexpr;
	struct sym_module;
	struct type_table;
	struct type;

	namespace types {
		// Integers
		extern nyla::type* type_byte;
		extern nyla::type* type_short;
		extern nyla::type* type_int;
		extern nyla::type* type_long;
		extern nyla::type* type_ubyte;
		extern nyla::type* type_ushort;
		extern nyla::type* type_uint;
		extern nyla::type* type_ulong;
		// Characters
		extern nyla::type* type_char8;
		extern nyla::type* type_char16;
		extern nyla::type* type_char32;
		// Floats
		extern nyla::type* type_float;
		extern nyla::type* type_double;
		// Other
		extern nyla::type* type_bool;
		extern nyla::type* type_void;
		extern nyla::type* type_error;
		extern nyla::type* type_string;
		extern nyla::type* type_null;
		extern nyla::type* type_mixed;

	}

	struct type {
		type_tag tag;
		nyla::type*  element_type       = nullptr;
		u32          ptr_depth          = 0;
		u32          arr_depth          = 0;
		// Part of TYPE_MODULE
		u32          unique_module_key  = 0;
		sym_module*  sym_module         = nullptr;
		// Part of TYPE_FD_MODULE
		u32          fd_module_name_key = 0;

		type(type_tag _tag) : tag(_tag) {}
		type(type_tag _tag, nyla::type* _element_type)
			: tag(_tag), element_type(_element_type) {  }

		struct hash_gen {
			ulen operator()(const nyla::type& type) const {
				switch (type.tag) {
				case TYPE_PTR: {
					// TODO: could probably do with a better hash
					// method
					ulen hash = type.tag + type.ptr_depth;
					hash ^= type.get_base_type()->tag;
					return hash;
				}
				case TYPE_ARR: {
					// TODO: could probably do with a better hash
					// method
					ulen hash = type.tag + type.arr_depth;
					hash ^= type.get_base_type()->tag;
					return hash;
				}
				case TYPE_MODULE: {
					return type.tag + type.unique_module_key;
				}
				case TYPE_FD_MODULE: {
					return type.tag + type.fd_module_name_key;
				}
				default:
					return type.tag;
				}
			}
		};

		bool operator==(const nyla::type& o) const {
			if (tag != o.tag) return false;
			switch (tag) {
			case TYPE_PTR: {
				return ptr_depth == o.ptr_depth &&
					   get_base_type() == o.get_base_type();
			}
			case TYPE_ARR: {
				if (arr_depth != o.arr_depth) return false;
				return arr_depth == o.arr_depth &&
					   get_base_type() == o.get_base_type();
			}
			case TYPE_MODULE: {
				return unique_module_key == o.unique_module_key;
			}
			case TYPE_FD_MODULE: {
				return fd_module_name_key == o.fd_module_name_key;
			}
			default: return true;
			}
		}

		// Compares two types. Works for all types
		// where-as the '==' only works on some
		bool equals(const nyla::type* o) const;

		std::string to_string() const;

		// Recursively calculates the number of pointer '*'
		// subscripts for the pointer and if the element_type is
		// a pointer then it's pointer depth.
		void calculate_ptr_depth();

		// Recursively calculates the number of pointer '[' ']'
		// subscripts for the pointer and if the element_type is
		// an array then it's array depth.
		void calculate_arr_depth();

		// Retreive an integer type based on its size in bytes
		// and if it is signed
		static nyla::type* get_int(u32 mem_size, bool is_signed);

		// Retreive a float type based on its size in bytes
		static nyla::type* get_float(u32 mem_size);

		// Retreive a char type based on its size in bytes
		static nyla::type* get_char(u32 mem_size);

		// Get a pointer to that to the type of element_type.
		// element_type may be another pointer causes pointers
		// to pointers
		static nyla::type* get_ptr(nyla::type* element_type);

		// Get an array with element's of type element_type
		static nyla::type* get_arr(nyla::type* element_type);

		// Get/Enter a new module type based on the module's unique key accross
		// the entire program
		static nyla::type* get_or_enter_module(nyla::sym_module* sym_module);

		// Get a forward declared type based on the module's name key
		// and it's internal path
		static nyla::type* get_fd_module(nyla::type_table* local_type_table, u32 module_name_key);

		// Converts a forward declared type into a module type
		void resolve_fd_type(nyla::sym_module* sym_module);

		// Base type for pointers and arrays or returns
		nyla::type* get_base_type() const;
		nyla::type* get_arr_base_type() const;
		nyla::type* get_ptr_base_type() const;

		// Sets the base type for pointers and arrays
		void set_base_type(nyla::type* base_type);

		// 
		nyla::type* get_sub_array(u32 depth, u32 depth_count = 0);

		bool is_number();

		bool is_int();

		bool is_float();

		bool is_signed();

		bool is_char();

		bool is_ptr();

		bool is_arr();

		bool is_module();

		// Get the memory size in bytes
		u32 mem_size();

	};

	struct type_info {
		nyla::type* type = nullptr;
		// Optional in case of arrays.
		// Although parsed with types the dimension sizes
		// are not actually part of the type. They are part
		// of the variable to tell it how to perform allocation
		std::vector<nyla::aexpr*> dim_sizes;
	};

	class type_table {
	public:

		nyla::type* find_type(nyla::type* type);

		void clear_table();

	private:
		std::unordered_map<nyla::type, nyla::type*,
			               nyla::type::hash_gen> table;
	};
	extern type_table* g_type_table;
}

#endif