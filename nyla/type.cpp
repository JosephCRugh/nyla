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


nyla::type* type::get_byte()   { return nyla::reuse_types::byte_type;   }
nyla::type* type::get_short()  { return nyla::reuse_types::short_type;  }
nyla::type* type::get_int()    { return nyla::reuse_types::int_type;    }
nyla::type* type::get_long()   { return nyla::reuse_types::long_type;   }
nyla::type* type::get_ubyte()  { return nyla::reuse_types::ubyte_type;  }
nyla::type* type::get_ushort() { return nyla::reuse_types::ushort_type; }
nyla::type* type::get_uint()   { return nyla::reuse_types::uint_type;   }
nyla::type* type::get_ulong()  { return nyla::reuse_types::ulong_type;  }
nyla::type* type::get_float()  { return nyla::reuse_types::float_type;  }
nyla::type* type::get_double() { return nyla::reuse_types::double_type; }
nyla::type* type::get_bool()   { return nyla::reuse_types::bool_type;   }
nyla::type* type::get_void()   { return nyla::reuse_types::void_type;   }
nyla::type* type::get_string() { return nyla::reuse_types::string_type; }
nyla::type* type::get_char16() { return nyla::reuse_types::char16_type; }
nyla::type* type::get_error()  { return nyla::reuse_types::error_type;  }

// TODO: probably more efficient to lookup in a map
nyla::type* nyla::type::get_ptr_byte(u32 depth)   { return new nyla::type(TYPE_PTR, TYPE_BYTE, depth);   }
nyla::type* nyla::type::get_ptr_short(u32 depth)  { return new nyla::type(TYPE_PTR, TYPE_SHORT, depth);  }
nyla::type* nyla::type::get_ptr_int(u32 depth)    { return new nyla::type(TYPE_PTR, TYPE_INT, depth);    }
nyla::type* nyla::type::get_ptr_long(u32 depth)   { return new nyla::type(TYPE_PTR, TYPE_LONG, depth);   }
nyla::type* nyla::type::get_ptr_float(u32 depth)  { return new nyla::type(TYPE_PTR, TYPE_FLOAT, depth);  }
nyla::type* nyla::type::get_ptr_double(u32 depth) { return new nyla::type(TYPE_PTR, TYPE_DOUBLE, depth); }
nyla::type* nyla::type::get_ptr_bool(u32 depth)   { return new nyla::type(TYPE_PTR, TYPE_BOOL, depth);   }
nyla::type* nyla::type::get_ptr_char16(u32 depth) { return new nyla::type(TYPE_PTR, TYPE_CHAR16, depth); }

nyla::type* nyla::type::get_arr_byte(std::vector<nyla::aexpr*>& array_depths) {
	return make_arr(TYPE_BYTE, array_depths); }
nyla::type* nyla::type::get_arr_short(std::vector<nyla::aexpr*>& array_depths) {
	return make_arr(TYPE_SHORT, array_depths); }
nyla::type* nyla::type::get_arr_int(std::vector<nyla::aexpr*>& array_depths) {
	return make_arr(TYPE_INT, array_depths); }
nyla::type* nyla::type::get_arr_long(std::vector<nyla::aexpr*>& array_depths)
{ return make_arr(TYPE_LONG, array_depths); }
nyla::type* nyla::type::get_arr_float(std::vector<nyla::aexpr*>& array_depths)
{ return make_arr(TYPE_FLOAT, array_depths); }
nyla::type* nyla::type::get_arr_double(std::vector<nyla::aexpr*>& array_depths)
{ return make_arr(TYPE_DOUBLE, array_depths); }
nyla::type* nyla::type::get_arr_bool(std::vector<nyla::aexpr*>& array_depths)
{ return make_arr(TYPE_BOOL, array_depths); }
nyla::type* nyla::type::get_arr_char16(std::vector<nyla::aexpr*>& array_depths)
{ return make_arr(TYPE_CHAR16, array_depths); }
nyla::type* nyla::type::get_arr_mixed(std::vector<nyla::aexpr*>&array_depths) {
	return make_arr(TYPE_MIXED, array_depths); }

nyla::type* nyla::type::make_arr(type_tag elem_tag, std::vector<nyla::aexpr*>& array_depths) {
	nyla::type* type = new nyla::type(TYPE_ARR);
	type->elem_tag = elem_tag;
	type->array_depths = array_depths;
	return type;
}

