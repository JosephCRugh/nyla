#include "type.h"

#include <assert.h>
#include "sym_table.h"
#include "words.h"

// Integers
nyla::type* nyla::types::type_byte   = new nyla::type(nyla::TYPE_BYTE);
nyla::type* nyla::types::type_short  = new nyla::type(nyla::TYPE_SHORT);
nyla::type* nyla::types::type_int    = new nyla::type(nyla::TYPE_INT);
nyla::type* nyla::types::type_long   = new nyla::type(nyla::TYPE_LONG);
nyla::type* nyla::types::type_ubyte  = new nyla::type(nyla::TYPE_UBYTE);
nyla::type* nyla::types::type_ushort = new nyla::type(nyla::TYPE_USHORT);
nyla::type* nyla::types::type_uint   = new nyla::type(nyla::TYPE_UINT);
nyla::type* nyla::types::type_ulong  = new nyla::type(nyla::TYPE_ULONG);
// Characters
nyla::type* nyla::types::type_char8  = new nyla::type(nyla::TYPE_CHAR8);
nyla::type* nyla::types::type_char16 = new nyla::type(nyla::TYPE_CHAR16);
nyla::type* nyla::types::type_char32 = new nyla::type(nyla::TYPE_CHAR32);
// Floats
nyla::type* nyla::types::type_float  = new nyla::type(nyla::TYPE_FLOAT);
nyla::type* nyla::types::type_double = new nyla::type(nyla::TYPE_DOUBLE);
// Other
nyla::type* nyla::types::type_bool   = new nyla::type(nyla::TYPE_BOOL);
nyla::type* nyla::types::type_void   = new nyla::type(nyla::TYPE_VOID);
nyla::type* nyla::types::type_error  = new nyla::type(nyla::TYPE_ERROR);
nyla::type* nyla::types::type_string = new nyla::type(nyla::TYPE_STRING);
nyla::type* nyla::types::type_null   = new nyla::type(nyla::TYPE_NULL);
nyla::type* nyla::types::type_mixed  = new nyla::type(nyla::TYPE_MIXED);


nyla::type* nyla::type_table::find_type(nyla::type* type) {
	auto it = table.find(*type);
	if (it != table.end()) {
		delete type;
		return it->second;
	}
	table[*type] = type;
	return type;
}

void nyla::type_table::clear_table() {
	table.clear();
}

nyla::type_table* nyla::g_type_table = new nyla::type_table;




bool nyla::type::equals(const nyla::type* o) const {
	switch (tag) {
	case TYPE_MODULE: {
		if (o->tag != TYPE_MODULE) return false;
		return unique_module_key == o->unique_module_key;
	}
	case TYPE_PTR: {
		if (o->tag != TYPE_PTR) return false;
		if (get_base_type()->tag == TYPE_MODULE) {
			return ptr_depth == o->ptr_depth
				&& get_base_type()->equals(o->get_base_type());
		}
		return o == this;
	}
	default: return o == this; // Simple pointer comparison
						       // Benefits of the type table
	}
}

std::string nyla::type::to_string() const {
	switch (tag) {
	case TYPE_BYTE:      return "byte";
	case TYPE_SHORT:     return "short";
	case TYPE_INT:       return "int";
	case TYPE_LONG:      return "long";
	case TYPE_UBYTE:     return "ubyte";
	case TYPE_USHORT:    return "ushort";
	case TYPE_UINT:      return "uint";
	case TYPE_ULONG:     return "ulong";
	case TYPE_FLOAT:     return "float";
	case TYPE_DOUBLE:    return "double";
	case TYPE_BOOL:      return "bool";
	case TYPE_VOID:      return "void";
	case TYPE_CHAR8:     return "char8";
	case TYPE_CHAR16:    return "char16";
	case TYPE_CHAR32:    return "char32";
	case TYPE_ERROR:     return "error";
	case TYPE_STRING:    return "String";
	case TYPE_MIXED:     return "<T>";
	case TYPE_NULL:      return "null";
	case TYPE_FD_MODULE: return nyla::g_word_table->get_word(fd_module_name_key).c_str();
	case TYPE_MODULE:    return nyla::g_word_table->get_word(sym_module->name_key).c_str();
	case TYPE_PTR:       return element_type->to_string() + "*";
	case TYPE_ARR:       return element_type->to_string() + "[]";
	}
	assert(!"Unimplemented to_string");
	return "";
}

void nyla::type::calculate_ptr_depth() {
	assert(tag == TYPE_PTR);
	if (element_type->tag == TYPE_PTR) {
		element_type->calculate_ptr_depth();
		ptr_depth = element_type->ptr_depth + 1;
	} else {
		ptr_depth = 1;
	}
}

void nyla::type::calculate_arr_depth() {
	assert(tag == TYPE_ARR);
	if (element_type->tag == TYPE_ARR) {
		element_type->calculate_arr_depth();
		arr_depth = element_type->arr_depth + 1;
	} else {
		arr_depth = 1;
	}
}

nyla::type* nyla::type::get_int(u32 mem_size, bool is_signed) {
	switch (mem_size) {
	case 1: return is_signed ? types::type_byte : types::type_ubyte;
	case 2: return is_signed ? types::type_short : types::type_ushort;
	case 4: return is_signed ? types::type_int : types::type_uint;
	case 8: return is_signed ? types::type_long : types::type_ulong;
	default:
		assert(!"Bad memory size");
		return nullptr;
	}
}

nyla::type* nyla::type::get_float(u32 mem_size) {
	switch (mem_size) {
	case 4: return types::type_float;
	case 8: return types::type_double;
	default:
		assert(!"Bad memory size");
		return nullptr;
	}
}

