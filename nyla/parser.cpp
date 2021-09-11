#include "parser.h"

#include "words.h"
#include "modifiers.h"
#include "type.h"

#include <assert.h>
#include <stack>
#include <unordered_set>

// Switch cases for all built-in
// types
#define TYPE_START_CASES  \
case TK_TYPE_BYTE:        \
case TK_TYPE_UBYTE:       \
case TK_TYPE_SHORT:       \
case TK_TYPE_USHORT:      \
case TK_TYPE_INT:         \
case TK_TYPE_UINT:        \
case TK_TYPE_LONG:        \
case TK_TYPE_ULONG:       \
case TK_TYPE_FLOAT:       \
case TK_TYPE_DOUBLE:      \
case TK_TYPE_CHAR8:       \
case TK_TYPE_CHAR16:      \
case TK_TYPE_CHAR32:      \
case TK_TYPE_BOOL:        \
case TK_TYPE_VOID:

#define MODIFIERS_START_CASES \
case TK_STATIC:               \
case TK_PRIVATE:              \
case TK_PROTECTED:            \
case TK_PUBLIC:               \
case TK_EXTERNAL:             \
case TK_CONST:

/*===========---------------===========*\
 *             Top Level               *
\*===========---------------===========*/

nyla::parser::parser(nyla::compiler& compiler, nyla::lexer& lexer,
	                 nyla::log& log, nyla::sym_table* sym_table,
	                 nyla::afile_unit* file_unit)
	: m_compiler(compiler), m_lexer(lexer), m_log(log), m_sym_table(sym_table), m_file_unit(file_unit) {
	// Read first token to get things started.
	next_token();
}

void nyla::parser::parse_imports() {

	// Parsing imports
	bool parsing_imports = true;
	while (m_current.tag != TK_EOF && parsing_imports) {
		switch (m_current.tag) {
		case TK_IMPORT:
			if (!parse_import()) {
				return; // Aborting since we encounted
				        // a high level error
			}
			break;
			MODIFIERS_START_CASES
		case TK_MODULE:
			parsing_imports = false;
			break;
		default: {
			m_log.err(ERR_EXPECTED_IMPORT, m_current);
			return; // Aborting since we encounted
				    // a high level error
		}
		}
	}
}

bool nyla::parser::parse_import() {

	nyla::aimport* nimport = make<nyla::aimport>(AST_IMPORT, m_current);
	nyla::token st = m_current;
	next_token(); // Consuming 'import' token
	bool more_dots = false;
	std::string file_path = "";
	do {
		u32 ident_key = parse_identifier();
		if (ident_key == nyla::unidentified_ident) {
			skip_recovery();
			return false; // Not continuing due to an error
		}

		if (!file_path.empty()) {
			file_path += "/";
		}

		file_path += g_word_table->get_word(ident_key).c_str();

		more_dots = m_current.tag == '.';
		if (more_dots) {
			next_token(); // Consuming '.'
		}
	} while (more_dots);
	nyla::token et = m_current;

	// Import aliasing to resolve module conflicts
	if (m_current.tag == '(') {
		next_token(); // Consuming (
		bool more_aliasing = false;
		do {
			nyla::token alias_st = m_current;
			u32 original_module_name_key = parse_identifier();
			if (original_module_name_key == nyla::unidentified_ident) {
				return false;
			}
			if (!match(TK_MINUS_GT)) {
				return false;
			}
			u32 alias_module_name_key = parse_identifier();
			if (alias_module_name_key == nyla::unidentified_ident) {
				return false;
			}
			auto it = nimport->module_aliases.find(original_module_name_key);
			if (it != nimport->module_aliases.end()) {
				m_log.err(ERR_DUPLICATE_MODULE_ALIAS, alias_st, m_prev_token);
				return false;
			}

			// Adding alias
			nimport->module_aliases[original_module_name_key] = alias_module_name_key;
			m_file_unit->module_aliases[alias_module_name_key] = original_module_name_key;

			more_aliasing = m_current.tag == ',';
			if (more_aliasing) {
				next_token(); // Consuming ','
			}
		} while (more_aliasing);

		if (!match(')')) {
			return false;
		}
	}

	if (!match(';')) {
		return false;
	}

	nimport->path = file_path;
	auto it = m_file_unit->imports.find(file_path);
	if (it != m_file_unit->imports.end()) {
		m_log.err(ERR_DUPLICATE_IMPORT, st, et);
	}

	sym_table* sym_table = m_compiler.find_sym_table(nimport->path);
	if (!sym_table) {
		m_log.err(ERR_CANNOT_FIND_IMPORT, nimport);
	}

	m_file_unit->imports[file_path] = nimport;

	return true;
}

