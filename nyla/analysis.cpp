#include "analysis.h"

inline u32 max(u32 a, u32 b) {
	return a > b ? a : b;
}

nyla::analysis::analysis(nyla::compiler& compiler, nyla::log& log,
	                     nyla::sym_table* sym_table, nyla::afile_unit* file_unit)
	: m_compiler(compiler), m_log(log), m_sym_table(sym_table),
	  m_llvm_generator(compiler, new llvm::Module("JIT module", *llvm_context), nullptr, false),
      m_file_unit(file_unit) {
	// TODO: clean up JIT module
}

void nyla::analysis::check_file_unit() {
	m_sym_table->m_started_analysis = true;
	m_file_unit->literal_constant = false;
	for (nyla::amodule* nmodule : m_file_unit->modules) {
		check_module(nmodule);
	}
}

void nyla::analysis::check_module(nyla::amodule* nmodule) {
	nmodule->literal_constant = false;
	m_sym_module = nmodule->sym_module;
	m_sym_scope  = nmodule->sym_scope;
	m_checking_fields = true;
	for (nyla::avariable_decl* field : nmodule->fields) {
		check_expression(field);
		if (field->type->is_module()) {
			check_circular_fields(field, field->type->sym_module, nmodule->sym_module->unique_module_id);
		}
	}
	m_checking_fields = false;
	m_checking_globals = true;
	for (nyla::avariable_decl* global : nmodule->globals) {
		check_expression(global);
	}
	m_checking_globals = false;
	for (nyla::afunction* function : nmodule->functions) {
		check_function(function);
	}
	for (nyla::afunction* constructor : nmodule->constructors) {
		check_function(constructor);
	}
	m_sym_scope = m_sym_scope->parent;
}

bool nyla::analysis::check_circular_fields(nyla::avariable_decl* original_field, sym_module* sym_module, u32 unique_module_key) {
	for (nyla::avariable_decl* field : sym_module->fields) {
		if (field->type->is_module()) {
			if (field->type->unique_module_key == unique_module_key) {
				m_log.err(ERR_CIRCULAR_FIELDS, original_field);
				return true;
			}
			if (check_circular_fields(original_field, field->type->sym_module, unique_module_key)) {
				return true;
			}
		}
	}
	return false;
}

void nyla::analysis::check_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_VARIABLE_DECL:
		check_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr));
		break;
	case AST_RETURN:
		check_return(dynamic_cast<nyla::areturn*>(expr));
		break;
	case AST_VALUE_BYTE:
	case AST_VALUE_SHORT:
	case AST_VALUE_INT:
	case AST_VALUE_LONG:
	case AST_VALUE_UBYTE:
	case AST_VALUE_USHORT:
	case AST_VALUE_UINT:
	case AST_VALUE_ULONG:
	case AST_VALUE_FLOAT:
	case AST_VALUE_DOUBLE:
	case AST_VALUE_CHAR8:
	case AST_VALUE_CHAR16:
	case AST_VALUE_CHAR32:
		check_number(dynamic_cast<nyla::anumber*>(expr));
		break;
	case AST_VALUE_BOOL:
		expr->type = nyla::types::type_bool;
		break;
	case AST_VALUE_NULL:
		expr->type = nyla::types::type_null;
		break;
	case AST_BINARY_OP:
		check_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
		break;
	case AST_UNARY_OP:
		check_unary_op(dynamic_cast<nyla::aunary_op*>(expr));
		break;
	case AST_IDENT: {
		bool static_context = true;
		if (m_function) {
			static_context = m_function->sym_function->mods & MOD_STATIC;
		}
		if (m_checking_fields) {
			static_context = false;
		}
		check_ident(static_context, m_sym_scope, dynamic_cast<nyla::aident*>(expr));
		break;
	}
	case AST_FOR_LOOP:
		check_for_loop(dynamic_cast<nyla::afor_loop*>(expr));
		break;
	case AST_WHILE_LOOP:
		check_while_loop(dynamic_cast<nyla::awhile_loop*>(expr));
		break;
	case AST_TYPE_CAST:
		check_type_cast(dynamic_cast<nyla::atype_cast*>(expr));
		break;
	case AST_STRING8:
	case AST_STRING16:
	case AST_STRING32:
		expr->type = nyla::types::type_string;
		break;
	case AST_FUNCTION_CALL: {
		bool static_context = true;
		if (m_function) {
			static_context = m_function->sym_function->mods & MOD_STATIC;
		}
		check_function_call(static_context, m_sym_module, dynamic_cast<nyla::afunction_call*>(expr), false);
		break;
	}
	case AST_ARRAY_ACCESS: {
		bool static_context = true;
		if (m_function) {
			static_context = m_function->sym_function->mods & MOD_STATIC;
		}
		check_array_access(static_context, m_sym_scope, dynamic_cast<nyla::aarray_access*>(expr));
		break;
	}
	case AST_ARRAY:
		check_array(dynamic_cast<nyla::aarray*>(expr));
		break;
	case AST_VAR_OBJECT:
		check_var_object(dynamic_cast<nyla::avar_object*>(expr));
		break;
	case AST_DOT_OP:
		check_dot_op(dynamic_cast<nyla::adot_op*>(expr));
		break;
	case AST_THIS:
		m_log.err(ERR_THIS_KEYWORD_EXPECTS_DOT_OP, expr);
		expr->type = nyla::types::type_error;
		break;
	case AST_IF:
		check_if(dynamic_cast<nyla::aif*>(expr));
		break;
	default:
		assert(!"Unhandled analysis check for expression");
		break;
	}
}

