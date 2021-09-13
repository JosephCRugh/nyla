#include "modifiers.h"

#include <assert.h>

std::string nyla::mod_to_string(u32 mod) {
	switch (mod) {
	case MOD_STATIC:    return "static";
	case MOD_PRIVATE:   return "private";
	case MOD_PROTECTED: return "protected";
	case MOD_EXTERNAL:  return "external";
	case MOD_CONST:     return "const";
	case MOD_COMPTIME:  return "comptime";
	}
	assert("Unimplemented mod_to_string");
	return "unknown-mod";
}