void nyla::parser::resolve_imports() {

	m_sym_table->m_started_import_resolution = true;

	// TODO: Should ensure that aliases are not created for modules
	// that do not exist and should ensure that aliases are always
	// needed

	// Loading the symbol modules in the imports into the loaded modules
	for (auto& pair : m_file_unit->imports) {
		nyla::aimport* nimport = pair.second;
		sym_table* sym_table = m_compiler.find_sym_table(nimport->path);

		// Looping through all the modules within an import file
		// and mapping the module name (or its alias) to a module
		// for lookup by that name
		const std::vector<sym_module*>& modules = sym_table->get_modules();
		for (sym_module* sym_module : modules) {
			
			bool using_alias = false;

			// Either alias name or default module name
			u32 used_name_key = sym_module->name_key;
			auto alias_it = nimport->module_aliases.find(sym_module->name_key);
			if (alias_it != nimport->module_aliases.end()) {
				used_name_key = alias_it->second; // Use the alias instead
				using_alias = true;
				nimport->module_aliases.erase(alias_it);
			}

			// Checking for a conflict due to two modules sharing the same name
			auto it = m_file_unit->loaded_modules.find(used_name_key);
			if (it != m_file_unit->loaded_modules.end()) {
				error_tag tag_type = using_alias ?
					ERR_MODULE_ALIAS_NAME_ALREADY_USED : ERR_CONFLICTING_IMPORTS;
				m_log.err(tag_type,
					error_payload::conflicting_import({
							used_name_key,
							it->second->internal_path,
							sym_table->get_file_location().internal_path
						}),
					nimport);
				return;
			}
			m_file_unit->loaded_modules[used_name_key] = sym_module;
		}

		for (const auto& pair : nimport->module_aliases) {
			u32 original_name = pair.first;
			u32 alias_name    = pair.second;
			m_log.err(ERR_MODULE_FOR_ALIAS_NOT_FOUND,
				error_payload::word(original_name),
				nimport);
			// Want to remove the forward declaration as to
			// not produce an error
			auto it = m_forward_declared_types.find(alias_name);
			if (it != m_forward_declared_types.end()) {
				m_forward_declared_types.erase(it);
			}
		}
	}

	for (const auto& pair : m_forward_declared_types) {
		const forward_declared_type& forward_declared_type = pair.second;
		nyla::type* type = forward_declared_type.type;

		sym_module* sym_module = m_file_unit->find_module(type->fd_module_name_key);
		if (sym_module) {
			type->resolve_fd_type(sym_module);
		} else {
			for (const forward_declared_type::debug& debug_location : forward_declared_type.debug_locations) {
				m_log.err(ERR_COULD_NOT_FIND_MODULE_TYPE,
					debug_location.line_num,
					debug_location.spos,
					debug_location.epos);
			}
		}
	}
}

void nyla::parser::parse_file_unit() {

	// Parsing modules
	while (m_current.tag != TK_EOF) {
		switch (m_current.tag) {
		MODIFIERS_START_CASES
		case TK_MODULE: {
			nyla::amodule* nmodule = parse_module();
			if (nmodule) {
				m_file_unit->modules.push_back(nmodule);
			} else {
				return; // Aborting since we encounted
				        // a high level error
			}
			break;
		}
		default:
			m_log.err(ERR_EXPECTED_MODULE, m_current);
			return; // Aborting since we encounted
				    // a high level error
		}
	}
}

nyla::amodule* nyla::parser::parse_module() {
	nyla::token st = m_current;
	u32 mods = parse_modifiers();
	next_token(); // Consuming 'module'

	nyla::amodule* nmodule = make<nyla::amodule>(AST_MODULE, st, m_current);
	nmodule->name_key      = parse_identifier();
	m_module = nmodule;
	
	if (nmodule->name_key == nyla::unidentified_ident) {
		return nullptr;
	}

	// Checking to see if a module was already declared by the given
	// name

	sym_module* declared_sym_module = m_file_unit->find_module(nmodule->name_key);

	// TODO: since the declaration can take place outside of the file it would
	// be nice to include debug information informing the user where the two declarations
	// occured to make the conflict resolving easier

	if (declared_sym_module) {
		m_log.err(ERR_MODULE_REDECLARATION, nmodule);
	}

	nmodule->sym_module                   = m_sym_table->enter_module(nmodule->name_key);
	nmodule->sym_module->unique_module_id = m_compiler.get_new_unique_module_id();
	m_file_unit->loaded_modules[nmodule->name_key] = nmodule->sym_module;

	// Entering in the module type
	nyla::type::get_or_enter_module(nmodule->sym_module);

	// Checking for forward declared types
	auto it = m_forward_declared_types.find(nmodule->name_key);
	if (it != m_forward_declared_types.end()) {
		// Type was forward declared
		it->second.type->resolve_fd_type(nmodule->sym_module);
		// Erasing it since it was resolved
		m_forward_declared_types.erase(nmodule->name_key);
	}

	nmodule->sym_module->name_key      = nmodule->name_key;
	nmodule->sym_module->mods          = mods;
	nmodule->sym_module->internal_path = m_sym_table->get_file_location().internal_path;
	
	
	nmodule->sym_scope = m_sym_table->push_scope();
	nmodule->sym_module->scope = nmodule->sym_scope;
	nmodule->sym_scope->is_module_scope = true;
	u32 field_count = 0;
	match('{');
	while (m_current.tag != '}' && m_current.tag != TK_EOF) {
		switch (m_current.tag) {
			TYPE_START_CASES
			MODIFIERS_START_CASES
			case TK_IDENTIFIER: {
				u32 mods         = parse_modifiers();
				const type_info& type = parse_type();
				if (peek_token(1).tag == '(') {
					// Assumed a function declaration
					nmodule->functions.push_back(parse_function(mods, type));
				} else {
					// Assumed a variable declaration
					nyla::aident* ident = make<nyla::aident>(AST_IDENT, m_current);
					ident->ident_key    = parse_identifier();
					m_field_mode = true;
					nyla::avariable_decl* var_decl = parse_variable_decl(mods, type, ident, true);
					u32 last = nmodule->fields.size();
					parse_assignment_list(var_decl, nmodule->fields, true);
					m_field_mode = false;
					for (u32 i = last; i < nmodule->fields.size(); i++) {
						nyla::avariable_decl* field = nmodule->fields[i];
						field->sym_variable->is_field = true;
						field->sym_variable->field_index = field_count++;
						nmodule->sym_module->fields.push_back(field);
					}
					parse_semis();
				}
				break;
			}
			case ';': {
				parse_semis();
				break;
			}
			default: {
				m_log.err(ERR_EXPECTED_MODULE_STMT, m_current);
				next_token(); // Consuming the unexpected token
				break;
			}
		}
	}
	match('}');
	m_sym_table->pop_scope();

	return nmodule;
}

