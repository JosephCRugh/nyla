#pragma once

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
		ERROR

	};

	struct type {
		type_tag tag;
		
		type() {}
		type(type_tag _tag) : tag(_tag) {}
	};

}