nyla::type* type::as_ptr(u32 depth) {
	switch (tag) {
	case TYPE_BYTE:   return nyla::type::get_ptr_byte(depth);
	case TYPE_SHORT:  return nyla::type::get_ptr_short(depth);
	case TYPE_INT:    return nyla::type::get_ptr_int(depth);
	case TYPE_LONG:   return nyla::type::get_ptr_long(depth);
	case TYPE_FLOAT:  return nyla::type::get_ptr_float(depth);
	case TYPE_DOUBLE: return nyla::type::get_ptr_double(depth);
	case TYPE_BOOL:   return nyla::type::get_ptr_bool(depth);
	case TYPE_CHAR16: return nyla::type::get_ptr_char16(depth);
	default:
		assert(!"Must be basic type tag");
		break;
	}
	return nullptr;
}

nyla::type* nyla::type::as_arr(std::vector<nyla::aexpr*>& array_depths) {
	switch (tag) {
	case TYPE_BYTE:   return nyla::type::get_arr_byte(array_depths);
	case TYPE_SHORT:  return nyla::type::get_arr_short(array_depths);
	case TYPE_INT:    return nyla::type::get_arr_int(array_depths);
	case TYPE_LONG:   return nyla::type::get_arr_long(array_depths);
	case TYPE_FLOAT:  return nyla::type::get_arr_float(array_depths);
	case TYPE_DOUBLE: return nyla::type::get_arr_double(array_depths);
	case TYPE_BOOL:   return nyla::type::get_arr_bool(array_depths);
	case TYPE_CHAR16: return nyla::type::get_arr_char16(array_depths);
	default:
		assert(!"Must be basic type tag");
		break;
	}
	return nullptr;
}

nyla::type* nyla::type::get_element_type() {
	switch (elem_tag) {
	case TYPE_BYTE:   return nyla::type::get_byte();
	case TYPE_SHORT:  return nyla::type::get_short();
	case TYPE_INT:    return nyla::type::get_int();
	case TYPE_LONG:   return nyla::type::get_long();
	case TYPE_FLOAT:  return nyla::type::get_float();
	case TYPE_DOUBLE: return nyla::type::get_double();
	case TYPE_BOOL:   return nyla::type::get_bool();
	case TYPE_CHAR16: return nyla::type::get_char16();
	default:
		assert(!"Must be array or ptr");
		break;
	}
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
	if (is_arr()) {
		if (tag != o.tag) return false;
		if (array_depths.size() != o.array_depths.size()) return false;
		for (u32 i = 0; i < o.array_depths.size(); i++) {
			if (o.array_depths[i] != array_depths[i]) {
				return false;
			}
		}
		return true;
	} else {
		return tag == o.tag &&
			ptr_depth == o.ptr_depth;
	}
}

std::ostream& nyla::operator<<(std::ostream& os, const nyla::type& type) {
	switch (type.tag) {
	case TYPE_PTR:
	case TYPE_ARR:
		type.print_elem(os, type.elem_tag);
		break;
	default:
		type.print_elem(os, type.tag);
		break;
	}
	for (u32 i = 0; i < type.ptr_depth; i++) {
		os << "*";
	}
	for (u32 i = 0; i < type.array_depths.size(); i++) {
		os << "[]";
	}
	return os;
}

void type::print_elem(std::ostream& os, type_tag elem_tag) const {
	switch (elem_tag) {
	case TYPE_BYTE:     os << "byte"; break;
	case TYPE_SHORT:    os << "short"; break;
	case TYPE_INT:      os << "int"; break;
	case TYPE_LONG:     os << "long"; break;
	case TYPE_UBYTE:    os << "ubyte"; break;
	case TYPE_USHORT:   os << "ushort"; break;
	case TYPE_UINT:     os << "uint"; break;
	case TYPE_ULONG:    os << "ulong"; break;
	case TYPE_FLOAT:    os << "float"; break;
	case TYPE_DOUBLE:   os << "double"; break;
	case TYPE_BOOL:     os << "bool"; break;
	case TYPE_VOID:     os << "void"; break;
	case TYPE_STRING:   os << "String"; break;
	case TYPE_CHAR16:   os << "char16"; break;
	case TYPE_MIXED:    os << "<T>"; break;
	default:
		os << "THE TYPE NOT IMPLEMENTED: " << elem_tag << std::endl;
		assert(!"Unimplemented elem tag!");
		break;
	}
}