u32 nyla::parser::parse_modifiers() {
	u32 mods = 0;
	while (true) {
		switch (m_current.tag) {
		case TK_STATIC: {
			mods |= nyla::modifier::MOD_STATIC;
			next_token(); // Consuming static
			break;
		}
		case TK_PRIVATE: {
			if (mods & nyla::ACCESS_MODS) {
				m_log.err(ERR_CAN_ONLY_HAVE_ONE_ACCESS_MOD, m_current);
			}
			mods |= nyla::modifier::MOD_PRIVATE;
			next_token(); // Consuming private
			break;
		}
		case TK_PROTECTED: {
			if (mods & nyla::ACCESS_MODS) {
				m_log.err(ERR_CAN_ONLY_HAVE_ONE_ACCESS_MOD, m_current);
			}
			mods |= nyla::modifier::MOD_PROTECTED;
			next_token(); // Consuming protected
			break;
		}
		case TK_PUBLIC: {
			if (mods & nyla::ACCESS_MODS) {
				m_log.err(ERR_CAN_ONLY_HAVE_ONE_ACCESS_MOD, m_current);
			}
			mods |= nyla::modifier::MOD_PUBLIC;
			next_token(); // Consuming public
			break;
		}
		case TK_EXTERNAL: {
			mods |= nyla::modifier::MOD_EXTERNAL;
			next_token(); // Consuming public
			break;
		}
		case TK_CONST: {
			mods |= nyla::modifier::MOD_CONST;
			next_token(); // Consuming public
			break;
		}
		default:
			// TODO: check for incompatible or repeated mods
			return mods;
		}
	}
}

nyla::type_info nyla::parser::parse_type() {
	type_info info;
	nyla::token st = m_current;
	nyla::type* base_type = nullptr;
	switch (m_current.tag) {
		// Integers
	case TK_TYPE_BYTE:   base_type = nyla::types::type_byte;   next_token(); break;
	case TK_TYPE_SHORT:  base_type = nyla::types::type_short;  next_token(); break;
	case TK_TYPE_INT:    base_type = nyla::types::type_int;    next_token(); break;
	case TK_TYPE_LONG:   base_type = nyla::types::type_long;   next_token(); break;
	case TK_TYPE_UBYTE:  base_type = nyla::types::type_ubyte;  next_token(); break;
	case TK_TYPE_USHORT: base_type = nyla::types::type_ushort; next_token(); break;
	case TK_TYPE_UINT:   base_type = nyla::types::type_uint;   next_token(); break;
	case TK_TYPE_ULONG:  base_type = nyla::types::type_ulong;  next_token(); break;
		// Characters
	case TK_TYPE_CHAR8:   base_type = nyla::types::type_char8;   next_token(); break;
	case TK_TYPE_CHAR16:  base_type = nyla::types::type_char16;  next_token(); break;
	case TK_TYPE_CHAR32:  base_type = nyla::types::type_char32;  next_token(); break;
		// Floats
	case TK_TYPE_FLOAT:  base_type = nyla::types::type_float;  next_token(); break;
	case TK_TYPE_DOUBLE: base_type = nyla::types::type_double; next_token(); break;
		// Other
	case TK_TYPE_BOOL:   base_type = nyla::types::type_bool; next_token(); break;
	case TK_TYPE_VOID:   base_type = nyla::types::type_void; next_token(); break;
		// Module
	case TK_IDENTIFIER: {
		sym_module* sym_module = m_file_unit->find_module(m_current.word_key);
		// If the sym module does not exist then for now a forward
		// declared type of that module is created. Either that module
		// exist within this file further down in which case it is
		// resolved while parsing the file or it exist in a dependency
		// file and is resolved after parsing

		if (!sym_module) {
			base_type = nyla::type::get_fd_module(&m_fd_type_table, m_current.word_key);

			// Storing information about the forward declaration to ensure it's
			// resolution later
			auto it = m_forward_declared_types.find(m_current.word_key);
			if (it == m_forward_declared_types.end()) {
				forward_declared_type forward_declared_type;
				forward_declared_type.type = base_type;
				m_forward_declared_types[m_current.word_key] = forward_declared_type;
			}
			forward_declared_type& forward_declared_type =
				m_forward_declared_types[m_current.word_key];
			forward_declared_type::debug forward_declared_debug;
			forward_declared_debug.spos = m_current.spos;
			forward_declared_debug.epos = m_current.epos;
			forward_declared_debug.line_num = m_current.line_num;
			forward_declared_type.debug_locations.push_back(forward_declared_debug);
		} else {
			// Not forward declared so using the type
			base_type = nyla::type::get_or_enter_module(sym_module);
		}

		next_token();
		break;
	}
	default: {
		info.type = nyla::types::type_error;
		m_log.err(ERR_EXPECTED_VALID_TYPE, m_current);
		skip_recovery();
		return info;
	}
	}

	if (m_current.tag == '*') {
		next_token(); // Consuming *
		u32 num_stars = 1;
		while (m_current.tag == '*') {
			next_token(); // Consuming *
			++num_stars;
		}
		base_type = nyla::type::get_ptr(base_type);
		for (u32 i = 1; i < num_stars; i++) {
			base_type = nyla::type::get_ptr(base_type);
		}

		if (num_stars > nyla::MAX_SUBSCRIPTS) {
			m_log.err(ERR_TOO_MANY_PTR_SUBSCRIPTS, st, m_prev_token);
		}
	}

	if (m_current.tag == '[') {
		std::vector<nyla::aexpr*> dim_sizes;
		while (m_current.tag == '[') {
			next_token(); // Consuming [
			if (m_current.tag != ']') {
				nyla::aexpr* expr = parse_expression();
				if (expr->tag == AST_ERROR) {
					info.type = base_type;
					return info;
				}
				dim_sizes.push_back(expr);
			} else {
				dim_sizes.push_back(nullptr);
			}
			match(']');
		}

		u32 num_brackets = dim_sizes.size();
		base_type = nyla::type::get_arr(base_type);
		for (u32 i = 1; i < num_brackets; i++) {
			base_type = nyla::type::get_arr(base_type);
		}
		info.dim_sizes = dim_sizes;

		if (num_brackets > nyla::MAX_SUBSCRIPTS) {
			m_log.err(ERR_TOO_MANY_ARR_SUBSCRIPTS, st, m_prev_token);
		}
	}

	info.type = base_type;
	return info;
}