void nyla::analysis::check_function(nyla::afunction* function) {
	function->literal_constant = function->sym_function->mods & MOD_COMPTIME;

	if (function->is_external()) return;
	m_function = function;
	m_sym_scope = function->sym_scope;
	check_scope(function->stmts, function->comptime_compat);
	
	if (!m_sym_scope->found_return) {
		if (function->return_type == nyla::types::type_void) {
			// Adding a void return
			nyla::areturn* ret = make<nyla::areturn>(AST_RETURN, function);
			function->stmts.push_back(ret);
		} else {
			m_log.err(ERR_FUNCTION_EXPECTS_RETURN, function);
		}
	}

	if (!function->comptime_compat && function->sym_function->mods & MOD_COMPTIME) {
		// TODO: Produce an error since the function cannot be computed
		// at compile time
	}

	m_sym_scope = m_sym_scope->parent;
}

void nyla::analysis::check_variable_decl(nyla::avariable_decl* variable_decl) {
	variable_decl->literal_constant = variable_decl->sym_variable->mods & MOD_COMPTIME;

	nyla::type* type = variable_decl->type;
	nyla::type* underlying_type = type;
	if (type->is_arr() || type->is_ptr()) {
		underlying_type = type->get_base_type();
	}

	if (variable_decl->assignment != nullptr) {
		check_expression(variable_decl->assignment);
		if (!variable_decl->assignment->comptime_compat) {
			variable_decl->comptime_compat = false;
		}

		if (variable_decl->assignment->type == nyla::types::type_error) {
			variable_decl->type = nyla::types::type_error;
			return;
		}
	}

	if (variable_decl->type->is_arr()) {

		// Computing the dimension sizes   int[n][k]
		//                                     ^  ^
		//                                     these
		for (nyla::aexpr* arr_dim_size : variable_decl->sym_variable->arr_dim_sizes) {
			if (arr_dim_size == nullptr) {
				// TODO: probably should do more here!
				continue;
			}

			check_expression(arr_dim_size);
			if (arr_dim_size->type == nyla::types::type_error) {
				variable_decl->type = nyla::types::type_error;
				return;
			}
			if (!arr_dim_size->type->is_int()) {
				variable_decl->type = nyla::types::type_error;
				return;
			}

			// TODO: This also needs to be moved to when generating
			// comptime code
			// TODO: create anonymous function
			llvm::ConstantInt* result =
				llvm::cast<llvm::ConstantInt>(m_llvm_generator.gen_expr_rvalue(arr_dim_size));
			s64 computed_dim_size = result->getValue().getSExtValue();
			// TODO: make sure the value is POSITIVE
			variable_decl->sym_variable->computed_arr_dim_sizes.push_back(computed_dim_size);
		}

		if (variable_decl->assignment != nullptr &&
			!variable_decl->sym_variable->computed_arr_dim_sizes.empty()) {

			// TODO: All of this will need to be moved to the comptime section since
			// the computed arr size information will not be present until then
			nyla::abinary_op* assignment = dynamic_cast<nyla::abinary_op*>(variable_decl->assignment);
			if (assignment->rhs->tag == AST_ARRAY || assignment->rhs->type == nyla::types::type_string) {

				bool sizes_match;
				if (assignment->rhs->tag == AST_ARRAY) {
					nyla::aarray* arr = dynamic_cast<nyla::aarray*>(assignment->rhs);
					sizes_match = compare_arr_size(arr, variable_decl->sym_variable->computed_arr_dim_sizes);
				} else {
					nyla::astring* str = dynamic_cast<nyla::astring*>(assignment->rhs);
					sizes_match = compare_arr_size(str, variable_decl->sym_variable->computed_arr_dim_sizes);
				}
				if (!sizes_match) {
					variable_decl->type = nyla::types::type_error;
					return;
				}
			}
		}
	}

	// In case of assignments when the identifier of the declaration
	// was checked it set the literal_constant of the lhs/eq_op to
	// be false. Reverting this so that it can fold the assignment
	if (variable_decl->assignment) {
		nyla::abinary_op* eq_op = dynamic_cast<nyla::abinary_op*>(variable_decl->assignment);
		// lhs should always be a literal constant since it is just the identifier of the
		// declaration
		if (eq_op->rhs->literal_constant) {
			eq_op->literal_constant = true;
		}
	}



	if (!variable_decl->comptime_compat && variable_decl->sym_variable->mods & MOD_COMPTIME) {
		// TODO: produce an error that the variable is not able to be computed
		// at compile time
	}
}

