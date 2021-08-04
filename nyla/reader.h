#pragma once

#include <string>
#include <tuple>
#include "types_ext.h"

namespace nyla {
	class reader {
	public:
		explicit reader(c_string buffer, u32 length)
			: m_buffer(buffer), m_length(length)
		{}

		explicit reader(c_string buffer)
			: m_buffer(buffer), m_length(strlen(buffer))
		{ }

		c8 cur_char();

		c8 next_char();

		c8 peek_char();

		u32 position() const { return m_ptr; }

		c8 operator[](u32 i) const;

		std::string from_range(const std::tuple<u32, u32>& range);

		std::string from_window_till_nl(u32 start_pos, s32 direction);

	private:
		u32      m_length;
		c_string m_buffer;
		u32      m_ptr = 0;
	};
}