nyla::afunction* nyla::parser::parse_function(u32 mods, const nyla::type_info& return_type) {
	nyla::afunction* function = make<nyla::afunction>(AST_FUNCTION, m_current);
	m_function = function;

	function->return_type = return_type.type;
	function->name_key    = parse_identifier();
	bool is_external      = mods & MOD_EXTERNAL;

	// Pushing the scope earlier to include function parameters
	function->sym_scope = m_sym_table->push_scope();
	match('(');
	if (m_current.tag != ')') {
		bool more_parameters = false;
		do {

			function->parameters.push_back(parse_variable_decl(false));

			more_parameters = m_current.tag == ',';
			if (more_parameters) {
				next_token(); // consuming ,
			}
		} while (more_parameters);
	}
	match(')');
	// External functions do not have bodies so popping the
	// scope early
	if (is_external) {
		m_sym_table->pop_scope();
	}

	std::vector<nyla::type*> param_types;
	for (nyla::avariable_decl* param : function->parameters) {
		param_types.push_back(param->type);
	}

	s32 decl_line_num = m_sym_table->has_function_been_declared(m_module->sym_module, function->name_key, param_types);
	if (decl_line_num != -1) {
		m_log.err(ERR_FUNCTION_REDECLARATION,
			error_payload::func_decl({function->name_key, (u32)decl_line_num }), function);
	}

	function->sym_function = m_sym_table->enter_function(m_module->sym_module, function->name_key);
	function->sym_function->line_num    = function->line_num;
	function->sym_function->mods        = mods;
	function->sym_function->sym_module  = m_module->sym_module;
	function->sym_function->name_key    = function->name_key;
	function->sym_function->return_type = function->return_type;
	function->sym_function->param_types = param_types;

	if (!is_external) {
		match('{');
		parse_stmts(function->stmts);
		match('}');
		m_sym_table->pop_scope();
	} else {
		parse_semis(); // type <name>(params);  for external functions
	}

	if (m_sym_table->m_search_for_main_function) {
		if (!function->sym_function->is_member_function()) {
			if (!function->is_external() && (function->sym_function->mods & MOD_PUBLIC)) {
				if (function->name_key == nyla::main_ident) {
					// Canidate function for "main"
					// TODO: need to also check for program arguments
					if (function->parameters.size() == 0) {
						function->is_main_function = true;
						m_compiler.set_main_function(function->sym_function);
					}
				}
			}
		}
	}

	return function;
}

nyla::avariable_decl* nyla::parser::parse_variable_decl(u32 mods,
	                                                    const nyla::type_info& type_info,
	                                                    nyla::aident* ident,
	                                                    bool check_module_scope) {
	
	return parse_variable_decl(mods, type_info.type, type_info.dim_sizes, ident, check_module_scope);
}

nyla::avariable_decl* nyla::parser::parse_variable_decl(u32 mods,
	                                                    nyla::type* type,
	                                                    const std::vector<nyla::aexpr*>& dim_sizes,
	                                                    nyla::aident* ident, bool check_module_scope) {

	if (!m_field_mode && (mods & nyla::ACCESS_MODS)) {
		m_log.err(ERR_ILLEGAL_MODIFIERS, ident);
	}
	
	nyla::avariable_decl* variable_decl = make<nyla::avariable_decl>(AST_VARIABLE_DECL, ident, ident);
	variable_decl->ident = ident;

	variable_decl->name_key = ident->ident_key;
	variable_decl->type = type;

	if (variable_decl->name_key == nyla::unidentified_ident) {
		// TODO
	}

	if (type == nyla::types::type_void) {
		m_log.err(ERR_VARIABLE_HAS_VOID_TYPE,
			error_payload::word(variable_decl->name_key),
			variable_decl);
	}

	if (m_sym_table->has_variable_been_declared(variable_decl->name_key, check_module_scope)) {
		m_log.err(ERR_VARIABLE_REDECLARATION,
			error_payload::word(variable_decl->name_key),
			variable_decl);
	}

	sym_variable* sym_variable = m_sym_table->enter_variable(variable_decl->name_key);
	sym_variable->name_key = variable_decl->name_key;
	sym_variable->type = type;
	sym_variable->mods = mods;
	variable_decl->sym_variable = sym_variable;
	variable_decl->sym_variable->position_declared_at = ident->spos;

	sym_variable->arr_dim_sizes = dim_sizes;
	
	return variable_decl;
}

nyla::avariable_decl* nyla::parser::parse_variable_decl(bool check_module_scope) {
	// C++ compiler seems not to call functions in order listed so pulling them out
	u32 mods                         = parse_modifiers();
	const nyla::type_info& type_info = parse_type();
	nyla::aident* ident              = make<nyla::aident>(AST_IDENT, m_current);
	ident->ident_key                 = parse_identifier();
	return parse_variable_decl(mods, type_info, ident, check_module_scope);
}

void nyla::parser::parse_assignment_list(nyla::avariable_decl* first_decl,
	                                     std::vector<nyla::avariable_decl*>& declarations,
	                                     bool check_module_scope) {
	parse_assignment(first_decl);
	declarations.push_back(first_decl);
	while (m_current.tag == ',') {
		next_token(); // Consuming ,
		nyla::aident* ident = make<nyla::aident>(AST_IDENT, m_current);
		ident->ident_key    = parse_identifier();
		nyla::avariable_decl* var_decl = parse_variable_decl(
			                                      first_decl->sym_variable->mods,
			                                      first_decl->type,
			                                      first_decl->sym_variable->arr_dim_sizes,
			                                      ident,
			                                      check_module_scope);
		parse_assignment(var_decl);
		declarations.push_back(var_decl);
	}
}

