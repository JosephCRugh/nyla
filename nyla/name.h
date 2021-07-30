#pragma once

#include "types_ext.h"
#include <iostream>
#include <tuple>
#include <unordered_set>

namespace nyla {

	class name {
		name(c_string buffer) {
			m_ptr = strlen(buffer);
			memcpy(m_buffer, buffer, m_ptr);
		}
	public:
		name() {

		}

		static nyla::name make(c_string buffer);

		void append(c8 ch);

		void null_append();

		c_string c_str() const;

		u32 size() const { return m_ptr & 0xFF; }

		friend std::ostream& operator<<(std::ostream& os, const nyla::name& name) {
			os << name.c_str();
			return os;
		}

		bool operator!=(const nyla::name& o) const {
			return !(*this == o);
		}

		bool operator==(const nyla::name& o) const {
			if (o.m_ptr != m_ptr) return false;
			for (u32 i = 0; i < (m_ptr & 0xFF); i++) {
				if (m_buffer[i] != o.m_buffer[i]) return false;
			}
			return true;
		}

		struct hash_gen {
			ulen operator()(const nyla::name& name) const {
				ulen hash = 0;
				for (u32 i = 0; i < name.size(); i++)
					hash = 31 * hash + (name.m_buffer[i] & 0xFF);
				return hash;
			}
		};

	private:
		ulen m_ptr = 0;
		c8   m_buffer[257];
	};

	class name_table : public std::unordered_set<nyla::name,
		                                         nyla::name::hash_gen> {
	public:
		nyla::name register_name(nyla::name& name);
	};

	extern nyla::name_table* g_name_table;
}