#ifndef NYLA_SOURCE_H
#define NYLA_SOURCE_H

#include "types_ext.h"
#include <string>

namespace nyla {

	/*
	 * Range into the source from some
	 * start position to an end position.
	 */
	struct range {
		u32 start;
		u32 end;  // End position is actually 1 character past end.
		          // If the range is 0-1 then it encompesses only character
		          // at index 0.

		inline u32 length() const { return end - start; }
	};

	/*
	 * Allows traversing over a buffer by
	 * keeeping an internal pointer.
	 */
	class source {
	public:
		explicit source(c_string buffer, ulen length)
			: m_buffer(buffer), m_length(length)
		{ }

		// Current character at the buffer's position.
		c8 cur_char();

		// Moves the buffer's position forward by one and returns
		// the character
		c8 next_char();

		// Views the next character without moving the buffer's position
		// forward
		c8 peek_char();

		// The current position in the buffer
		u32 position() const { return m_ptr; }

		// Character at index i
		c8 operator[](u32 i) const;

		// Gets the string from within the range
		std::string from_range(const range& range);

		std::string from_window_till_nl(u32 start_pos, s32 direction);

	private:
		u32      m_length;
		c_string m_buffer;
		u32      m_ptr = 0;
	};

}

#endif