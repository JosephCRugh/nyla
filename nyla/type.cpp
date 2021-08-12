#include "type.h"

#include <assert.h>

using namespace nyla;

nyla::type* nyla::reuse_types::byte_type   = new nyla::type(TYPE_BYTE);
nyla::type* nyla::reuse_types::short_type  = new nyla::type(TYPE_SHORT);
nyla::type* nyla::reuse_types::int_type    = new nyla::type(TYPE_INT);
nyla::type* nyla::reuse_types::long_type   = new nyla::type(TYPE_LONG);
nyla::type* nyla::reuse_types::ubyte_type  = new nyla::type(TYPE_UBYTE);
nyla::type* nyla::reuse_types::ushort_type = new nyla::type(TYPE_USHORT);
nyla::type* nyla::reuse_types::uint_type   = new nyla::type(TYPE_UINT);
nyla::type* nyla::reuse_types::ulong_type  = new nyla::type(TYPE_ULONG);
nyla::type* nyla::reuse_types::float_type  = new nyla::type(TYPE_FLOAT);
nyla::type* nyla::reuse_types::double_type = new nyla::type(TYPE_DOUBLE);
nyla::type* nyla::reuse_types::bool_type   = new nyla::type(TYPE_BOOL);
nyla::type* nyla::reuse_types::void_type   = new nyla::type(TYPE_VOID);
nyla::type* nyla::reuse_types::string_type = new nyla::type(TYPE_STRING);
nyla::type* nyla::reuse_types::char16_type = new nyla::type(TYPE_CHAR16);
nyla::type* nyla::reuse_types::error_type  = new nyla::type(TYPE_ERROR);
nyla::type* nyla::reuse_types::mixed_type  = new nyla::type(TYPE_MIXED);
nyla::type* nyla::reuse_types::null_type   = new nyla::type(TYPE_NULL);

nyla::type* type::get_byte()       { return nyla::reuse_types::byte_type;   }
nyla::type* type::get_short()      { return nyla::reuse_types::short_type;  }
nyla::type* type::get_int()        { return nyla::reuse_types::int_type;    }
nyla::type* type::get_long()       { return nyla::reuse_types::long_type;   }
nyla::type* type::get_ubyte()      { return nyla::reuse_types::ubyte_type;  }
nyla::type* type::get_ushort()     { return nyla::reuse_types::ushort_type; }
nyla::type* type::get_uint()       { return nyla::reuse_types::uint_type;   }
nyla::type* type::get_ulong()      { return nyla::reuse_types::ulong_type;  }
nyla::type* type::get_float()      { return nyla::reuse_types::float_type;  }
nyla::type* type::get_double()     { return nyla::reuse_types::double_type; }
nyla::type* type::get_bool()       { return nyla::reuse_types::bool_type;   }
nyla::type* type::get_void()       { return nyla::reuse_types::void_type;   }
nyla::type* type::get_string()     { return nyla::reuse_types::string_type; }
nyla::type* type::get_char16()     { return nyla::reuse_types::char16_type; }
nyla::type* type::get_mixed()      { return nyla::reuse_types::mixed_type;  }
nyla::type* type::get_error()      { return nyla::reuse_types::error_type;  }
nyla::type* type::get_null()       { return nyla::reuse_types::null_type;   }

void type::calculate_arr_depth() {
	assert(tag == TYPE_ARR);
	if (elem_type->tag == TYPE_ARR) {
		elem_type->calculate_arr_depth();
		arr_depth = elem_type->arr_depth + 1;
	} else {
		arr_depth = 1;
	}
}

void type::calculate_ptr_depth() {
	assert(tag == TYPE_PTR);
	if (elem_type->tag == TYPE_ARR) {
		elem_type->calculate_ptr_depth();
		ptr_depth = elem_type->ptr_depth + 1;
	} else {
		ptr_depth = 1;
	}
}

nyla::type* nyla::type::get_array_at_depth(u32 req_depth, u32 depth) {
	if (req_depth == depth) {
		return this;
	}
	return elem_type->get_array_at_depth(req_depth, depth + 1);
}

nyla::type* type::get_array_base_type() {
	if (elem_type->tag == TYPE_ARR) {
		return elem_type->get_array_base_type();
	} else {
		return elem_type;
	}
}

nyla::type* type::get_ptr_base_type() {
	if (elem_type->tag == TYPE_PTR) {
		return elem_type->get_ptr_base_type();
	} else {
		return elem_type;
	}
}