void nyla::analysis::check_return(nyla::areturn* ret) {
	ret->literal_constant = false;

	m_sym_scope->found_return = true;
	if (ret->value) {
		check_expression(ret->value);
		if (ret->value->type == nyla::types::type_error) {
			return;
		}
		if (!ret->value->comptime_compat) {
			ret->comptime_compat = false;
		}
		if (is_assignable_to(m_function->return_type, ret->value->type)) {
			attempt_assignment(m_function->return_type, ret->value);
		} else {
			m_log.err(ERR_RETURN_VALUE_NOT_COMPATIBLE_WITH_RETURN_TYPE,
				      error_payload::types({ ret->value->type, m_function->return_type }),
				      ret);
		}
	} else {
		if (m_function->return_type != nyla::types::type_void) {
			m_log.err(ERR_FUNCTION_EXPECTS_RETURN_VALUE, ret);
		}
	}
}

void nyla::analysis::check_number(nyla::anumber* number) {
	switch (number->tag) {
	case AST_VALUE_BYTE:   number->type = nyla::types::type_byte;   break;
	case AST_VALUE_SHORT:  number->type = nyla::types::type_short;  break;
	case AST_VALUE_INT:    number->type = nyla::types::type_int;    break;
	case AST_VALUE_LONG:   number->type = nyla::types::type_long;   break;
	case AST_VALUE_UBYTE:  number->type = nyla::types::type_ubyte;  break;
	case AST_VALUE_USHORT: number->type = nyla::types::type_ushort; break;
	case AST_VALUE_UINT:   number->type = nyla::types::type_uint;   break;
	case AST_VALUE_ULONG:  number->type = nyla::types::type_ulong;  break;
	case AST_VALUE_FLOAT:  number->type = nyla::types::type_float;  break;
	case AST_VALUE_DOUBLE: number->type = nyla::types::type_double; break;
	case AST_VALUE_CHAR8:  number->type = nyla::types::type_char8;  break;
	case AST_VALUE_CHAR16: number->type = nyla::types::type_char16; break;
	case AST_VALUE_CHAR32: number->type = nyla::types::type_char32; break;
	default:
		assert(!"Haven't implemented type mapping for value.");
		break;
	}
}

void nyla::analysis::check_binary_op(nyla::abinary_op* binary_op) {
	
	check_expression(binary_op->lhs);
	check_expression(binary_op->rhs);

	nyla::type* lhs_type = binary_op->lhs->type;
	nyla::type* rhs_type = binary_op->rhs->type;

	if (lhs_type == nyla::types::type_error ||
		rhs_type == nyla::types::type_error) {
		binary_op->type = nyla::types::type_error;
		return;
	}

	if (!binary_op->lhs->comptime_compat || !binary_op->rhs->comptime_compat) {
		binary_op->comptime_compat = false;
	}

	if (!binary_op->lhs->literal_constant || !binary_op->rhs->literal_constant) {
		binary_op->literal_constant = false;
	}

	switch (binary_op->op) {
	case '=': {
		if (!is_assignable_to(lhs_type, rhs_type)) {
			m_log.err(ERR_CANNOT_ASSIGN,
				      error_payload::types({ rhs_type, lhs_type }),
				      binary_op);
		}

		attempt_assignment(lhs_type, binary_op->rhs);

		binary_op->type = binary_op->lhs->type;
		break;
	}
	case '+': case '-': case '*': case '/': {
		if (!lhs_type->is_number()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				      error_payload::op_cannot_apply({ binary_op->op, lhs_type }),
				      binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}
		if (!rhs_type->is_number()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				      error_payload::op_cannot_apply({ binary_op->op, rhs_type }),
				      binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}
		
		if (lhs_type->is_int() && rhs_type->is_int()) {
			u32 larger_mem_size = max(lhs_type->mem_size(), rhs_type->mem_size());
			nyla::type* to_type =
				nyla::type::get_int(larger_mem_size,
					lhs_type->is_signed() || lhs_type->is_signed());
			binary_op->lhs = make_cast(binary_op->lhs, to_type);
			binary_op->rhs = make_cast(binary_op->rhs, to_type);
			binary_op->type = to_type;
		} else {
			// At least one float
			u32 larger_mem_size = max(lhs_type->mem_size(), rhs_type->mem_size());
			nyla::type* to_type = nyla::type::get_float(larger_mem_size);
			binary_op->lhs = make_cast(binary_op->lhs, to_type);
			binary_op->rhs = make_cast(binary_op->rhs, to_type);
			binary_op->type = to_type;
		}

		break;
	}
	case '%': case '&': case '^': case '|':
	case nyla::TK_LT_LT: case nyla::TK_GT_GT: {

		if (!lhs_type->is_int()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				error_payload::op_cannot_apply({ binary_op->op, lhs_type }),
				binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}
		if (!rhs_type->is_int()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				error_payload::op_cannot_apply({ binary_op->op, rhs_type }),
				binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}

		u32 larger_mem_size = max(lhs_type->mem_size(), rhs_type->mem_size());
		nyla::type* to_type =
			nyla::type::get_int(larger_mem_size,
				lhs_type->is_signed() || lhs_type->is_signed());
		binary_op->lhs = make_cast(binary_op->lhs, to_type);
		binary_op->rhs = make_cast(binary_op->rhs, to_type);
		binary_op->type = to_type;

		break;
	}
	case '<': case '>': case TK_EQ_EQ: case TK_LT_EQ: case TK_GT_EQ: {
		if (!lhs_type->is_number()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				error_payload::op_cannot_apply({ binary_op->op, lhs_type }),
				binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}
		if (!rhs_type->is_number()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				error_payload::op_cannot_apply({ binary_op->op, rhs_type }),
				binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}

		binary_op->type = nyla::types::type_bool;
		break;
	}
	case TK_AMP_AMP: case TK_BAR_BAR: {
		if (lhs_type->tag != TYPE_BOOL) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				error_payload::op_cannot_apply({ binary_op->op, lhs_type }),
				binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}
		if (rhs_type->tag != TYPE_BOOL) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				error_payload::op_cannot_apply({ binary_op->op, rhs_type }),
				binary_op);
			binary_op->type = nyla::types::type_error;
			return;
		}

		binary_op->type = nyla::types::type_bool;
		break;
	}
	default: {
		assert(!"Unimplemented binary operation!");
		break;
	}
	}

}

