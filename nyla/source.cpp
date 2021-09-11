#include "source.h"

c8 nyla::source::cur_char() {
	if (m_ptr >= m_length) return '\0';
	return m_buffer[m_ptr];
}

c8 nyla::source::next_char() {
	++m_ptr;
	return cur_char();
}

c8 nyla::source::peek_char() {
	++m_ptr;
	c8 ch = cur_char();
	--m_ptr;
	return ch;
}

c8 nyla::source::operator[](u32 i) const {
	return m_buffer[i];
}

std::string nyla::source::from_range(const range& range) {
	std::string str;
	str.resize(range.length());
	memcpy(&str[0], &m_buffer[range.start], range.length());
	return str;
}

std::string nyla::source::from_window_till_nl(u32 start_pos, s32 direction) {
	s32 index = start_pos;
	std::string s;
	s32 move = 0;
	while (index >= 0 && index < m_length) {
		if (m_buffer[index] == '\n') break;
		if (m_buffer[index] == '\r') break; // Do no include new lines
		s += m_buffer[index];
		if (direction < 0) {
			--move;
			--index;
		} else {
			++move;
			++index;
		}
		if (move == direction) break;
	}
	if (direction < 0) {
		std::reverse(s.begin(), s.end());
	}
	return s;
}
