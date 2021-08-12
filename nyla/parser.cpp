#include "parser.h"

#include <unordered_map>

using namespace nyla;

parser::parser(nyla::lexer& lexer, nyla::sym_table& sym_table, nyla::log& log)
	: m_lexer(lexer), m_sym_table(sym_table), m_log(log)
{
	// Read first token to get things started.
	next_token();
}

nyla::afile_unit* nyla::parser::parse_file_unit() {
	nyla::afile_unit* file_unit = make_node<nyla::afile_unit>(AST_FILE_UNIT, m_current, m_current);
	while (m_current->tag != TK_EOF) {
		switch (m_current->tag) {
		case TK_TYPE_BYTE:  case TK_TYPE_SHORT:
		case TK_TYPE_INT:   case TK_TYPE_LONG:
		case TK_TYPE_FLOAT: case TK_TYPE_DOUBLE:
		case TK_TYPE_BOOL:  case TK_TYPE_VOID: {
			file_unit->functions.push_back(parse_function());
			break;
		}
		case TK_DLLIMPORT:
			parse_dll_import();
			break;
		case TK_EXTERNAL:
			file_unit->functions.push_back(parse_external_function());
			break;
		default:
			// TODO: report error
			next_token();
			break;
		}
	}
	return file_unit;
}

nyla::afunction* parser::parse_function_decl(bool make_new_scope) {
	// TODO: parse function modifiers (public, private access, ect..)
	nyla::type* return_type = parse_type();

	nyla::afunction* function = make_node<nyla::afunction>(AST_FUNCTION, m_current, m_current);
	function->return_type = return_type;
	function->ident        = parse_identifier();
	
	// Needed here so that the parameters declarations are registered
	// in the function scope.
	if (make_new_scope) {
		nyla::ascope* scope = m_sym_table.push_scope(m_current);
		function->scope = scope;
	}

	match('(');
	if (m_current->tag != ')') {
		// Assuming parameters.

		bool more_parameters = false;
		do {
			nyla::avariable_decl* parameter = parse_variable_decl(make_new_scope);
			function->parameters.push_back(parameter);

			more_parameters = m_current->tag == ',';
			if (more_parameters) {
				next_token(); // consuming ,
			}
		} while (more_parameters);
	}
	match(')');

	return function;
}

nyla::afunction* nyla::parser::parse_function() {
	nyla::afunction* function = parse_function_decl();
	m_sym_table.store_function_decl(function);
	m_found_ret = false;
	match('{');
	while (m_current->tag != '}' && m_current->tag != TK_EOF) {
		if (m_current->tag == ';') {
			match_semis(); // Could be semis without a statement
			continue;
		}
		function->scope->stmts.push_back(parse_function_stmt());
	}
	match('}');
	if (!m_found_ret && function->return_type->tag == TYPE_VOID) {
		nyla::areturn* void_ret = make_node<nyla::areturn>(AST_RETURN, m_current, m_current);
		function->scope->stmts.push_back(void_ret);
	}
	m_sym_table.pop_scope();
	return function;
}

nyla::afunction* nyla::parser::parse_external_function() {
	next_token(); // Parsing external
	nyla::afunction* function = parse_function_decl(false);
	m_sym_table.store_function_decl(function);
	function->is_external = true;
	match_semis();
	return function;
}

