#include "name.h"

using namespace nyla;

nyla::name_table* nyla::g_name_table = new nyla::name_table;

nyla::name name::make(c_string buffer) {
	nyla::name name = nyla::name(buffer);
	return nyla::g_name_table->register_name(name);
}

void name::append(c8 ch) {
	m_buffer[m_ptr++ & 0xFF] = ch;
}

c_string name::c_str() const {
	return m_buffer;
}

void name::null_append() {
	m_buffer[m_ptr & 0xFF] = '\0';
}

nyla::name nyla::name_table::register_name(nyla::name& name) {
	auto it = find(name);
	if (it != end()) {
		return *it;
	}
	name.null_append();
	insert(name);
	return name;
}