void type::set_array_base_type(nyla::type* type) {
	if (elem_type->tag == TYPE_ARR) {
		elem_type->set_array_base_type(type);
	} else {
		elem_type = type;
	}
}

nyla::type* type::get_arr(nyla::type* elem_type, nyla::aexpr* dim_size) {
	return new nyla::type(TYPE_ARR, elem_type, dim_size);
}

nyla::type* type::get_ptr(nyla::type* elem_type) {
	return new nyla::type(TYPE_PTR, elem_type);
}

u32 type::get_mem_size() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_UBYTE:
		return 1;
	case TYPE_SHORT:
	case TYPE_USHORT:
		return 2;
	case TYPE_INT:
	case TYPE_UINT:
		return 4;
	case TYPE_LONG:
	case TYPE_ULONG:
		return 8;
	case TYPE_FLOAT:  return 4;
	case TYPE_DOUBLE: return 8;
	case TYPE_BOOL:   return 1;
	case TYPE_VOID:   return 0;
	case TYPE_ERROR:  return 0;
	default:
		assert(!"Missing memory size for type");
		break;
	}
	return u32();
}

bool type::is_int() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_UBYTE:
	case TYPE_USHORT:
	case TYPE_UINT:
	case TYPE_ULONG:
		return true;
	default:
		return false;
	}
}

bool type::is_number() {
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
		return true;
	default:
		return false;
	}
}

bool type::is_signed() {
	switch (tag) {
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_FLOAT:
	case TYPE_DOUBLE:
		return true;
	default:
		return false;
	}
}

bool type::is_float() {
	switch (tag) {
	case TYPE_FLOAT:
	case TYPE_DOUBLE:
		return true;
	default:
		return false;
	}
}

bool type::is_ptr() {
	return tag == TYPE_PTR;
}

bool nyla::type::is_arr() {
	return tag == TYPE_ARR;
}

bool nyla::type::operator!=(const nyla::type& o) {
	return !(*this == o);
}

bool nyla::type::operator==(const nyla::type& o) {
	assert(!is_arr());
	if (tag != o.tag) {
		return false;
	}
	if (tag == TYPE_PTR) {
		return ptr_depth == o.ptr_depth;
	}
	return true;
}

std::ostream& nyla::operator<<(std::ostream& os, const nyla::type& type) {
	switch (type.tag) {
	case TYPE_ARR:
		os << *type.elem_type;
		os << "[]";
		break;
	case TYPE_PTR:
		os << *type.elem_type;
		os << "*";
		break;
	default:
		type.print_elem(os, type.tag);
		break;
	}

	//switch (type.tag) {
	//case TYPE_PTR:
	//case TYPE_ARR:
	//	type.print_elem(os, type.elem_tag);
	//	break;
	//default:
	//	type.print_elem(os, type.tag);
	//	break;
	//}
	//for (u32 i = 0; i < type.ptr_depth; i++) {
	//	os << "*";
	//}
	//for (u32 i = 0; i < type.array_depths.size(); i++) {
	//	os << "[]";
	//}
	return os;
}

void type::print_elem(std::ostream& os, type_tag elem_tag) const {
	switch (elem_tag) {
	case TYPE_BYTE:     os << "byte";   break;
	case TYPE_SHORT:    os << "short";  break;
	case TYPE_INT:      os << "int";    break;
	case TYPE_LONG:     os << "long";   break;
	case TYPE_UBYTE:    os << "ubyte";  break;
	case TYPE_USHORT:   os << "ushort"; break;
	case TYPE_UINT:     os << "uint";   break;
	case TYPE_ULONG:    os << "ulong";  break;
	case TYPE_FLOAT:    os << "float";  break;
	case TYPE_DOUBLE:   os << "double"; break;
	case TYPE_BOOL:     os << "bool";   break;
	case TYPE_VOID:     os << "void";   break;
	case TYPE_STRING:   os << "String"; break;
	case TYPE_CHAR16:   os << "char16"; break;
	case TYPE_MIXED:    os << "<T>";    break;
	case TYPE_NULL:     os << "null";   break;
	case TYPE_ERROR:    os << "error";  break;
	default:
		os << "THE TYPE NOT IMPLEMENTED: " << elem_tag << std::endl;
		assert(!"Unimplemented elem tag!");
		break;
	}
}