nyla::type* nyla::type::get_char(u32 mem_size) {
	switch (mem_size) {
	case 1: return types::type_char8;
	case 2: return types::type_char16;
	case 4: return types::type_char32;
	default:
		assert(!"Bad memory size");
		return nullptr;
	}
}

nyla::type* nyla::type::get_ptr(nyla::type* element_type) {
	// TODO: optimize by only creating the type IF it does not exist in the table
	nyla::type* ptr_t = new nyla::type(TYPE_PTR, element_type);
	ptr_t->calculate_ptr_depth();
    return nyla::g_type_table->find_type(ptr_t);
}

nyla::type* nyla::type::get_arr(nyla::type* element_type) {
	// TODO: optimize by only creating the type IF it does not exist in the table
	nyla::type* arr_t = new nyla::type(TYPE_ARR, element_type);
	arr_t->calculate_arr_depth();
	return nyla::g_type_table->find_type(arr_t);
}

nyla::type* nyla::type::get_or_enter_module(nyla::sym_module* sym_module) {
	assert(sym_module);
	// TODO: optimize by only creating the type IF it does not exist in the table
	nyla::type* type_m = new nyla::type(TYPE_MODULE);
	type_m->unique_module_key = sym_module->unique_module_id;
	type_m->sym_module = sym_module;
	return nyla::g_type_table->find_type(type_m);
}

nyla::type* nyla::type::get_fd_module(nyla::type_table* local_type_table, u32 module_name_key) {
	nyla::type* type_m = new nyla::type(TYPE_FD_MODULE);
	type_m->fd_module_name_key = module_name_key;
	return local_type_table->find_type(type_m);
}

void nyla::type::resolve_fd_type(nyla::sym_module* sym_module) {
	assert(sym_module);
	tag               = TYPE_MODULE;
	unique_module_key = sym_module->unique_module_id;
	this->sym_module  = sym_module;
}

nyla::type* nyla::type::get_base_type() const {
	switch (tag) {
	case TYPE_PTR: return get_ptr_base_type();
	case TYPE_ARR: return get_arr_base_type();
	default: assert(!"Should be unreachable");
	}
}

nyla::type* nyla::type::get_arr_base_type() const {
	switch (element_type->tag) {
	case TYPE_ARR: return element_type->get_base_type();
	default:       return element_type;
	}
}

nyla::type* nyla::type::get_ptr_base_type() const {
	switch (element_type->tag) {
	case TYPE_PTR: return element_type->get_base_type();
	default:       return element_type;
	}
}

void nyla::type::set_base_type(nyla::type* base_type) {
	switch (element_type->tag) {
	case TYPE_PTR:
	case TYPE_ARR:
		element_type->set_base_type(base_type);
		break;
	default:
		element_type = base_type;
		break;
	}
}

nyla::type* nyla::type::get_sub_array(u32 depth, u32 depth_count) {
	if (depth == depth_count) return this;
	// int[][][] b;
	// b[0][0]
	// depth = 2
	// b[0]
	// depth = 1
	// b
	// depth = 0
	assert(element_type);
	return element_type->get_sub_array(depth, depth_count + 1);
}

bool nyla::type::is_number() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_UBYTE:
	case TYPE_USHORT:
	case TYPE_UINT:
	case TYPE_ULONG:
	case TYPE_FLOAT:
	case TYPE_DOUBLE:
	case TYPE_CHAR8:
	case TYPE_CHAR16:
	case TYPE_CHAR32:
		return true;
	default:
		return false;
	}
}

bool nyla::type::is_int() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_UBYTE:
	case TYPE_USHORT:
	case TYPE_UINT:
	case TYPE_ULONG:
		// Characters get included so math can
		// be performed on them
	case TYPE_CHAR8:
	case TYPE_CHAR16:
	case TYPE_CHAR32:
		return true;
	default:
		return false;
	}
}

bool nyla::type::is_float() {
	switch (tag) {
	case TYPE_FLOAT:
	case TYPE_DOUBLE:
		return true;
	default:
		return false;
	}
}

bool nyla::type::is_signed() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_FLOAT:
	case TYPE_DOUBLE:
	case TYPE_CHAR8:  // It might not be good to have
	case TYPE_CHAR16: // characters be signed?
	case TYPE_CHAR32:
		return true;
	default:
		return false;
	}
}

bool nyla::type::is_char() {
	switch (tag) {
	case TYPE_CHAR8:
	case TYPE_CHAR16:
	case TYPE_CHAR32:
		return true;
	default:
		return false;
	}
}

bool nyla::type::is_ptr() {
	return tag == TYPE_PTR;
}

bool nyla::type::is_arr() {
	return tag == TYPE_ARR;
}

bool nyla::type::is_module() {
	return tag == TYPE_MODULE;
}

u32 nyla::type::mem_size() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_UBYTE:
	case TYPE_CHAR8:
		return 1;
	case TYPE_SHORT:
	case TYPE_USHORT:
	case TYPE_CHAR16:
		return 2;
	case TYPE_INT:
	case TYPE_UINT:
	case TYPE_CHAR32:
		return 4;
	case TYPE_LONG:
	case TYPE_ULONG:
		return 8;
	case TYPE_FLOAT:  return 4;
	case TYPE_DOUBLE: return 8;
	case TYPE_BOOL:   return 1;
	case TYPE_VOID:   return 0;
		// TODO: Hard coding this is probably a bad idea
		// if we want to allow building 32 bit code on 64 bit machines
	case TYPE_ARR:
	case TYPE_PTR:    return sizeof(void*);
	default:
		assert(!"Missing memory size for type");
		return 0;
	}
}