void nyla::parser::parse_assignment_list(std::vector<nyla::avariable_decl*>& declarations,
	                                     bool check_module_scope) {
	parse_assignment_list(parse_variable_decl(check_module_scope), declarations, check_module_scope);
}

void nyla::parser::parse_assignment(nyla::avariable_decl* variable_decl) {
	if (m_current.tag == '=') {
		if (peek_token(1).tag == TK_QQQ) {
			next_token(); // Consuming =
			next_token(); // Consumg ???
			variable_decl->default_initialize = false;
		} else {
			variable_decl->assignment = parse_expression(variable_decl->ident);
		}
	}
}

/*===========---------------===========*\
 *             Statements              *
\*===========---------------===========*/

void nyla::parser::parse_stmts(std::vector<nyla::aexpr*>& stmts) {
	while (m_current.tag != '}' && m_current.tag != TK_EOF) {
		parse_stmt(stmts);
	}
}

void nyla::parser::parse_stmt(std::vector<nyla::aexpr*>& stmts) {
	switch (m_current.tag) {
	case TK_RETURN:
		stmts.push_back(parse_return());
		break;
	TYPE_START_CASES
	MODIFIERS_START_CASES {
		std::vector<nyla::avariable_decl*> declarations;
		parse_assignment_list(declarations, false);
		// TODO: optimize by reserving additional space first?
		for (u32 i = 0; i < declarations.size(); i++) {
			stmts.push_back(declarations[i]);
		}
		parse_semis();
		break;
	}
	case TK_FOR: {
		stmts.push_back(parse_for_loop());
		break;
	}
	case TK_WHILE: {
		stmts.push_back(parse_while_loop());
		break;
	}
	case TK_IF: {
		stmts.push_back(parse_if());
		break;
	}
	case TK_BREAK: {
		nyla::acontrol* break_stmt = make<nyla::acontrol>(AST_BREAK, m_current);
		stmts.push_back(break_stmt);
		next_token(); // Consuming 'break' token
		parse_semis();
		break;
	}
	case TK_CONTINUE: {
		nyla::acontrol* continue_stmt = make<nyla::acontrol>(AST_CONTINUE, m_current);
		stmts.push_back(continue_stmt);
		next_token(); // Consuming 'break' token
		parse_semis();
		break;
	}
	case TK_IDENTIFIER: {
		nyla::token next_token = peek_token(1);
		switch (next_token.tag) {
		case TK_IDENTIFIER:
		case '*': {
			std::vector<nyla::avariable_decl*> declarations;
			parse_assignment_list(declarations, false);
			// TODO: optimize by reserving additional space first?
			for (u32 i = 0; i < declarations.size(); i++) {
				stmts.push_back(declarations[i]);
			}
			parse_semis();
			break;
		}
		case '[': {
			
			// As long as there is no variable name by the identifier in the scope
			// it is a declaration
			bool is_declaration = !m_sym_table->has_variable_been_declared(m_current.word_key, true);

			if (is_declaration) {
				stmts.push_back(parse_variable_decl(false));
				parse_semis();
			} else {
				// Array access
				nyla::aexpr* expr = parse_expression();
				parse_semis();
				stmts.push_back(expr);
			}

			break;
		}
		default: {
			nyla::aexpr* expr = parse_expression();
			parse_semis();
			stmts.push_back(expr);
		}
		}
		break;
	}
	case ';': {
		parse_semis();
		break;
	}
	default: {
		m_log.err(ERR_EXPECTED_STMT, m_current);
		next_token(); // Consuming the unexpected token
		skip_recovery();
		break;
	}
	}
}

nyla::aexpr* nyla::parser::parse_return() {
	nyla::areturn* ret = make<nyla::areturn>(AST_RETURN, m_current);
	next_token(); // Consuming return token
	if (m_current.tag == ';') {
		// Must be a void return
		parse_semis();
		return ret;
	}
	ret->value = parse_expression();
	parse_semis();
	return ret;
}

nyla::aexpr* nyla::parser::parse_for_loop() {
	nyla::afor_loop* loop = make<nyla::afor_loop>(AST_FOR_LOOP, m_current);

	// Need to create a new scope early so declarations are placed
	// there instead of on the outside of the loop
	loop->sym_scope = m_sym_table->push_scope();

	next_token(); // Consuming 'for' token.
	if (m_current.tag != ';') {
		parse_assignment_list(loop->declarations, false);
	}

	match(';');
	if (m_current.tag != ';') {
		loop->cond = parse_expression();
	} else {
		// Defaults to always being true
		abool* always_true = make<nyla::abool>(AST_VALUE_BOOL, m_current, m_current);
		always_true->tof = true;
		loop->cond = always_true;
	}

	match(';');
	nyla::aexpr* post = nullptr;
	if (m_current.tag != '{') {
		post = parse_expression();
	}
	if (m_current.tag == '{') {
		match('{');
		parse_stmts(loop->body);
		match('}');
	} else {
		// Single statement
		//   for expr ; expr ; expr   stmt
		parse_stmt(loop->body);
	}

	if (post) {
		loop->post_exprs.push_back(post);
	}

	m_sym_table->pop_scope();
	return loop;
}

nyla::aexpr* nyla::parser::parse_while_loop() {
	nyla::awhile_loop* loop = make<nyla::awhile_loop>(AST_WHILE_LOOP, m_current, m_current);

	next_token(); // Consuming 'while' token.

	loop->cond = parse_expression();
	parse_scope(loop->sym_scope, loop->body);

	return loop;
}

nyla::aif* nyla::parser::parse_if() {
	nyla::aif* ifstmt = make<nyla::aif>(AST_IF, m_current, m_current);

	next_token(); // Consuming 'if' token.

	ifstmt->cond = parse_expression();


	parse_scope(ifstmt->sym_scope, ifstmt->body);

	if (m_current.tag == TK_ELSE) {
		next_token(); // Consuming 'else' token.
		if (m_current.tag == TK_IF) { // else if
			ifstmt->else_if = parse_if();
		} else { // leftover else
			parse_scope(ifstmt->else_sym_scope, ifstmt->else_body);
		}
	}

	return ifstmt;
}