void nyla::analysis::check_unary_op(nyla::aunary_op* unary_op) {
	check_expression(unary_op->factor);
	if (unary_op->factor->type == nyla::types::type_error) {
		unary_op->type = nyla::types::type_error;
		return;
	}
	if (!unary_op->factor->comptime_compat) {
		unary_op->comptime_compat = false;
	}
	switch (unary_op->op) {
	case '-': case '+': {
		if (!unary_op->factor->type->is_number()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				      error_payload::op_cannot_apply({ unary_op->op, unary_op->factor->type }),
				      unary_op);
			unary_op->type = nyla::types::type_error;
			return;
		}
		unary_op->type = unary_op->factor->type;
		break;
	}
	case '&': {
		
		unary_op->literal_constant = false;

		if (!is_lvalue(unary_op->factor)) {
			// TODO: produce error message
			unary_op->type = nyla::types::type_error;
			return;
		}

		unary_op->type = nyla::type::get_ptr(unary_op->factor->type);
		unary_op->type->calculate_ptr_depth();
		break;
	}
	case TK_PLUS_PLUS:
	case TK_MINUS_MINUS: {
		if (!unary_op->factor->type->is_int()) {
			m_log.err(ERR_OP_CANNOT_APPLY_TO,
				      error_payload::op_cannot_apply({ unary_op->op, unary_op->factor->type }),
				      unary_op);
			unary_op->type = nyla::types::type_error;
			return;
		}
		unary_op->type = unary_op->factor->type;
		break;
	}
	case '!': {
		if (unary_op->factor->type != nyla::types::type_bool) {
			// TODO; report error!
			unary_op->type = nyla::types::type_error;
			return;
		}
		unary_op->type = nyla::types::type_bool;
		break;
	}
	default:
		assert(!"Handled unary check!");
		break;
	}
}

void nyla::analysis::check_ident(bool static_context, sym_scope* lookup_scope, nyla::aident* ident) {
	ident->literal_constant = false;

	// Assumed a variable at this point
	ident->sym_variable = m_sym_table->find_variable(lookup_scope, ident->ident_key);
	if (ident->sym_variable) {

		// This is only relevent if the variable is not a field/global
		if (!(ident->sym_variable->is_field || ident->sym_variable->is_global)) {
			if (ident->spos < ident->sym_variable->position_declared_at) {
				m_log.err(ERR_USE_OF_VARIABLE_BEFORE_DECLARATION,
					error_payload::word(ident->ident_key),
					ident);
			}
		}
		
		if (static_context) {
			if (ident->sym_variable->is_field) {
				m_log.err(ERR_ACCESSING_FIELD_FROM_STATIC_CONTEXT, ident);
			}
		}

		if (ident->sym_variable->sym_module != m_sym_module) {
			// Accessing an identifier in another module so have
			// to check the access modifiers
			switch (ident->sym_variable->mods & nyla::ACCESS_MODS) {
			case MOD_PRIVATE: {
				m_log.err(ERR_FIELD_NOT_VISIBLE, ident);
				break;
			}
			case MOD_PROTECTED: break; // TODO
			default: break; // Default public
			}
		}

		ident->type = ident->sym_variable->type;
	} else {
		m_log.err(ERR_UNDECLARED_VARIABLE,
			      error_payload::word(ident->ident_key),
			      ident);
		ident->type = nyla::types::type_error;
	}
}

void nyla::analysis::check_for_loop(nyla::afor_loop* for_loop) {
	for_loop->literal_constant = false;
	m_sym_scope = for_loop->sym_scope;
	for (nyla::avariable_decl* var_decl : for_loop->declarations) {
		check_expression(var_decl);
		if (var_decl->type == nyla::types::type_error) return;
		if (!var_decl->comptime_compat) {
			for_loop->comptime_compat = false;
		}
	}
	check_loop(dynamic_cast<nyla::aloop_expr*>(for_loop));
	m_sym_scope = m_sym_scope->parent;
}

void nyla::analysis::check_while_loop(nyla::awhile_loop* while_loop) {
	while_loop->literal_constant = false;
	m_sym_scope = while_loop->sym_scope;
	check_loop(dynamic_cast<nyla::aloop_expr*>(while_loop));
	m_sym_scope = m_sym_scope->parent;
}