nyla::type* parser::parse_type() {
	nyla::type* type = nyla::type::get_error();
	nyla::token* st = m_current;
	switch (m_current->tag) {
	case TK_TYPE_BYTE:   type = nyla::type::get_byte();   next_token(); break;
	case TK_TYPE_SHORT:  type = nyla::type::get_short();  next_token(); break;
	case TK_TYPE_INT:    type = nyla::type::get_int();    next_token(); break;
	case TK_TYPE_LONG:   type = nyla::type::get_long();   next_token(); break;
	case TK_TYPE_FLOAT:  type = nyla::type::get_float();  next_token(); break;
	case TK_TYPE_DOUBLE: type = nyla::type::get_double(); next_token(); break;
	case TK_TYPE_BOOL:   type = nyla::type::get_bool();   next_token(); break;
	case TK_TYPE_CHAR16: type = nyla::type::get_char16(); next_token(); break;
	case TK_TYPE_VOID:   type = nyla::type::get_void();   next_token(); break;
	default:
		produce_error(
			ERR_CANNOT_RESOLVE_TYPE,
			error_data::make_str_literal_load(m_current->to_string().c_str()),
			m_current, m_current
		);
		break;
	}
	if (m_current->tag == '*') {
		match('*');
		nyla::type* fst_ptr_type = nyla::type::get_ptr(nullptr);
		nyla::type* cur_ptr_type = fst_ptr_type;
		while (m_current->tag == '*') {
			match('*');
			nyla::type* ptr_type = nyla::type::get_ptr(nullptr);
			cur_ptr_type->elem_type = ptr_type;
			cur_ptr_type = ptr_type;
		}
		cur_ptr_type->elem_type = type;
		type = fst_ptr_type;
		type->calculate_ptr_depth();
	}
	if (m_current->tag == '[') {
		// fst
		// match(token1)
		// process( fst )
		// match(token2)
		// cur = fst
		// while (token1)
		// match(token1)
		// process( nth )
		// cur.ptr = nth
		// cur = nth
		// match(token2)
		m_recovery_accept_brackets = true;
		match('[');
		nyla::type* fst_arr_type = nyla::type::get_arr(nullptr, m_current->tag != ']' ?
			parse_expression() : nullptr);
		match(']');
		nyla::type* cur_arr_type = fst_arr_type;
		while (m_current->tag == '[') {
			match('[');
			nyla::type* arr_type = nyla::type::get_arr(nullptr, m_current->tag != ']' ?
				parse_expression() : nullptr);
			cur_arr_type->elem_type = arr_type;
			cur_arr_type = arr_type;
			match(']');
		}
		cur_arr_type->elem_type = type;
		type = fst_arr_type;
		type->calculate_arr_depth();
		/*if (array_depths.size() > MAX_MEM_DEPTH) {
			produce_error(ERR_TOO_MANY_ARRAY_SUBSCRIPTS, nullptr, st, m_current);
			array_depths.resize(MAX_MEM_DEPTH);
		}
		if (m_error_recovery) {
			type = nyla::type::get_error();
		} else {
			type = type->as_arr(array_depths);
		}*/
	}
	type->st = st;
	type->et = m_current;
	return type;
}

nyla::aidentifier* parser::parse_identifier() {
	nyla::aidentifier* identifier = make_node<nyla::aidentifier>(AST_IDENTIFIER, m_current, m_current);
	if (m_current->tag == TK_IDENTIFIER) {
		nyla::name name = dynamic_cast<nyla::word_token*>(m_current)->name;
		next_token(); // Consuming identifier
		identifier->name = name;
		return identifier;
	}
	produce_error(ERR_EXPECTED_IDENTIFIER,
		error_data::make_empty_load(),
		look_back(1), look_back(1));
	identifier->tag = AST_ERROR;
	return identifier;
}

nyla::avariable_decl* nyla::parser::parse_variable_decl(bool has_scope) {
	return parse_variable_decl(parse_type(), has_scope);
}

nyla::avariable_decl* nyla::parser::parse_variable_decl(nyla::type* type, bool has_scope) {
	nyla::aidentifier* identifier = parse_identifier();
	nyla::avariable* variable = make_node<nyla::avariable>(AST_VARIABLE, identifier->st, identifier->et);
	variable->checked_type = type;	
	variable->ident = identifier;
	nyla::avariable_decl* variable_delc =
		make_node<nyla::avariable_decl>(AST_VARIABLE_DECL, type->st, variable->st);
	variable_delc->variable = variable;
	variable_delc->checked_type = variable->checked_type;

	if (identifier->tag == AST_ERROR) {
		return variable_delc;
	}

	if (has_scope) {
		if (m_sym_table.has_been_declared(variable)) {
			produce_error(ERR_VARIABLE_REDECLARATION,
				error_data::make_str_literal_load(variable->ident->name.c_str()),
				type->st, variable->st);
		} else {
			m_sym_table.store_declared_variable(variable);
		}
	}
	return variable_delc;
}

