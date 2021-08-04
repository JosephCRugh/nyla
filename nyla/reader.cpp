#include "reader.h"

#include <algorithm>

using namespace nyla;

c8 reader::cur_char() {
	if (m_ptr >= m_length) return '\0';
	return m_buffer[m_ptr];
}

c8 reader::next_char() {
	++m_ptr;
	return cur_char();
}

c8 reader::peek_char() {
	++m_ptr;
	c8 ch = cur_char();
	--m_ptr;
	return ch;
}

c8 reader::operator[](u32 i) const {
	return m_buffer[i];
}

std::string reader::from_range(const std::tuple<u32, u32>& range) {
	std::string str;
	u32 len = std::get<1>(range) - std::get<0>(range);
	str.resize(len);
	memcpy(&str[0], &m_buffer[std::get<0>(range)], len);
	return str;
}

std::string reader::from_window_till_nl(u32 start_pos, s32 direction) {
	s32 index = start_pos;
	std::string s;
	s32 move = 0;
	while (index > 0 && index < m_length) {
		if (m_buffer[index] == '\n') break;
		if (m_buffer[index] == '\r') break;
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