void nyla::analysis::check_loop(nyla::aloop_expr* loop) {
	check_expression(loop->cond);
	if (loop->cond->type == nyla::types::type_error) return;
	if (!loop->cond->comptime_compat) {
		loop->comptime_compat = false;
	}
	if (loop->cond->type != nyla::types::type_bool) {
		m_log.err(ERR_EXPECTED_BOOL_COND, loop->cond);
	}
	check_scope(loop->body, loop->comptime_compat);
	for (nyla::aexpr* expr : loop->post_exprs) {
		check_expression(expr);
		if (!expr->comptime_compat) loop->comptime_compat = false;
	}
}

void nyla::analysis::check_type_cast(nyla::atype_cast* type_cast) {
	// TODO: ensure it is a valid cast
	check_expression(type_cast->value);
	if (!type_cast->comptime_compat) type_cast->comptime_compat = false;
}

void nyla::analysis::check_function_call(bool static_call,
	                                     sym_module* lookup_module,
	                                     nyla::afunction_call* function_call,
	                                     bool is_constructor) {
	
	// Checking types of arguments
	for (nyla::aexpr* argument : function_call->arguments) {
		check_expression(argument);
		if (argument->type == nyla::types::type_error) {
			function_call->type = nyla::types::type_error;
			return;
		}
		if (!argument->comptime_compat) function_call->comptime_compat = false;
	}

	std::vector<sym_function*> canidates;
	if (is_constructor) {
		canidates = m_sym_table->get_constructors(lookup_module);
	} else {
		canidates = m_sym_table->get_functions(lookup_module, function_call->name_key);;
	}

	nyla::sym_function* matched_function = find_best_canidate(canidates, function_call, is_constructor);

	if (matched_function == nullptr) {
		if (is_constructor) {
			m_log.err(ERR_COULD_NOT_FIND_CONSTRUCTOR,
				error_payload::function_call(function_call), function_call);
		} else {
			m_log.err(ERR_COULD_NOT_FIND_FUNCTION,
				error_payload::function_call(function_call), function_call);
		}
		function_call->type = nyla::types::type_error;
		return;
	}

	if (static_call) {
		if (matched_function->is_member_function()) {
			m_log.err(ERR_CALLED_NON_STATIC_FUNC_FROM_STATIC, function_call);
		}
	}

	// Accessing outside of the module so have to check
	// access modifiers
	if (matched_function->sym_module != m_sym_module) {
		switch (matched_function->mods & nyla::ACCESS_MODS) {
		case MOD_PRIVATE: {
			m_log.err(ERR_FUNCTION_NOT_VISIBLE, function_call);
			break;
		}
		case MOD_PROTECTED: break; // TODO
		default:            break; // Defaults to public
		}
	}

	check_function_call(matched_function, function_call);
}

nyla::sym_function* nyla::analysis::find_best_canidate(const std::vector<sym_function*>& canidates,
	                                                   nyla::afunction_call* function_call,
	                                                   bool is_constructor) {

	u32 least_conflicts = 214124124;
	s32 current_selection = -1;

	for (u32 i = 0; i < canidates.size(); i++) {
		const sym_function* canidate = canidates[i];
		// Making sure the function actually qualifies as a
		// canidate
		if (canidate->name_key != function_call->name_key) continue;
		if (canidate->param_types.size() != function_call->arguments.size()) continue;
		bool arguments_assignable = true;

		for (u32 j = 0; j < function_call->arguments.size(); j++) {
			if (!is_assignable_to(canidate->param_types[j], function_call->arguments[j]->type)) {
				arguments_assignable = false;
				break;
			}
		}
		if (!arguments_assignable) continue;

		u32 num_conflicts = 0;
		for (u32 j = 0; j < function_call->arguments.size(); j++) {
			if (function_call->arguments[j]->type != canidate->param_types[j]) {
				++num_conflicts;
			}
		}

		if (num_conflicts < least_conflicts) {
			least_conflicts = num_conflicts;
			current_selection = i;
		}
	}
	if (current_selection == -1) {
		return nullptr;
	}
	return canidates[current_selection];
}

void nyla::analysis::check_function_call(sym_function* called_function,
	                                     nyla::afunction_call* function_call) {
	
	function_call->type = called_function->return_type;
	function_call->called_function = called_function;

	for (u32 i = 0; i < called_function->param_types.size(); i++) {
		attempt_assignment(called_function->param_types[i], function_call->arguments[i]);
	}

	bool called_has_comptime = function_call->called_function->sym_module->mods & MOD_COMPTIME;

	function_call->comptime_compat  = called_has_comptime;
	function_call->literal_constant = called_has_comptime;
}