std::vector<avariable_decl*> nyla::parser::parse_variable_assign_list() {
	std::vector<avariable_decl*> decl_list;
	nyla::type* fst_type = parse_type();
	bool more_decls = false;
	do {
		nyla::avariable_decl* var_decl = parse_variable_decl(fst_type);
		if (m_current->tag != ';' && m_current->tag != ',')
			var_decl->assignment = parse_expression(var_decl->variable->ident);
		decl_list.push_back(var_decl);
		more_decls = m_current->tag == ',';
		if (more_decls) {
			next_token(); // consuming ,
		}
	} while (more_decls);
	return decl_list;
}

/*-----------------------------*\
 *     STATEMENT PARSING       *
\*-----------------------------*/

nyla::aexpr* nyla::parser::parse_function_stmt() {
	if (m_current->tag == ';') {
		match_semis();
	}
	nyla::aexpr* expr = parse_function_stmt_rest();
	m_error_recovery = false;
	return expr;
}

nyla::aexpr* parser::parse_function_stmt_rest() {
	switch (m_current->tag) {
	case TK_RETURN: {
		nyla::areturn* ret = make_node<nyla::areturn>(AST_RETURN, m_current, m_current);
		next_token(); // Consuming return token.
		m_found_ret = true;
		if (m_current->tag == ';') {
			match_semis();
			// Must be a void return.
			return ret;
		}
		ret->value = parse_expression();
		match_semis();
		return ret;
	}
	case TK_FOR: {
		return parse_for_loop();
	}
	case TK_TYPE_BYTE:  case TK_TYPE_SHORT: 
	case TK_TYPE_INT:   case TK_TYPE_LONG:  
	case TK_TYPE_FLOAT: case TK_TYPE_DOUBLE:
	case TK_TYPE_BOOL:  case TK_TYPE_VOID:
	case TK_TYPE_CHAR16: {
		nyla::avariable_decl* variable_decl = parse_variable_decl();
		// int a  ;
		//        ^
		//        |-- no assignment
		if (m_current->tag != ';') {
			match('=', false);
			variable_decl->assignment = parse_expression(variable_decl->variable->ident);
		}
		if (!m_error_recovery) {
			match_semis();
		}
		return variable_decl;
	}
	case TK_IDENTIFIER: {
		// If followed by another identifier the first
		// identifier is assumed to be a type. Otherwise
		// it is assumed to be part of an expression
		nyla::token* next_token = peek_token(1);
		if (next_token->tag == TK_IDENTIFIER) {
			// TODO: Handle variable declaration!
		} else {
			nyla::aexpr* expr = parse_expression();
			match_semis();
			return expr;
		}
		break;
	}
	default: {
		produce_error(
			ERR_EXPECTED_STMT,
			error_data::make_token_load(m_current),
			m_current, m_current
		);
		next_token(); // Consuming the token that doesn't belong
		return make_node<nyla::err_expr>(AST_ERROR, m_current, m_current);
	}
	}
}

