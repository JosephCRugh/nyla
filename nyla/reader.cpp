#include "reader.h"

using namespace nyla;

c8 reader::cur_char() {
	if (m_ptr >= m_length) return '\0';
	return m_buffer[m_ptr];
}

c8 reader::next_char() {
	++m_ptr;
	return cur_char();
}

c8 reader::operator[](u32 i) const {
	return m_buffer[i];
}