void nyla::analysis::check_array_access(bool static_context, sym_scope* lookup_scope, nyla::aarray_access* array_access) {
	array_access->literal_constant = false;

	check_ident(static_context, lookup_scope, array_access->ident);
	if (array_access->ident->type == nyla::types::type_error) {
		array_access->type = nyla::types::type_error;
		return;
	}

	if (!array_access->ident->comptime_compat) {
		array_access->comptime_compat = false;
	}

	nyla::type* type_at_index = array_access->ident->type;
	for (nyla::aexpr* index : array_access->indexes) {
		check_expression(index);
		if (index->type == nyla::types::type_error) {
			array_access->type = nyla::types::type_error;
			return;
		}
		if (!index->type->is_int()) {
			m_log.err(ERR_ARRAY_ACCESS_EXPECTS_INT, index);
			array_access->type = nyla::types::type_error;
			return;
		}

		if (!(type_at_index->is_arr() || type_at_index->is_ptr())) {
			// TODO: pass over type info
			m_log.err(ERR_ARRAY_ACCESS_ON_INVALID_TYPE, array_access);
			array_access->type = nyla::types::type_error;
			return;
		}

		if (!index->comptime_compat) array_access->comptime_compat = false;

		type_at_index = type_at_index->element_type;
	}
	if (array_access->indexes.size() >
		array_access->ident->type->arr_depth + array_access->ident->type->ptr_depth) {
		m_log.err(ERR_TOO_MANY_ARRAY_ACCESS_INDEXES, array_access);
		array_access->type = nyla::types::type_error;
		return;
	}

	if (array_access->indexes.size() == array_access->ident->type->arr_depth) {
		array_access->type = array_access->ident->type->get_base_type();
	} else {
		array_access->type = array_access->ident->type->get_sub_array(array_access->indexes.size());
	}
}

void nyla::analysis::check_array(nyla::aarray* arr, u32 depth) {
	arr->literal_constant = false;
	if (arr->type == nyla::types::type_error) return;

	bool        last_nesting_level = false;
	nyla::type* element_array_type = nullptr;
	for (nyla::aexpr* element : arr->elements) {
		if (element->tag == AST_ARRAY) {
			check_array(dynamic_cast<nyla::aarray*>(element), depth + 1);
			element_array_type = element->type;
		} else {
			last_nesting_level = true;
			check_expression(element);
		}

		if (element->type == nyla::types::type_error) {
			arr->type = nyla::types::type_error;
			return;
		}

		if (!element->comptime_compat) arr->comptime_compat = false;
	}

	if (last_nesting_level) {
		arr->type = nyla::type::get_arr(nyla::types::type_mixed);
	} else {
		arr->type = nyla::type::get_arr(element_array_type);
	}
}

void nyla::analysis::check_var_object(nyla::avar_object* var_object) {
	var_object->literal_constant = false;

	nyla::afunction_call* constructor_call = var_object->constructor_call;
	
	u32 module_name_key = var_object->constructor_call->name_key;
	sym_module* sym_module = m_file_unit->find_module(module_name_key);

	if (!sym_module) {
		m_log.err(ERR_COULD_NOT_FIND_MODULE_TYPE, var_object->constructor_call);
		var_object->type = nyla::types::type_error;
		return;
	}

	var_object->sym_module = sym_module;

	if (sym_module->no_constructors_found && constructor_call->arguments.empty()) {
		// Assumed there is a default constructor
		var_object->assumed_default_constructor = true;
		var_object->type = nyla::type::get_or_enter_module(sym_module);
	} else {
		check_function_call(false, sym_module, constructor_call, true);
		if (constructor_call->type == nyla::types::type_error) {
			var_object->type = nyla::types::type_error;
			return;
		}
		if (!constructor_call->comptime_compat) var_object->comptime_compat = false;
		var_object->type = nyla::type::get_or_enter_module(sym_module);
	}
}