nyla::aexpr* nyla::parser::parse_for_loop() {

#define CASE_ERR_RET_AST_ERR() \
if (m_error_recovery &&        \
    m_current->tag != ';') {   \
	loop->tag = AST_ERROR;     \
    m_sym_table.pop_scope();   \
	return loop;               \
}
// for expr? ; expr? ; expr?
//      ^
//      |
///  error! AND not current = ';'
//   then must be a different statement!
//   stop processing for loop

	nyla::afor_loop* loop = make_node<nyla::afor_loop>(AST_FOR_LOOP, m_current, m_current);
	// Need to create a new scope early so declarations are placed
	// there instead of on the outside of the loop
	loop->scope = m_sym_table.push_scope(m_current);
	
	next_token(); // Consuming for token.
	if (m_current->tag != ';') {
		loop->declarations = parse_variable_assign_list();
	}
	CASE_ERR_RET_AST_ERR()
	match(';');
	if (m_current->tag != ';') {
		loop->cond = parse_expression();
	} else {
		// If there is no expression it is just assumed to always be true!
		abool* always_true = make_node<nyla::abool>(AST_VALUE_BOOL, m_current, m_current);
		always_true->tof = true;
		loop->cond = always_true;
	}
	CASE_ERR_RET_AST_ERR()
	match(';');
	nyla::aexpr* post = nullptr;
	if (m_current->tag != '{') {
		post = parse_expression();
	}
	if (m_current->tag == '{') {
		match('{');
		while (m_current->tag != '}' && m_current->tag != TK_EOF) {
			if (m_current->tag == ';') {
				match_semis(); // Could be semis without a statement
				continue;
			}
			loop->scope->stmts.push_back(parse_function_stmt());
		}
		match('}');
	} else {
		// Single statement
		//   for expr ; expr ; expr   stmt
		loop->scope->stmts.push_back(parse_function_stmt());
	}
	if (post) {
		loop->scope->stmts.push_back(post);
	}
	m_sym_table.pop_scope();
	return loop;
#undef CASE_ERR_RET_AST_ERR
}

/*-----------------------------*\
 *     EXPRESSION PARSING      *
\*-----------------------------*/

std::unordered_set<u32> binary_ops_set = {
	'+', '-', '*', '/', '=', '<', '>',
	TK_PLUS_EQ, TK_MINUS_EQ, TK_DIV_EQ, TK_MUL_EQ,
};
std::unordered_map<u32, u32> precedence = {
	{ '=', 0 },
	{ TK_PLUS_EQ, 0 },
	{ TK_MINUS_EQ, 0 },
	{ TK_DIV_EQ, 0 },
	{ TK_MUL_EQ, 0 },

	{ '<', 1 },
	{ '>', 1 },

	{ '+', 2 },
	{ '-', 2 },

	{ '*', 3 },
	{ '/', 3 },
};

