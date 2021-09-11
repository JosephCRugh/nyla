#include "words.h"

#include <assert.h>

u32 nyla::word::make(c_string buffer) {
	nyla::word word = nyla::word(buffer);
	return g_word_table->get_key(word);
}

void nyla::word::append(c8 ch) {
	m_buffer[m_ptr++ & 0xFF] = ch;
}

void nyla::word::finalize() {
	m_buffer[m_ptr & 0xFF] = '\0'; // Null appending
	ulen hash = 0;
	for (u32 i = 0; i < size(); i++)
		hash = 31 * hash + (m_buffer[i] & 0xFF);
	m_precomputed_hash = hash;
}

u32 nyla::word_table::get_key(nyla::word& word) {
	word.finalize();

	auto it = key_mapping.find(word);
	if (it != key_mapping.end()) {
		return it->second;
	}

	u32 this_key_index = key_index;
	key_mapping[word] = this_key_index;
	m_words.push_back(word);
	++key_index;
	return this_key_index;
}

nyla::word nyla::word_table::get_word(u32 word_key) {
	assert(m_words.size() > word_key);
	return m_words[word_key];
}

void nyla::word_table::clear_table() {
	key_mapping.clear();
	m_words.clear();
	key_index = 0;
}

nyla::word_table* nyla::g_word_table = new nyla::word_table;