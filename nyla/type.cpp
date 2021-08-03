#include "type.h"

#include <assert.h>

using namespace nyla;

nyla::type* nyla::reuse_types::byte_type   = new nyla::type(TYPE_BYTE);
nyla::type* nyla::reuse_types::short_type  = new nyla::type(TYPE_SHORT);
nyla::type* nyla::reuse_types::int_type    = new nyla::type(TYPE_INT);
nyla::type* nyla::reuse_types::long_type   = new nyla::type(TYPE_LONG);
nyla::type* nyla::reuse_types::float_type  = new nyla::type(TYPE_FLOAT);
nyla::type* nyla::reuse_types::double_type = new nyla::type(TYPE_DOUBLE);
nyla::type* nyla::reuse_types::bool_type   = new nyla::type(TYPE_BOOL);
nyla::type* nyla::reuse_types::void_type   = new nyla::type(TYPE_VOID);
nyla::type* nyla::reuse_types::error_type  = new nyla::type(TYPE_ERROR);

nyla::type* type::get_byte()   { return nyla::reuse_types::byte_type; }
nyla::type* type::get_short()  { return nyla::reuse_types::short_type; }
nyla::type* type::get_int()    { return nyla::reuse_types::int_type; }
nyla::type* type::get_long()   { return nyla::reuse_types::long_type; }
nyla::type* type::get_float()  { return nyla::reuse_types::float_type; }
nyla::type* type::get_double() { return nyla::reuse_types::double_type; }
nyla::type* type::get_bool()   { return nyla::reuse_types::bool_type; }
nyla::type* type::get_void()   { return nyla::reuse_types::void_type; }
nyla::type* type::get_error()  { return nyla::reuse_types::error_type; }

u32 nyla::type::get_mem_size() {
	switch (tag) {
	case TYPE_BYTE:   return 1;
	case TYPE_SHORT:  return 2;
	case TYPE_INT:    return 4;
	case TYPE_LONG:   return 8;
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
		return true;
	default:
		return false;
	}
}

bool nyla::type::is_number() {
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

bool nyla::type::is_signed() {
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

bool nyla::type::is_float() {
	switch (tag) {
	case TYPE_FLOAT:
	case TYPE_DOUBLE:
		return true;
	default:
		return false;
	}
}