void nyla::analysis::check_dot_op(nyla::adot_op* dot_op) {
	
	// TODO: literal_constant
	// TODO: comptime checking

#define LAST (idx+1 == dot_op->factor_list.size())

	sym_scope*  ref_scope  = m_sym_scope;
	sym_module* ref_module = m_sym_module;
	bool static_context = !m_function->sym_function->is_member_function();
	for (u32 idx = 0; idx < dot_op->factor_list.size(); idx++) {
		nyla::aexpr*  factor = dot_op->factor_list[idx];
		u32 factor_name_key;

		switch (factor->tag) {
		case AST_IDENT: {
			nyla::aident* ident = dynamic_cast<nyla::aident*>(factor);
			
			// Possible for the variable to be referencing a static
			// module but only if it is the first factor and there
			// does not already exist a variable by the name
			if (idx == 0) {
				// TODO: This does cause duplicate variable searches
				// (Although only on first search)
				sym_variable* sym_variable = m_sym_table->find_variable(m_sym_scope, ident->ident_key);
				if (!sym_variable) {
					sym_module* sym_module = m_file_unit->find_module(ident->ident_key);
					if (sym_module) {
						// References a static module
						ref_scope  = sym_module->scope;
						ref_module = sym_module;
						ident->references_module = true;
						continue; // Continuing to the next factor
					}
				}
			}

			check_ident(static_context, ref_scope, ident);
			factor_name_key = ident->ident_key;

			break;
		}
		case AST_ARRAY_ACCESS: {
			nyla::aarray_access* array_access = dynamic_cast<nyla::aarray_access*>(factor);
			check_array_access(static_context, ref_scope, array_access);
			factor_name_key = array_access->ident->ident_key;
			break;
		}
		case AST_FUNCTION_CALL: {
			nyla::afunction_call* function_call = dynamic_cast<nyla::afunction_call*>(factor);
			check_function_call(static_context, ref_module, function_call, false);
			factor_name_key = function_call->name_key;
			break;
		}
		case AST_THIS: {
			if (static_context) {
				m_log.err(ERR_CANNOT_USE_THIS_KEYWORD_IN_STATIC_CONTEXT, factor);
				dot_op->type = nyla::types::type_error;
				return;
			}
			if (idx != 0) {
				m_log.err(ERR_THIS_KEYWORD_MUST_COME_FIRST, factor);
				dot_op->type = nyla::types::type_error;
				return;
			}
			// Look up variables in the module scope
			ref_scope = m_sym_module->scope;
			continue; // Continue to next factor
		}
		default: {
			m_log.err(ERR_DOT_OP_EXPECTS_VARIABLE, factor);
			dot_op->type = nyla::types::type_error;
			return;
		}
		}

		if (factor->type == nyla::types::type_error) {
			dot_op->type = nyla::types::type_error;
			return;
		}

		if (!LAST) {
			if (factor->type->is_arr()) {
				// The only valid case is that the next
				// factor in the list is a length identifier
				nyla::aexpr* next_factor = dot_op->factor_list[idx + 1];
				bool is_dot_length = true;
				if (idx + 2 != dot_op->factor_list.size() || next_factor->tag != AST_IDENT) {
					is_dot_length = false;
				}
				if (is_dot_length) {
					nyla::aident* next_ident = dynamic_cast<nyla::aident*>(next_factor);
					if (next_ident->ident_key == nyla::length_ident) {
						next_ident->is_array_length = true;
						dot_op->type = nyla::types::type_uint; // Lengths are in uint
						return;
					} else {
						is_dot_length = false;
					}
				}
				if (!is_dot_length) {
					m_log.err(ERR_TYPE_DOES_NOT_HAVE_FIELD,
						      error_payload::word({ factor_name_key }),
						      factor);
					dot_op->type = nyla::types::type_error;
					return;
				}

			} else if (factor->type->is_module()) {
				// TODO: could also be a pointer to a module
				// 
				static_context = false;
				ref_module = factor->type->sym_module;
				ref_scope  = factor->type->sym_module->scope;
			} else {
				m_log.err(ERR_TYPE_DOES_NOT_HAVE_FIELD,
					      error_payload::word({ factor_name_key }),
					      factor);
				dot_op->type = nyla::types::type_error;
				return;
			}
		} else {
			// Is last so it takes on the type of the last factor
			dot_op->type = factor->type;
		}
	}
#undef LAST
}

void nyla::analysis::check_if(nyla::aif* ifstmt) {
	
	// Always false since you cant assign an ifstmt
	ifstmt->literal_constant = false;

	bool all_if_scopes_return = true;
	bool comptime = true;
	nyla::aif* cur_if = ifstmt;
	while (cur_if) {

		check_expression(cur_if->cond);
		if (cur_if->cond->type == nyla::types::type_error) return;
		if (cur_if->cond->type != nyla::types::type_bool) {
			m_log.err(ERR_EXPECTED_BOOL_COND, cur_if->cond);
		}
		if (!cur_if->cond->comptime_compat) comptime = false;

		m_sym_scope = cur_if->sym_scope;
		check_scope(cur_if->body, ifstmt->comptime_compat);
		if (!m_sym_scope->found_return) {
			all_if_scopes_return = false;
		}
		m_sym_scope = m_sym_scope->parent;

		if (cur_if->else_sym_scope) {
			m_sym_scope = cur_if->else_sym_scope;
			check_scope(cur_if->else_body, ifstmt->comptime_compat);
			if (!m_sym_scope->found_return) {
				all_if_scopes_return = false;
			}
			m_sym_scope = m_sym_scope->parent;
		} else {
			all_if_scopes_return = false;
		}

		cur_if = cur_if->else_if;
	}
	if (!comptime) {
		ifstmt->comptime_compat = false;
	}

	if (all_if_scopes_return) {
		m_sym_scope->found_return = true;
	}
}

void nyla::analysis::check_scope(const std::vector<nyla::aexpr*>& stmts, bool& comptime) {

	for (nyla::aexpr* stmt : stmts) {
		if (m_sym_scope->found_return) {
			// TODO: may need to mark the rest of the statements with nyla::types::type_error
			m_log.err(ERR_STMTS_AFTER_RETURN, stmt);
			break;
		}
		check_expression(stmt);
		if (!stmt->comptime_compat) {
			comptime = false;
		}
	}
}

/*
 * Utilities
 */

