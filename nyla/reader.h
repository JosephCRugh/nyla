#pragma once

#include <string.h>
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

	private:
		u32      m_length;
		c_string m_buffer;
		u32      m_ptr = 0;
	};
}