/*===========---------------===========*\
 *             Expressions             *
\*===========---------------===========*/

std::unordered_map<u32, u32> binary_ops = {
	
	{ '*', 9 },
	{ '/', 9 },
	{ '%', 9 },

	{ '+', 8 },
	{ '-', 8 },

	{ nyla::TK_LT_LT, 7 }, // <<
	{ nyla::TK_GT_GT, 7 }, // >>

	{ '<'           , 6 },
	{ '>'           , 6 },
	{ nyla::TK_LT_EQ, 6 }, // <=
	{ nyla::TK_GT_EQ, 6 }, // >=

	{ nyla::TK_EQ_EQ , 5 }, // ==
	{ nyla::TK_EXL_EQ, 5 }, // !=

	{ '&', 4 },
	
	{ '^', 3 },

	{ '|', 2 },

	{ nyla::TK_AMP_AMP, 1 }, // &&
	{ nyla::TK_BAR_BAR, 1 }, // ||

	{ '='              , 0 },
	{ nyla::TK_PLUS_EQ , 0 }, // +=
	{ nyla::TK_MINUS_EQ, 0 }, // -=
	{ nyla::TK_STAR_EQ , 0 }, // *=
	{ nyla::TK_SLASH_EQ, 0 }, // /=
	{ nyla::TK_MOD_EQ  , 0 }, // %=
	{ nyla::TK_AMP_EQ  , 0 }, // &=
	{ nyla::TK_CRT_EQ  , 0 }, // ^=
	{ nyla::TK_BAR_EQ  , 0 }, // |=
	{ nyla::TK_LT_LT_EQ, 0 }, // <<=
	{ nyla::TK_GT_GT_EQ, 0 }, // >>=

};

nyla::aexpr* nyla::parser::parse_expression() {
	return parse_expression(parse_unary());
}

nyla::aexpr* nyla::parser::parse_expression(nyla::aexpr* lhs) {
	nyla::token op = m_current;
	nyla::token next_op;

	// Since some operations have to be delayed
	// because of order of operations a stack
	// is formed keeping a backlog of those operations
	// that need to be processed later
	struct stack_unit {
		nyla::token  op;
		nyla::aexpr* expr;
	};
	std::stack<stack_unit> op_stack;

	while (binary_ops.find(op.tag) != binary_ops.end()) {
		next_token(); // Consuming the operator.

		nyla::aexpr* rhs = parse_unary();

		next_op = m_current;
		bool more_operators = binary_ops.find(next_op.tag) != binary_ops.end();
		if (more_operators && binary_ops[next_op.tag] > binary_ops[op.tag]) {
			// Delaying the operation until later since the next operator has a
			// higher precedence.
			stack_unit unit = { op, lhs };
			op_stack.push(unit);
			lhs = rhs;
			op = next_op;

		} else {
			// Apply the binary operator!
			nyla::aexpr* res = on_binary_op(op, lhs, rhs);
			lhs = res;

			while (!op_stack.empty()) {
				rhs = lhs;
				stack_unit unit = op_stack.top();
				// Still possible to have the right side have higher precedence.
				if (more_operators && binary_ops[next_op.tag] > binary_ops[unit.op.tag]) {
					lhs = rhs;
					op = next_op;
					break;
				}

				op_stack.pop();
				lhs = unit.expr;

				// Apply the binary operator!
				nyla::aexpr* res = on_binary_op(unit.op, lhs, rhs);
				lhs = res;
			}
			op = m_current;
		}
	}

	return lhs;
}

nyla::aexpr* nyla::parser::on_binary_op(const nyla::token& op_token, nyla::aexpr* lhs, nyla::aexpr* rhs) {
	// Subdivides an equal and operator into seperate nodes.
	static auto equal_and_op_apply =
		[this, op_token](u32 op, nyla::aexpr* lhs, nyla::aexpr* rhs) -> nyla::abinary_op* {
		nyla::abinary_op* eq_op = make<nyla::abinary_op>(AST_BINARY_OP, op_token);
		nyla::abinary_op* op_op = make<nyla::abinary_op>(AST_BINARY_OP, op_token);
		eq_op->op  = '=';
		op_op->op  = op;
		op_op->lhs = lhs;
		op_op->rhs = rhs;
		eq_op->lhs = lhs;
		eq_op->rhs = op_op;
		return eq_op;
	};
	// Letting folding occure via LLVM folding operation. Could handle it here
	// which may result in faster compilation.
	switch (op_token.tag) {
	case TK_PLUS_EQ:  return equal_and_op_apply('+', lhs, rhs);
	case TK_MINUS_EQ: return equal_and_op_apply('-', lhs, rhs);
	case TK_STAR_EQ:  return equal_and_op_apply('*', lhs, rhs);
	case TK_SLASH_EQ: return equal_and_op_apply('/', lhs, rhs);
	case TK_MOD_EQ:   return equal_and_op_apply('%', lhs, rhs);
	case TK_AMP_EQ:   return equal_and_op_apply('&', lhs, rhs);
	case TK_CRT_EQ:   return equal_and_op_apply('^', lhs, rhs);
	case TK_BAR_EQ:   return equal_and_op_apply('|', lhs, rhs);
	case TK_LT_LT_EQ: return equal_and_op_apply(nyla::TK_LT_LT, lhs, rhs);
	case TK_GT_GT_EQ: return equal_and_op_apply(nyla::TK_GT_GT, lhs, rhs);
	default: {
		nyla::abinary_op* binary_op = make<nyla::abinary_op>(AST_BINARY_OP, op_token);
		binary_op->lhs = lhs;
		binary_op->rhs = rhs;
		binary_op->op = op_token.tag;
		return binary_op;
	}
	}
}