bool nyla::analysis::is_assignable_to(nyla::type* to, nyla::type* from) {
	switch (to->tag) {
		// is_int()
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_UBYTE:
	case TYPE_USHORT:
	case TYPE_UINT:
	case TYPE_ULONG:
	case TYPE_CHAR8:
	case TYPE_CHAR16:
	case TYPE_CHAR32: {
		if (from->is_int()) {  // int & int
			return to->mem_size() >= from->mem_size();
		}
		return false;
	}
	case TYPE_FLOAT:
	case TYPE_DOUBLE: {
		if (from->is_float()) {  // float & float
			return to->mem_size() >= from->mem_size();
		} else if (from->is_int()) {
			return true; // Can always assign integers to floats
		}
		return false;
	}
	case TYPE_BOOL: {
		return from->tag == TYPE_BOOL;
	}
	case TYPE_PTR: {
		if (from->is_ptr()) {  // ptr & ptr
			return to->ptr_depth == from->ptr_depth &&
				to->get_base_type() == from->get_base_type();
		} else if (from == nyla::types::type_null) { // ptr & null
			return true; // Pointers are always assignable null
		} else if (from->is_arr()) { // ptr & arr
			return to->ptr_depth == from->arr_depth &&
				to->get_base_type() == from->get_base_type();
		} else if (from == nyla::types::type_string) { // ptr & string
			return to->ptr_depth == 1
				&& to->get_base_type()->is_char();
		}
		return false;
	}
	case TYPE_ARR: {
		if (from->is_arr()) { // arr & arr
			return to->arr_depth == from->arr_depth &&
				(from->get_base_type()->tag == TYPE_MIXED ||
					to->get_base_type() == from->get_base_type());

		} else if (from == nyla::types::type_string) {
			return to->arr_depth == 1
				&& to->get_base_type()->is_char();
		}
		return false;
	}
	case TYPE_MODULE: {
		if (!from->is_module()) { // TODO: allow for implicit initialization!
			return false;
		}
		// TODO: need to check for inheritence cases
		return to->unique_module_key == from->unique_module_key;
	}
	default:
		assert(!"Unhandled case!");
		return false;
	}
}

nyla::aexpr* nyla::analysis::make_cast(nyla::aexpr* value, nyla::type* to_type) {
	if (value->type->equals(to_type)) {
		return value;
	}
	nyla::atype_cast* type_cast = make<nyla::atype_cast>(AST_TYPE_CAST, value);
	type_cast->value = value;
	type_cast->type = to_type;
	type_cast->comptime_compat = value->comptime_compat;
	return type_cast;
}

void nyla::analysis::attempt_assignment(nyla::type* to_type, nyla::aexpr*& value) {
	nyla::type* value_type = value->type;
	
	if (value_type == nyla::types::type_string) {
		// Strings become the type they are being set to
		value->type = to_type;
	} else if (value_type->is_arr() && to_type->is_arr()) {
		if (value->tag == AST_ARRAY) {
			// Checking to make sure that the elements of the array are assignable
			// to the destination array. If they are then cast will be added where
			// needed
			attempt_array_assignment(to_type->get_base_type(), dynamic_cast<nyla::aarray*>(value));
		}
		
		value->type->set_base_type(to_type->get_base_type());
	} else if (value_type == nyla::types::type_null) {
		value->type = to_type; // Replacing null type with the type of the pointer
	} else if (value_type != to_type) {
		value = make_cast(value, to_type);
	}
}

bool nyla::analysis::attempt_array_assignment(nyla::type* to_element_type, nyla::aarray* arr) {
	for (nyla::aexpr*& element : arr->elements) {
		if (element->tag == AST_ARRAY) {
			if (!attempt_array_assignment(to_element_type, dynamic_cast<nyla::aarray*>(element))) {
				return false;
			}
		} else {
			if (is_assignable_to(to_element_type, element->type)) {
				attempt_assignment(to_element_type, element);
			} else {
				m_log.err(ERR_ELEMENT_OF_ARRAY_NOT_COMPATIBLE_WITH_ARRAY,
					      error_payload::types({ to_element_type, element->type }),
					      element);
				return false;
			}
		}
	}
	return true;
}

bool nyla::analysis::is_lvalue(nyla::aexpr* expr) {

	// lvalue includes
	// 1. identifiers
	// 2. array accesses
	// 3. dot ops (except for .length or a function call that returns void)
	// 4. function calls except if they return void (is this accurate?)


	// TODO: implement!!
	return true;
}

bool nyla::analysis::compare_arr_size(nyla::aarray* arr,
	                                  const std::vector<u32>& computed_arr_dim_sizes,
	                                  u32 depth) {
	if (arr->elements.size() > computed_arr_dim_sizes[depth]) {
		m_log.err(ERR_ARR_TOO_MANY_INIT_VALUES, arr);
		return false;
	}
	arr->dim_size = computed_arr_dim_sizes[depth];
	if (arr->type->element_type->tag == TYPE_ARR) {
		for (nyla::aexpr* element : arr->elements) {
			nyla::aarray* arr_element = dynamic_cast<nyla::aarray*>(element);
			if (!compare_arr_size(arr_element, computed_arr_dim_sizes, depth + 1)) {
				return false;
			}
		}
	}
	return true;
}

bool nyla::analysis::compare_arr_size(nyla::astring* str,
	                                  const std::vector<u32>& computed_arr_dim_sizes,
	                                  u32 depth) {
	// TODO: only comparing against 8 bit strings
	if (str->lit8.length() > computed_arr_dim_sizes[depth]) {
		m_log.err(ERR_ARR_TOO_MANY_INIT_VALUES, str);
	}
	str->dim_size = computed_arr_dim_sizes[depth];
	return true;
}
