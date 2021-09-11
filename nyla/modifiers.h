#ifndef NYLA_MODIFIERS_H
#define NYLA_MODIFIERS_H

#include "types_ext.h"
#include <string>

namespace nyla {
	enum modifier {
		MODS_START     = 0x01,
		MOD_STATIC     = 0x01,
		MOD_PRIVATE    = 0x02,
		MOD_PROTECTED  = 0x04,
		MOD_PUBLIC     = 0x08,
		MOD_EXTERNAL   = 0x10,
		MOD_CONST      = 0x20,
		MOD_COMPTIME   = 0x40,
		MODS_END       = MOD_COMPTIME,
	};

	constexpr u32 ACCESS_MODS = MOD_PRIVATE | MOD_PROTECTED | MOD_PUBLIC;

	std::string mod_to_string(u32 mod);
}

#endif