nyla::aexpr* nyla::parser::parse_factor() {
	switch (m_current->tag) {
	case TK_VALUE_INT: {
		nyla::anumber* number = make_node<nyla::anumber>(AST_VALUE_INT, m_current, m_current);
		number->value_int = dynamic_cast<nyla::num_token*>(m_current)->value_int;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_FLOAT: {
		nyla::anumber* number = make_node<nyla::anumber>(AST_VALUE_FLOAT, m_current, m_current);
		number->value_float = dynamic_cast<nyla::num_token*>(m_current)->value_float;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_DOUBLE: {
		nyla::anumber* number = make_node<nyla::anumber>(AST_VALUE_DOUBLE, m_current, m_current);
		number->value_double = dynamic_cast<nyla::num_token*>(m_current)->value_double;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_VALUE_BOOL: {
		nyla::abool* boolean = make_node<nyla::abool>(AST_VALUE_BOOL, m_current, m_current);
		boolean->tof = dynamic_cast<nyla::bool_token*>(m_current)->tof;
		next_token(); // Consuming the bool token.
		return boolean;
	}
	case TK_STRING_VALUE: {
		nyla::astring* str = make_node<nyla::astring>(AST_VALUE_STRING, m_current, m_current);
		str->lit = dynamic_cast<nyla::string_token*>(m_current)->lit;
		next_token(); // Consuming the string token.
		return str;
	}
	case TK_NULL: {
		nyla::anull* null_value = make_node<nyla::anull>(AST_VALUE_NULL, m_current, m_current);
		next_token(); // Consuming the null token.
		return null_value;
	}
	case '(': {
		match('(');
		nyla::aexpr* expr = parse_expression();
		match(')');
		return expr;
	}
	case TK_CAST: {
		next_token(); // consuming 'cast' keyword
		nyla::atype_cast* type_cast = make_node<nyla::atype_cast>(AST_TYPE_CAST, m_current, m_current);
		match('(');
		type_cast->checked_type = parse_type();
		match(')');
		type_cast->value = parse_expression();
		return type_cast;
	}
	case TK_IDENTIFIER: {
		nyla::token* identifier_token = m_current;
		nyla::aidentifier* identifier = parse_identifier();
		
		switch (m_current->tag) { 
		case '(':
			return parse_function_call(identifier, identifier_token);
		case '[':
			return parse_array_access(identifier, identifier_token);
		default:
			break;
		}
		
		// The type is determined during symantic analysis.
		return identifier;
	}
	case '{': {
		return parse_array();
	}
	default: {
		produce_error(ERR_EXPECTED_FACTOR,
			error_data::make_token_tag_load(m_current->tag),
			m_current, m_current
		);
		skip_recovery();
		return make_node<nyla::err_expr>(AST_ERROR, m_current, m_current);
	}
	}
}

nyla::aexpr* parser::parse_unary() {
	switch (m_current->tag) {
	case '-': {
		next_token(); // Consuming -
		nyla::aexpr* factor = parse_factor();
		nyla::aunary_op* unary_op = make_node<nyla::aunary_op>(AST_UNARY_OP, m_current, factor->st);
		unary_op->op = '-';
		unary_op->factor = factor;
		if (m_current->tag == '.') {
			nyla::aexpr* dot_op = parse_dot_op(factor);
			unary_op->factor = dot_op;
		}
		return unary_op;
	}
	default: {
		nyla::aexpr* factor = parse_factor();
		switch (m_current->tag) {
		case '.':
			return parse_dot_op(factor);
		default:
			return factor;
		}
	}
	}
}

nyla::aexpr* parser::on_binary_op(nyla::token* op_token,
	                              nyla::aexpr* lhs, nyla::aexpr* rhs) {
	// Subdivides an equal + operator into seperate nodes.
	static auto equal_and_op_apply = [this, op_token](u32 op, nyla::aexpr* lhs, nyla::aexpr* rhs) -> nyla::abinary_op* {
		nyla::abinary_op* eq_op = make_node<nyla::abinary_op>(AST_BINARY_OP, lhs->et, rhs->et);
		nyla::abinary_op* op_op = make_node<nyla::abinary_op>(AST_BINARY_OP, lhs->et, rhs->et);
		eq_op->op  = '=';
		op_op->op  = op;
		op_op->lhs = lhs;
		op_op->rhs = rhs;
		eq_op->lhs = lhs;
		eq_op->rhs = op_op;
		return eq_op;
	};
	switch (op_token->tag) {
	case TK_PLUS_EQ:  return equal_and_op_apply('+', lhs, rhs);
	case TK_MINUS_EQ: return equal_and_op_apply('-', lhs, rhs);
	case TK_DIV_EQ:   return equal_and_op_apply('/', lhs, rhs);
	case TK_MUL_EQ:   return equal_and_op_apply('*', lhs, rhs);
	default: {
		// Letting folding occure via LLVM folding operation. Could handle it here
		// which may result in faster compilation.
		nyla::abinary_op* binary_op = make_node<nyla::abinary_op>(AST_BINARY_OP, lhs->et, rhs->et);
		binary_op->lhs = lhs;
		binary_op->rhs = rhs;
		binary_op->op = op_token->tag;
		return binary_op;
	}
	}
}

nyla::aexpr* parser::parse_expression() {
	return parse_expression(parse_unary());
}

nyla::aexpr* parser::parse_expression(nyla::aexpr* lhs) {
	
	nyla::token* op = m_current;
	nyla::token* next_op;

	struct stack_unit {
		nyla::token* op;
		nyla::aexpr* expr;
	};
	std::stack<stack_unit> op_stack;

	while (binary_ops_set.find(op->tag) != binary_ops_set.end()) {
		next_token(); // Consuming the operator.
	
		nyla::aexpr* rhs = parse_unary();
		
		next_op = m_current;
		bool more_operators = binary_ops_set.find(next_op->tag) != binary_ops_set.end();
		if (more_operators && precedence[next_op->tag] > precedence[op->tag]) {
			// Delaying the operation until later since the next operator has a
			// higher precedence.
			stack_unit unit = { op, lhs };
			op_stack.push(unit);
			lhs = rhs;
			op  = next_op;

		} else {
			// Apply the binary operator!
			nyla::aexpr* res = on_binary_op(op, lhs, rhs);
			lhs = res;

			while (!op_stack.empty()) {
				rhs = lhs;
				stack_unit unit = op_stack.top();
				// Still possible to have the right side have higher precedence.
				if (more_operators && precedence[next_op->tag] > precedence[unit.op->tag]) {
					lhs = rhs;
					op  = next_op;
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

nyla::afunction_call* parser::parse_function_call(nyla::aidentifier* name, nyla::token* start_token) {
	nyla::afunction_call* function_call = make_node<nyla::afunction_call>(
		AST_FUNCTION_CALL, start_token, start_token);
	function_call->ident = name;
	match('(');
	if (m_current->tag != ')') {
		bool more_expressions = false;
		do {
			function_call->parameter_values.push_back(
				parse_expression()
			);
			
			more_expressions = m_current->tag == ',';
			if (more_expressions) {
				next_token(); // Consuming ,
			}
		} while (more_expressions);
	}
	match(')');
	return function_call;
}

nyla::aarray* parser::parse_array() {
	// NOTE OF WARNING: Before returning on this function
	// the function should ALWAYS decrement m_array_depth if
	// it has been incremented
	
	nyla::token* start_token = m_current;
	nyla::aarray* arr = make_node<nyla::aarray>(AST_ARRAY, m_current, m_current);
	match('{');
	if (m_array_depth == 0) {
		m_array_too_deep = false;
		m_array_depth_ptr = 0;
		for (u32 i = 0; i < MAX_MEM_DEPTH; i++) {
			m_max_array_depths[i] = 0;
		}
	}
	++m_array_depth;
	if (m_array_depth > m_array_depth_ptr) {
		m_array_depth_ptr = m_array_depth;
	}
	if (m_array_depth_ptr > 8) {
		m_array_depth_ptr = 8;
	}
	bool more_elements = false;
	if (m_current->tag != '}') {
		do {
			nyla::aexpr* element = parse_expression();
			arr->elements.push_back(element);
			more_elements = m_current->tag == ',';
			if (more_elements) {
				next_token(); // Consuming ,
			}
		} while (more_elements);
	}
	--m_array_depth;
	
	if (m_array_depth > MAX_MEM_DEPTH - 1) {
		m_array_too_deep = true;
	} else {
		if (arr->elements.size() > m_max_array_depths[m_array_depth]) {
			m_max_array_depths[m_array_depth] = arr->elements.size();
		}
	}
	if (m_array_depth == 0) {
		if (m_array_too_deep) {
			produce_error(ERR_ARRAY_TOO_DEEP, nullptr, arr->st, m_current);
			arr->checked_type = nyla::type::get_error();
		}

		for (u32 i = 0; i < m_array_depth_ptr; i++) {
			arr->depths.push_back(m_max_array_depths[i]);
		}
	}
	arr->et = m_current;
	match('}');
	return arr;
}

nyla::abinary_op* parser::parse_dot_op(nyla::aexpr* lhs) {
	match('.');
	nyla::aexpr* rhs = parse_factor();
	nyla::abinary_op* dot_op = make_node<nyla::abinary_op>(AST_BINARY_OP, lhs->et, rhs->et);
	dot_op->op = '.';
	dot_op->lhs = lhs;
	dot_op->rhs = rhs;
	while (m_current->tag == '.') {
		match('.');
		rhs = parse_factor();
		nyla::abinary_op* next_dot_op = make_node<nyla::abinary_op>(AST_BINARY_OP, dot_op->et, rhs->et);
		next_dot_op->op = '.';
		next_dot_op->rhs = dot_op;
		next_dot_op->lhs = rhs;
		dot_op = next_dot_op;
	}
	return dot_op;
}

nyla::aarray_access* parser::parse_array_access(nyla::aidentifier* name, nyla::token* start_token) {
	nyla::aarray_access* fst_array_access = make_node<nyla::aarray_access>(
		AST_ARRAY_ACCESS, start_token, start_token);
	
	match('[');
	fst_array_access->index = parse_expression();
	fst_array_access->et = m_current;
	fst_array_access->ident = name;
	match(']');
	
	nyla::aarray_access* current = fst_array_access;
	while (m_current->tag == '[') {
		match('[');
		nyla::aarray_access* array_access = make_node<nyla::aarray_access>(
			AST_ARRAY_ACCESS, m_current, start_token);
		array_access->index = parse_expression();
		array_access->et = m_current;
		array_access->ident = name;
		current->next = array_access;
		current = array_access;
		match(']');
	}

	m_recovery_accept_brackets = false;
	return fst_array_access;
}

void nyla::parser::parse_dll_import() {
	next_token(); // Consuming dllimport
	match('(');
	if (m_current->tag != TK_STRING_VALUE) {
		// TODO: report error!
	}
	std::string dll_import_path = dynamic_cast<nyla::string_token*>(m_current)->lit;
	m_sym_table.import_dll(dll_import_path);
	next_token(); // Consuming the path
	match(')');
	match_semis();
}

nyla::token* parser::peek_token(u32 n) {
	if (n == 0) {
		assert(!"There is no reason to peek zero tokens");
	}
	for (u32 i = 0; i < n; i++) {
		m_saved_tokens.push(m_lexer.next_token());
	}
	return m_saved_tokens.back();
}

void parser::next_token() {
	if (!m_saved_tokens.empty()) {
		m_current = m_saved_tokens.front();
		m_saved_tokens.pop();
	} else {
		m_current = m_lexer.next_token();
	}
	m_processed_tokens.push_back(m_current);
}

bool nyla::parser::match(u32 token_tag, bool consume) {
	if (m_current->tag == token_tag) {
		if (consume) {
			next_token();
		}
		return true;
	} else {
		produce_error(ERR_EXPECTED_TOKEN,
			expected_token_data::make_expect_tk_tag(token_tag, m_current),
			look_back(1), look_back(1));
		return false;
	}
}

void parser::match_semis() {
	match(';');
	while (m_current->tag == ';') {
		next_token(); // Consuming any additional semis.
	}
}

void nyla::parser::skip_recovery() {
	m_error_recovery = true;
	// Eating tokens till we find something sensical that
	// could begin another statement.
	while (true) {
		switch (m_current->tag) {
		case '[':
		case ']': {
			if (m_recovery_accept_brackets) {
				return;
			}
			break;
		}
		case ';':
		
			// TODO: might not always want to stop here?
		case '{': case '}':

		case TK_EOF:
			// Statement keywords
		case TK_FOR:
		case TK_IF:
		case TK_WHILE:
		case TK_SWITCH:
			// Types
		case TK_TYPE_BYTE:
		case TK_TYPE_SHORT:
		case TK_TYPE_INT:
		case TK_TYPE_LONG:
		case TK_TYPE_FLOAT:
		case TK_TYPE_DOUBLE:
		case TK_TYPE_BOOL:
		case TK_TYPE_VOID:
		case TK_TYPE_CHAR16:
			return;
		default:
			next_token();
		}
	}
}

nyla::token* parser::look_back(s32 n) {
	s32 back = m_processed_tokens.size() - n - 1;
	if (back < 0) {
		return m_current;
	}
	return m_processed_tokens[back];
}

void parser::produce_error(error_tag tag, error_data* data,
	                             nyla::token* start_token, nyla::token* end_token) {
	m_log.error(tag, data, start_token, end_token);
}