std::unordered_set<u32> unary_ops_set = {
	'-', '+', '&', '*', '~', '!',
	nyla::TK_PLUS_PLUS,   // ++
	nyla::TK_MINUS_MINUS, // --
};

nyla::aexpr* nyla::parser::parse_unary() {
	nyla::aexpr* res = nullptr;
	if (unary_ops_set.find(m_current.tag) != unary_ops_set.end()) {
		u32 op = m_current.tag;
		nyla::token st = m_current;
		next_token(); // Consuming the unary operator.
		nyla::aexpr* factor = parse_factor();
		nyla::aunary_op* unary_op = make<nyla::aunary_op>(AST_UNARY_OP, st.line_num, st.spos, factor->epos);
		unary_op->op = op;
		unary_op->factor = factor;
		if (m_current.tag == '.') {
			nyla::aexpr* dot_op = parse_dot_op(factor);
			unary_op->factor = dot_op;
		}
		res = unary_op;
	} else {
		res = parse_factor();
		if (m_current.tag == '.') {
			res = parse_dot_op(res);
		}
	}

	// TODO: post dot ops
	// a++ / a--
	if (m_current.tag == TK_PLUS_PLUS || m_current.tag == TK_MINUS_MINUS) {

	}

	return res;
}

nyla::aexpr* nyla::parser::parse_factor() {
	switch (m_current.tag) {
	case TK_VALUE_INT: {
		nyla::anumber* number = make<nyla::anumber>(AST_VALUE_INT, m_current);
		number->value_int = m_current.value_int;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_UINT: {
		nyla::anumber* number = make<nyla::anumber>(AST_VALUE_UINT, m_current);
		number->value_uint = m_current.value_uint;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_LONG: {
		nyla::anumber* number = make<nyla::anumber>(AST_VALUE_LONG, m_current);
		number->value_long = m_current.value_long;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_ULONG: {
		nyla::anumber* number = make<nyla::anumber>(AST_VALUE_ULONG, m_current);
		number->value_ulong = m_current.value_ulong;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_FLOAT: {
		nyla::anumber* number = make<nyla::anumber>(AST_VALUE_FLOAT, m_current);
		number->value_float = m_current.value_float;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_DOUBLE: {
		nyla::anumber* number = make<nyla::anumber>(AST_VALUE_DOUBLE, m_current);
		number->value_double = m_current.value_double;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_FALSE: {
		nyla::abool* boolean = make<nyla::abool>(AST_VALUE_BOOL, m_current);
		boolean->tof = false;
		next_token(); // Consuming the false token.
		return boolean;
	}
	case TK_TRUE: {
		nyla::abool* boolean = make<nyla::abool>(AST_VALUE_BOOL, m_current);
		boolean->tof = true;
		next_token(); // Consuming the true token.
		return boolean;
	}
	case TK_NULL: {
		nyla::anumber* null_number = make<nyla::anumber>(AST_VALUE_NULL, m_current);
		next_token(); // Consuming null
		return null_number;
	}
	case TK_CAST: {
		next_token(); // consuming 'cast' keyword
		nyla::atype_cast* type_cast = make<nyla::atype_cast>(AST_TYPE_CAST, m_current);
		match('(');
		const nyla::type_info& type_info = parse_type();
		if (!type_info.dim_sizes.empty()) {
			// TODO: dont want to allow stuff like 'int[n]'
			// TODO: produce error
		}
		type_cast->type = type_info.type;
		if (type_cast->type == nyla::types::type_error) {
			return type_cast;
		}
		match(')');
		type_cast->value = parse_expression();
		return type_cast;
	}
	case TK_VALUE_STRING8: {
		nyla::astring* str = make<nyla::astring>(AST_STRING8, m_current);
		str->lit8 = m_current.value_string8;
		str->dim_size = str->lit8.size();
		next_token(); // Consuming the string
		return str;
	}
	case TK_VALUE_CHAR8: {
		nyla::anumber* char_number = make<nyla::anumber>(AST_VALUE_CHAR8, m_current);
		char_number->value_int = m_current.value_char8;
		next_token(); // Consuming character
		return char_number;
	}
	case TK_VAR: {
		nyla::avar_object* var_object = make<nyla::avar_object>(AST_VAR_OBJECT, m_current, m_current);
		next_token(); // Consuming var
		nyla::token identifier_token = m_current;
		u32 ident_key = parse_identifier();
		if (ident_key == nyla::unidentified_ident) {
			skip_recovery();
			return make<nyla::err_expr>(AST_ERROR, m_current);
		}
		auto alias_it = m_file_unit->module_aliases.find(ident_key);
		if (alias_it != m_file_unit->module_aliases.end()) {
			ident_key = alias_it->second;
		}

		nyla::afunction_call* function_call = parse_function_call(ident_key, identifier_token);
		var_object->constructor_call = function_call;
		return var_object;
	}

	case '(': {
		match('(');
		nyla::aexpr* expr = parse_expression();
		match(')');
		return expr;
	}
	case '{': {
		return parse_array();
	}
	case TK_IDENTIFIER: {
		nyla::token ident_token = m_current;
		u32 ident_key = parse_identifier();

		switch (m_current.tag) {
		case '(':
			return parse_function_call(ident_key, ident_token);
		case '[':
			return parse_array_access(ident_key, ident_token);
		default:
			break;
		}

		nyla::aident* ident = make<nyla::aident>(AST_IDENT, ident_token);
		ident->ident_key = ident_key;
		return ident;
	}
	default: {
		m_log.err(ERR_EXPECTED_FACTOR, m_current);
		skip_recovery();
		return make<nyla::err_expr>(AST_ERROR, m_current);
	}
	}
}

nyla::afunction_call* nyla::parser::parse_function_call(u32 name_key, const nyla::token& start_token) {
	nyla::afunction_call* function_call = make<nyla::afunction_call>(AST_FUNCTION_CALL, start_token);
	function_call->name_key = name_key;
	match('(');
	if (m_current.tag != ')') {
		bool more_expressions = false;
		do {
			function_call->arguments.push_back(parse_expression());

			more_expressions = m_current.tag == ',';
			if (more_expressions) {
				next_token(); // Consuming ,
			}
		} while (more_expressions);
	}
	match(')');
	return function_call;
}

nyla::aexpr* nyla::parser::parse_dot_op(nyla::aexpr* factor) {
	bool more_dots = false;
	nyla::adot_op* dot_op = make<nyla::adot_op>(AST_DOT_OP, factor, factor);
	dot_op->factor_list.push_back(factor);
	next_token(); // Consuming first '.'
	do {

		nyla::aexpr* factor = parse_factor();
		if (factor->tag == AST_ERROR) {
			break; // No reason to continue.
		}

		dot_op->factor_list.push_back(factor);

		more_dots = m_current.tag == '.';
		if (more_dots) {
			next_token(); // Consuming '.'
		}
	} while (more_dots);
	dot_op->epos = m_current.epos;
	return dot_op;
}

nyla::aarray_access* nyla::parser::parse_array_access(u32 name_key, const nyla::token& start_token) {

	nyla::aarray_access* array_access =
		make<nyla::aarray_access>(AST_ARRAY_ACCESS, start_token, start_token);
	nyla::aident* ident = make<nyla::aident>(AST_IDENT, start_token);
	ident->ident_key = name_key;
	array_access->ident = ident;

	while (m_current.tag == '[') {
		next_token(); // Consuming [
		nyla::aexpr* expr = parse_expression();
		if (expr->tag == AST_ERROR) {
			return array_access;
		}
		array_access->indexes.push_back(expr);
		match(']');
	}

	array_access->epos = m_prev_token.epos;
	return array_access;
}

nyla::aexpr* nyla::parser::parse_array() {
	// NOTE OF WARNING: Before returning on this function
	// the function should ALWAYS decrement m_array_depth if
	// it has been incremented

	nyla::aarray* arr = make<nyla::aarray>(AST_ARRAY, m_current);
	match('{');
	if (m_array_depth == 0) {
		m_array_too_deep = false;
		m_array_depth_ptr = 0;
	}
	++m_array_depth;
	if (m_array_depth > m_array_depth_ptr) {
		m_array_depth_ptr = m_array_depth;
	}
	if (m_array_depth_ptr > nyla::MAX_SUBSCRIPTS) {
		m_array_depth_ptr = nyla::MAX_SUBSCRIPTS;
	}
	bool more_elements = false;
	if (m_current.tag != '}') {
		do {
			nyla::aexpr* element = parse_expression();
			arr->elements.push_back(element);
			more_elements = m_current.tag == ',';
			if (more_elements) {
				next_token(); // Consuming ,
			}
		} while (more_elements);
	}
	--m_array_depth;
	if (m_array_depth > nyla::MAX_SUBSCRIPTS - 1) {
		m_array_too_deep = true;
	}
	if (m_array_depth == 0) {
		if (m_array_too_deep) {
			m_log.err(ERR_ARRAY_TOO_DEEP, arr->line_num, arr->spos, arr->spos);
		}

		// TODO: PREVENT ARRAYS OF FORM { { 1, 2, 3 }, 3, 5, 2 }
	}
	arr->epos = m_current.epos;
	arr->dim_size = arr->elements.size();
	match('}');
	return arr;
}

void nyla::parser::parse_scope(sym_scope*& sym_scope, std::vector<nyla::aexpr*>& stmts) {
	sym_scope = m_sym_table->push_scope();
	if (m_current.tag == '{') {
		match('{');
		parse_stmts(stmts);
		match('}');
	} else {
		// Single statement
		parse_stmt(stmts);
	}
	m_sym_table->pop_scope();
}

void nyla::parser::parse_semis() {
	match(';');
	while (m_current.tag == ';') {
		next_token(); // Consuming any additional semis.
	}
}

/*===========---------------===========*\
 *             Utilities               *
\*===========---------------===========*/

void nyla::parser::next_token() {
	m_prev_token = m_current;
	if (!m_saved_tokens.empty()) {
		m_current = m_saved_tokens.front();
		m_saved_tokens.pop();
	} else {
		m_current = m_lexer.next_token();
	}
}

nyla::token nyla::parser::peek_token(u32 n) {
	if (n == 0) {
		assert(!"There is no reason to peek zero tokens");
	}
	for (u32 i = 0; i < n; i++) {
		m_saved_tokens.push(m_lexer.next_token());
	}
	return m_saved_tokens.back();
}

u32 nyla::parser::parse_identifier() {
	if (m_current.tag == TK_IDENTIFIER) {
		u32 word_key = m_current.word_key;
		next_token(); // Consuming identifier
		return word_key;
	}
	m_log.err(ERR_EXPECTED_IDENTIFIER, m_current);
	return nyla::unidentified_ident;
}

void nyla::parser::skip_recovery() {
	while (true) {
		switch (m_current.tag) {
		case ';':
		case TK_EOF:
			// Statement keywords
		case TK_FOR:
		case TK_IF:
		case TK_WHILE:
		case TK_SWITCH:
		MODIFIERS_START_CASES
			return;
		TYPE_START_CASES
		case TK_IDENTIFIER: {
			if (peek_token(1).tag == TK_IDENTIFIER) {
				return;
			}
			next_token();
			break;
		}
		default:
			next_token();
		}
	}
}

bool nyla::parser::match(u32 token_tag, bool consume) {
	if (m_current.tag == token_tag) {
		if (consume) {
			next_token(); // Consuming the matched token
		}
		return true;
	} else {
		m_log.err(ERR_EXPECTED_TOKEN,
			      error_payload::expected_token({token_tag, m_current}),
			      m_prev_token);
		return false;
	}
}
