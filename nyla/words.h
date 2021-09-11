#ifndef NYLA_WORDS_H
#define NYLA_WORDS_H

#include "types_ext.h"
#include <vector>
#include <unordered_map>

namespace nyla {

	class word {
		word(c_string buffer) {
			m_ptr = strlen(buffer);
			memcpy(m_buffer, buffer, m_ptr);
		}
	public:
		word() {}

		struct hash_gen {
			ulen operator()(const nyla::word& word) const {
				return word.m_precomputed_hash;
			}
		};

		bool operator!=(const nyla::word& o) const {
			return !(*this == o);
		}

		bool operator==(const nyla::word& o) const {
			if (o.m_ptr != m_ptr) return false;
			for (u32 i = 0; i < (m_ptr & 0xFF); i++) {
				if (m_buffer[i] != o.m_buffer[i]) return false;
			}
			return true;
		}

		static u32 make(c_string buffer);

		// Appends a character onto the buffer
		void append(c8 ch);

		// Null terminate and compute the hash
		void finalize();

		// Word as a C-Style string
		c_string c_str() const { return m_buffer; }

		u32 size() const { return m_ptr; }

	private:
		ulen m_precomputed_hash;

		ulen m_ptr = 0;
		c8   m_buffer[257]; // Character buffer
	};

	/*
	 * Converts words into unsigned integers
	 * to save time comparing words.
	 */
	class word_table {
	public:

		u32 get_key(nyla::word& word);

		nyla::word get_word(u32 word_key);

		void clear_table();

	private:

		// Map between words and their keys
		std::unordered_map<nyla::word, u32,
			               nyla::word::hash_gen> key_mapping;

		std::vector<nyla::word> m_words; // Index into the vector
										 // is a word's key

		u32 key_index = 0;

	};

	extern nyla::word_table* g_word_table;

}

#endif