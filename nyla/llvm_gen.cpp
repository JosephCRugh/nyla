#include "llvm_gen.h"

struct ll_vtype_printer {
	ll_vtype_printer(llvm::Value* _arg)
		: arg(_arg) {}

	llvm::Value* arg;
	void print_ll_type(llvm::Value* value) const {
		value->getType()->print(llvm::outs());
	}
};

struct ll_type_printer {
	ll_type_printer(llvm::Type* _arg)
		: arg(_arg) {}

	llvm::Type* arg;
	void print_ll_type(llvm::Type* ll_type) const {
		ll_type->print(llvm::outs());
	}
};

std::ostream& operator<<(std::ostream& os, const ll_vtype_printer& ll_printer) {
	ll_printer.print_ll_type(ll_printer.arg);
	return os;
}

std::ostream& operator<<(std::ostream& os, const ll_type_printer& ll_printer) {
	ll_printer.print_ll_type(ll_printer.arg);
	return os;
}


nyla::llvm_generator::~llvm_generator() {
	delete m_llvm_builder;
}

nyla::llvm_generator::llvm_generator(nyla::compiler& compiler, llvm::Module* llvm_module,
	                                 nyla::afile_unit* file_unit, bool print)
	: m_compiler(compiler), m_llvm_module(llvm_module), m_print(print), m_file_unit(file_unit) {
	m_llvm_builder = new llvm::IRBuilder<>(*llvm_context);
}

void nyla::llvm_generator::gen_file_unit() {
	for (nyla::amodule* nmodule : m_file_unit->modules) {
		gen_module(nmodule);
	}
}

void nyla::llvm_generator::gen_declarations() {
	for (nyla::amodule* nmodule : m_file_unit->modules) {

		// Creating the type of the struct for the module
		std::vector<llvm::Type*> ll_struct_types;
		llvm::StructType* ll_struct_type = llvm::StructType::create(*llvm_context);
		nmodule->sym_module->ll_struct_type = ll_struct_type;

		for (nyla::avariable_decl* field : nmodule->fields) {
			ll_struct_types.push_back(gen_type(field->type));
		}

		if (ll_struct_types.empty()) {
			ll_struct_types.push_back(llvm::Type::getInt8Ty(*llvm_context));
		}
		ll_struct_type->setBody(ll_struct_types);

		if (m_print) {
			ll_struct_type->setName(get_word(nmodule->name_key).c_str());
		}
		

		if (m_print) {
			ll_struct_type->print(llvm::outs());
			std::cout << "\n\n";
		}

		for (nyla::afunction* constructor : nmodule->constructors) {
			gen_function_declaration(constructor);
		}
		for (nyla::afunction* function : nmodule->functions) {
			gen_function_declaration(function);
		}
	}
}

void nyla::llvm_generator::gen_global_initializers(sym_function* sym_main_function,
	                                               const std::vector<nyla::avariable_decl*>& initializer_expressions) {
	m_initializing_globals = true;
	llvm::BasicBlock* ll_main_bb = &sym_main_function->ll_function->getEntryBlock();
	m_llvm_builder->SetInsertPoint(&ll_main_bb->front());
	for (nyla::avariable_decl* global_initializer : initializer_expressions) {
		// Need to GEP into parts of the structure!

		gen_variable_decl(global_initializer, false);
	}
	m_initializing_globals = false;
}

void nyla::llvm_generator::gen_module(nyla::amodule* nmodule) {
	// Initializing what part of global variables that can
	// be initialized then passing the data off to the global
	// initializer expressions if they need further initialization
	
	// For any variable either shove it into global
	// memory somewhere or add it to the global initializer
	// list of expressions, and for some variables of some
	// types it must do both

	for (nyla::avariable_decl* global : nmodule->globals) {
		global->sym_variable->ll_alloc = gen_global_variable(global);

		if (m_print) {
			global->sym_variable->ll_alloc->print(llvm::outs());
			std::cout << '\n';
		}
	}

	for (nyla::afunction* constructor : nmodule->constructors) {
		gen_function_body(constructor);
	}
	for (nyla::afunction* function : nmodule->functions) {
		gen_function_body(function);
	}
}

void nyla::llvm_generator::gen_function_declaration(nyla::afunction* function) {

	llvm::Type* ll_return_type = gen_type(function->return_type);
	std::vector<llvm::Type*> ll_parameter_types;

	// First parameter of member functions are is the pointer to the object
	if (function->sym_function->is_member_function()) {
		ll_parameter_types.push_back(
			llvm::PointerType::get(function->sym_function
				                           ->sym_module
				                           ->ll_struct_type, 0));
	}

	for (nyla::avariable_decl* param : function->parameters) {
		ll_parameter_types.push_back(gen_type(param->type));
	}

	bool is_var_args = false;
	llvm::FunctionType* ll_function_type =
		llvm::FunctionType::get(ll_return_type, ll_parameter_types, is_var_args);

	std::string function_name;
	
	if (function->is_constructor) {
		function_name = "_C";
		function_name += get_word(function->name_key).c_str();
	} else {
		function_name = get_word(function->name_key).c_str();
	}

	// Mangling the name by parameter types
	if (!function->is_main_function && !function->is_external()) {
		function_name += "_";
		if (function->sym_function->is_member_function()) {
			function_name += "M";
		}
		function_name += ".";
		function_name += std::to_string(m_compiler.get_num_functions_count());
	}

	llvm::Function* ll_function = llvm::Function::Create(
		ll_function_type,
		llvm::Function::ExternalLinkage, // publically visible
		function_name.c_str(),
		*m_llvm_module
	);

	if (m_print) {
		u32 param_index = 0;
		u32 ll_param_index = 0;
		if (function->sym_function->is_member_function()) {
			ll_function->getArg(ll_param_index)->setName("this");
			++ll_param_index;
		}
		while (param_index < function->parameters.size()) {
			nyla::avariable_decl* param = function->parameters[param_index];
			ll_function->getArg(ll_param_index)->setName(get_word(param->name_key).c_str());
			++ll_param_index;
			++param_index;
		}
	}

	if (function->is_external()) {
		ll_function->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
		// TODO: the user should be allowed to set the calling convention in code.
		ll_function->setCallingConv(llvm::CallingConv::X86_StdCall); // TODO Windows only!
	}

	if (!function->is_external()) {

		// Entry block for the function.
		llvm::BasicBlock* ll_basic_block = llvm::BasicBlock::Create(*nyla::llvm_context, "entry block", ll_function);
		m_llvm_builder->SetInsertPoint(ll_basic_block);
	
		// Allocating memory for the parameters
		u32 param_index = 0;
		u32 ll_param_index = 0;
		if (!(function->sym_function->mods & MOD_STATIC)) {
			++ll_param_index; // Self reference
			// There is no reason to store/load the pointer to
			// self since the user cannot modify its value.
		}
		while (param_index < function->parameters.size()) {
			sym_variable* sym_variable = function->parameters[param_index]->sym_variable;
			llvm::Value* var_alloca = gen_allocation(sym_variable);
			m_llvm_builder->CreateStore(ll_function->getArg(ll_param_index), var_alloca); // Storing the incoming value
			++param_index;
			++ll_param_index;
		}
	}

	function->sym_function->ll_function = ll_function;
}

llvm::Value* nyla::llvm_generator::gen_global_variable(nyla::avariable_decl* global) {
	// TODO: im pretty sure this shit is broken AF for default initialization

	// If it is static then it becomes a global variable
	nyla::sym_variable* sym_variable = global->sym_variable;

	std::string global_name = "g_";
	global_name += get_word(sym_variable->name_key).c_str();
	global_name += ".";
	global_name += std::to_string(m_compiler.get_num_global_variable_count());

	m_llvm_module->getOrInsertGlobal(
		global_name.c_str(), gen_type(global->type));

	llvm::GlobalVariable* ll_gvar =
		m_llvm_module->getNamedGlobal(global_name);

	nyla::type* type = global->type;
	switch (type->tag) {
	case TYPE_ARR: {
		// Has to be initialized to something for it not to be external
		ll_gvar->setInitializer(llvm::ConstantPointerNull::get(
			llvm::cast<llvm::PointerType>(gen_type(type))));
		
		// Even if there is no assignment there still may be
		// array size allocation
		m_compiler.add_global_initialize_expr(global);
		break;
	}
	case TYPE_MODULE: {
		
		ll_gvar->setInitializer(gen_global_module(global));
		// Some fields may not be able to be fully generated during it's
		// declaration so must be handled later
		m_compiler.add_global_initialize_expr(global);
		break;
	}
	default: {
		if (!global->assignment) {
			ll_gvar->setInitializer(gen_default_value(type));
		} else {
			if (global->assignment->literal_constant) {
					ll_gvar->setInitializer(llvm::cast<llvm::Constant>(
						gen_expr_rvalue(
							dynamic_cast<nyla::abinary_op*>(global->assignment)->rhs)
					));
			} else {
				ll_gvar->setInitializer(gen_default_value(type));
				m_compiler.add_global_initialize_expr(global);
			}
		}
		break;
	}
	}

	return ll_gvar;
}

llvm::Constant* nyla::llvm_generator::gen_global_module(nyla::avariable_decl* static_module) {
	nyla::type* type = static_module->type;

	sym_module* sym_module = type->sym_module;

	std::vector<llvm::Constant*> parameters;
	bool has_member_fields = false;
	for (nyla::avariable_decl* field : sym_module->fields) {	
		has_member_fields = true;
		switch (field->type->tag) {
		case TYPE_MODULE: {
			llvm::Constant* constantStruct = gen_global_module(field);
			parameters.push_back(constantStruct);
			break;
		}
		case TYPE_ARR: {
			// Has to be initialized to something to fill the space in the structure
			parameters.push_back(llvm::ConstantPointerNull::get(
				llvm::cast<llvm::PointerType>(gen_type(field->type))));
			break;
		}
		default: {
			if (!field->assignment) {
				parameters.push_back(gen_default_value(field->type));
			} else {
				if (field->assignment->literal_constant) {
					parameters.push_back(
						llvm::cast<llvm::Constant>(
							gen_expr_rvalue(
								dynamic_cast<nyla::abinary_op*>(field->assignment)->rhs)
							));
				} else {
					// Have to come back and fill in later
					parameters.push_back(gen_default_value(field->type));
				}
			}
			break;
		}
		}
	}
	if (!has_member_fields) {
		parameters.push_back(get_ll_int8(0));
	}

	llvm::StructType* st = llvm::cast<llvm::StructType>(gen_type(type));

	llvm::Constant* constantStruct =
		llvm::ConstantStruct::get(st, parameters);

	return constantStruct;
}

void nyla::llvm_generator::gen_function_body(nyla::afunction* function) {
	m_function = function;
	llvm::Function* ll_function = function->sym_function->ll_function;
	if (!function->is_external()) {

		m_llvm_builder->SetInsertPoint(&ll_function->getEntryBlock());
		m_ll_function = ll_function;
		
		for (nyla::aexpr* stmt : function->stmts) {
			gen_expression(stmt);
		}
	}

	if (m_print) {
		if (!m_function->is_main_function) {
			ll_function->print(llvm::outs());
			std::cout << '\n';
		}
	}
	m_function = nullptr;
}

llvm::Value* nyla::llvm_generator::gen_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_VARIABLE_DECL:
		return gen_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr), true);
	case AST_RETURN:
		return gen_return(dynamic_cast<nyla::areturn*>(expr));
	case AST_BINARY_OP:
		return gen_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
	case AST_UNARY_OP:
		return gen_unary_op(dynamic_cast<nyla::aunary_op*>(expr));
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
		return gen_number(dynamic_cast<nyla::anumber*>(expr));
	case AST_VALUE_NULL:
		return gen_null(expr);
	case AST_VALUE_BOOL:
		return get_ll_int1(dynamic_cast<nyla::abool*>(expr)->tof);
	case AST_IDENT:
		return gen_ident(dynamic_cast<nyla::aident*>(expr));
	case AST_FUNCTION_CALL: {
		llvm::Value* ptr_to_module_struct = nullptr;
		if (m_function) {
			if (m_function->sym_function->is_member_function()) {
				// First argument is to self pointer
				ptr_to_module_struct = m_ll_function->getArg(0);
			}
		}
		return gen_function_call(ptr_to_module_struct, dynamic_cast<nyla::afunction_call*>(expr));
	}
	case AST_TYPE_CAST:
		return gen_type_cast(dynamic_cast<nyla::atype_cast*>(expr));
	case AST_ARRAY_ACCESS: {
		nyla::aarray_access* array_access = dynamic_cast<nyla::aarray_access*>(expr);
		return gen_array_access(array_access->ident->sym_variable->ll_alloc, array_access);
	}
	case AST_FOR_LOOP:
		return gen_for_loop(dynamic_cast<nyla::afor_loop*>(expr));
	case AST_WHILE_LOOP:
		return gen_while_loop(dynamic_cast<nyla::awhile_loop*>(expr));
	case AST_ARRAY:
		return gen_array(dynamic_cast<nyla::aarray*>(expr));
	case AST_STRING8:
	case AST_STRING16:
	case AST_STRING32:
		return gen_string(dynamic_cast<nyla::astring*>(expr));
	case AST_DOT_OP:
		return gen_dot_op(dynamic_cast<nyla::adot_op*>(expr));
	case AST_IF:
		return gen_if(dynamic_cast<nyla::aif*>(expr));
	default:
		assert(!"Unimplemented expression generator");
		return nullptr;
	}
}

llvm::Value* nyla::llvm_generator::gen_expr_rvalue(nyla::aexpr* expr) {
	llvm::Value* value = gen_expression(expr);
	switch (expr->tag) {
	case AST_IDENT:
		// Assumed a variable
		return m_llvm_builder->CreateLoad(value);
	case AST_ARRAY_ACCESS:
		return m_llvm_builder->CreateLoad(value);
	case AST_DOT_OP: {
		// TODO: fix for array accesses (should be reflected similar to above)
		nyla::adot_op* dot_op = dynamic_cast<nyla::adot_op*>(expr);
		nyla::aexpr* last_factor = dot_op->factor_list.back();
		switch (last_factor->tag) {
		case AST_IDENT:
			// Assumed a variable
			return m_llvm_builder->CreateLoad(value);
		case AST_ARRAY_ACCESS:
			return m_llvm_builder->CreateLoad(value);
		case AST_FUNCTION_CALL:
			return value; // Function calls return rvalues (TODO: if the return type is a module this may not hold)
		default: assert(!"Should be unreachable");
		}
	}
	}
	return value;
}

llvm::Value* nyla::llvm_generator::gen_variable_decl(nyla::avariable_decl* variable_decl, bool allocate_decl) {
	if (allocate_decl) {
		gen_allocation(variable_decl->sym_variable);
	}
	
	sym_variable* sym_variable = variable_decl->sym_variable;
	llvm::Value* ll_alloca = sym_variable->ll_alloc;
	if (variable_decl->assignment != nullptr) {
		gen_expression(variable_decl->assignment);
	} else {
		if (!sym_variable->computed_arr_dim_sizes.empty()) {
			// Allocating space for the array
			llvm::Value* arr_alloca = gen_precomputed_array_alloca(sym_variable->type, sym_variable->computed_arr_dim_sizes);
			m_llvm_builder->CreateStore(arr_alloca, ll_alloca);
		}
		gen_default_value(sym_variable, variable_decl->type, variable_decl->default_initialize);
	}
	return ll_alloca;
}

llvm::Value* nyla::llvm_generator::gen_return(nyla::areturn* ret) {
	if (ret->value == nullptr) {
		return m_llvm_builder->CreateRetVoid();
	}
	return m_llvm_builder->CreateRet(gen_expr_rvalue(ret->value));
}

llvm::Value* nyla::llvm_generator::gen_number(nyla::anumber* number) {
	bool is_signed = number->type->is_signed();
	switch (number->tag) {
	case AST_VALUE_BYTE:
	case AST_VALUE_UBYTE:
	case AST_VALUE_CHAR8:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt8Ty(*nyla::llvm_context), number->value_int, is_signed);
	case AST_VALUE_SHORT:
	case AST_VALUE_USHORT:
	case AST_VALUE_CHAR16:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt16Ty(*nyla::llvm_context), number->value_int, is_signed);
	case AST_VALUE_INT:
	case AST_VALUE_UINT:
	case AST_VALUE_CHAR32:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt32Ty(*nyla::llvm_context), number->value_int, is_signed);
	case AST_VALUE_LONG:
	case AST_VALUE_ULONG:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt64Ty(*nyla::llvm_context), number->value_int, is_signed);
	case AST_VALUE_FLOAT:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat(number->value_float));
	case AST_VALUE_DOUBLE:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat(number->value_double));
	default:
		assert(!"Unimplemented number generation");
		return nullptr;
	}
}

llvm::Value* nyla::llvm_generator::gen_null(nyla::aexpr* expr) {
	return llvm::ConstantPointerNull::get(
		llvm::cast<llvm::PointerType>(gen_type(expr->type)));
}

llvm::Value* nyla::llvm_generator::gen_binary_op(nyla::abinary_op* binary_op) {
	switch (binary_op->op) {
	case '=': {
		llvm::Value* ll_alloca = gen_expression(binary_op->lhs);

		switch (binary_op->rhs->tag) {
		case AST_VAR_OBJECT:
			return gen_var_object(ll_alloca, dynamic_cast<nyla::avar_object*>(binary_op->rhs));
		default: {
			llvm::Value* ll_rvalue = gen_expr_rvalue(binary_op->rhs);
			
			m_llvm_builder->CreateStore(ll_rvalue, ll_alloca);
			
			return ll_rvalue; // In case of multiple assignments they
							  // may assign the value assigned lower on the tree.
							  // Ex.     a = b = c + 5;    c + 5 Value is returned
							  //                           which is then used to assign
							  //                           a and b.
		}
		}
	}
	case '+': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->type->is_int()) {
			return m_llvm_builder->CreateAdd(ll_lhs, ll_rhs);
		}
		return m_llvm_builder->CreateFAdd(ll_lhs, ll_rhs);
	}
	case '-': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->type->is_int()) {
			return m_llvm_builder->CreateSub(ll_lhs, ll_rhs);
		}
		return m_llvm_builder->CreateFSub(ll_lhs, ll_rhs);
	}
	case '*': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->type->is_int()) {
			return m_llvm_builder->CreateMul(ll_lhs, ll_rhs);
		}
		return m_llvm_builder->CreateFMul(ll_lhs, ll_rhs);
	}
	case '/': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->type->is_int()) {
			if (binary_op->type->is_signed()) {
				return m_llvm_builder->CreateSDiv(ll_lhs, ll_rhs);
			} else {
				return m_llvm_builder->CreateUDiv(ll_lhs, ll_rhs);
			}
		}
		return m_llvm_builder->CreateFDiv(ll_lhs, ll_rhs);
	}
	case '<': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->lhs->type->is_int()) {
			if (binary_op->lhs->type->is_signed()) {
				return m_llvm_builder->CreateICmpSLT(ll_lhs, ll_rhs);
			} else {
				return m_llvm_builder->CreateICmpULT(ll_lhs, ll_rhs);
			}
		}
		return m_llvm_builder->CreateFCmpULT(ll_lhs, ll_rhs);
	}
	case '>': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->lhs->type->is_int()) {
			if (binary_op->lhs->type->is_signed()) {
				return m_llvm_builder->CreateICmpSGT(ll_lhs, ll_rhs);
			} else {
				return m_llvm_builder->CreateICmpUGT(ll_lhs, ll_rhs);
			}
		}
		return m_llvm_builder->CreateFCmpUGT(ll_lhs, ll_rhs);
	}
	case TK_EQ_EQ: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->lhs->type->is_int()) {
			return m_llvm_builder->CreateICmpEQ(ll_lhs, ll_rhs);
		}
		return m_llvm_builder->CreateFCmpUEQ(ll_lhs, ll_rhs);
	}
	case TK_LT_EQ: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->lhs->type->is_int()) {
			if (binary_op->lhs->type->is_signed()) {
				return m_llvm_builder->CreateICmpSLE(ll_lhs, ll_rhs);
			} else {
				return m_llvm_builder->CreateICmpULE(ll_lhs, ll_rhs);
			}
		}
		return m_llvm_builder->CreateFCmpULE(ll_lhs, ll_rhs);
	}
	case TK_GT_EQ: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->lhs->type->is_int()) {
			if (binary_op->lhs->type->is_signed()) {
				return m_llvm_builder->CreateICmpSGE(ll_lhs, ll_rhs);
			} else {
				return m_llvm_builder->CreateICmpUGE(ll_lhs, ll_rhs);
			}
		}
		return m_llvm_builder->CreateFCmpUGE(ll_lhs, ll_rhs);
	}
	case '%': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		if (binary_op->type->is_signed()) {
			return m_llvm_builder->CreateSRem(ll_lhs, ll_rhs);
		}
		return m_llvm_builder->CreateURem(ll_lhs, ll_rhs);
	}
	case '&': case TK_AMP_AMP: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		return m_llvm_builder->CreateAnd(ll_lhs, ll_rhs);
	}
	case '^': {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		return m_llvm_builder->CreateXor(ll_lhs, ll_rhs);
	}
	case '|': case TK_BAR_BAR: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		return m_llvm_builder->CreateOr(ll_lhs, ll_rhs);
	}
	case TK_LT_LT: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		return m_llvm_builder->CreateShl(ll_lhs, ll_rhs);
	}
	case TK_GT_GT: {
		llvm::Value* ll_lhs = gen_expr_rvalue(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expr_rvalue(binary_op->rhs);
		return m_llvm_builder->CreateLShr(ll_lhs, ll_rhs);
	}
	default:
		assert(!"Missing operator generation case");
		return nullptr;
	}
}

llvm::Value* nyla::llvm_generator::gen_unary_op(nyla::aunary_op* unary_op) {
	switch (unary_op->op) {
	case '-': {
		llvm::Value* ll_factor = gen_expr_rvalue(unary_op->factor);
		if (unary_op->type->is_int()) {
			return m_llvm_builder->CreateNeg(ll_factor);
		}
		return m_llvm_builder->CreateFNeg(ll_factor);
	}
	case '+':
		// unary + is merely semantics
		return gen_expr_rvalue(unary_op->factor);
	case '&': {
		return gen_expression(unary_op->factor);
	}
	case TK_PLUS_PLUS: {
		llvm::Value* ll_lvalue = gen_expression(unary_op->factor);
		llvm::Value* ll_rvalue = m_llvm_builder->CreateLoad(ll_lvalue);
		llvm::Value* ll_one = nullptr;
		switch (unary_op->factor->type->tag) {
		case TYPE_BYTE: case TYPE_CHAR8:   ll_one = get_ll_int8(1);   break;
		case TYPE_UBYTE:                   ll_one = get_ll_uint8(1);  break;
		case TYPE_SHORT: case TYPE_CHAR16: ll_one = get_ll_int16(1);  break;
		case TYPE_USHORT:                  ll_one = get_ll_uint16(1); break;
		case TYPE_INT: case TYPE_CHAR32:   ll_one = get_ll_int32(1);  break;
		case TYPE_UINT:                    ll_one = get_ll_uint32(1); break;
		case TYPE_LONG:                    ll_one = get_ll_int64(1);  break;
		case TYPE_ULONG:                   ll_one = get_ll_uint64(1); break;
		default: assert(!"Unimplemented");
		}
		llvm::Value* inc_res = m_llvm_builder->CreateAdd(ll_rvalue, ll_one);
		m_llvm_builder->CreateStore(inc_res, ll_lvalue);
		return inc_res;
	}
	case TK_MINUS_MINUS: {
		llvm::Value* ll_lvalue = gen_expression(unary_op->factor);
		llvm::Value* ll_rvalue = m_llvm_builder->CreateLoad(ll_lvalue);
		llvm::Value* ll_one = nullptr;
		switch (unary_op->factor->type->tag) {
		case TYPE_BYTE: case TYPE_CHAR8:   ll_one = get_ll_int8(1);   break;
		case TYPE_UBYTE:                   ll_one = get_ll_uint8(1);  break;
		case TYPE_SHORT: case TYPE_CHAR16: ll_one = get_ll_int16(1);  break;
		case TYPE_USHORT:                  ll_one = get_ll_uint16(1); break;
		case TYPE_INT: case TYPE_CHAR32:   ll_one = get_ll_int32(1);  break;
		case TYPE_UINT:                    ll_one = get_ll_uint32(1); break;
		case TYPE_LONG:                    ll_one = get_ll_int64(1);  break;
		case TYPE_ULONG:                   ll_one = get_ll_uint64(1); break;
		default: assert(!"Unimplemented");
		}
		llvm::Value* dec_res = m_llvm_builder->CreateSub(ll_rvalue, ll_one);
		m_llvm_builder->CreateStore(dec_res, ll_lvalue);
		return dec_res;
	}
	case '!': {
		return m_llvm_builder->CreateNeg(gen_expr_rvalue(unary_op->factor));
	}
	default:
		assert(!"Unimplemented unary case");
		return nullptr;
	}
}

llvm::Value* nyla::llvm_generator::gen_ident(nyla::aident* ident) {
	
	// Returns lvalue of variable
	nyla::sym_variable* sym_variable = ident->sym_variable;
	
	if (sym_variable->is_field && !(sym_variable->mods & MOD_STATIC)
		&& !m_initializing_module // Already GEP'd into the struct and placed the result in ll_alloc
		) {
		// Must be a member function. Eq. to this->var
		llvm::Value* ll_this = m_ll_function->getArg(0); // First argument is "this"
		return m_llvm_builder->CreateStructGEP(ll_this, sym_variable->field_index);
	} else {
		return sym_variable->ll_alloc;
	}
}

llvm::Value* nyla::llvm_generator::gen_function_call(llvm::Value* ptr_to_struct, nyla::afunction_call* function_call) {
	llvm::Function* ll_called_function = function_call->called_function->ll_function;
	std::vector<llvm::Value*> ll_parameter_values;
	if (function_call->called_function->is_member_function()) {
		ll_parameter_values.push_back(ptr_to_struct);
	}

	for (nyla::aexpr* parameter_value : function_call->arguments) {
		ll_parameter_values.push_back(gen_expr_rvalue(parameter_value));
	}

	//std::cout << "Call types:\n";
	//for (llvm::Value* v : ll_parameter_values) {
	//	v->getType()->print(llvm::outs());
	//	std::cout << '\n';
	//}

	return m_llvm_builder->CreateCall(ll_called_function, ll_parameter_values);
}

llvm::Value* nyla::llvm_generator::gen_array(nyla::aarray* arr) {
	
	u32 array_size = arr->elements.size();
	
	llvm::Value* arr_alloca = gen_array_alloca(arr->type->element_type, array_size);
	llvm::Value* ll_arr_ptr = get_arr_ptr(arr->type->element_type, arr_alloca);
	
	// TODO: is comptime_compat here even correct? I would think that it should be
	// that every element is constant_literal
	if (arr->comptime_compat && !arr->type->element_type->is_arr()) {
		// Constant array so creating global memory and memcpy'ing

		std::vector<llvm::Constant*> ll_element_values;
		for (u32 index = 0; index < array_size; index++) {
			ll_element_values.push_back(
				llvm::cast<llvm::Constant>(gen_expr_rvalue(arr->elements[index])));
		}

		gen_global_const_array(arr->type->element_type, ll_element_values, ll_arr_ptr);

	} else {
		for (u32 index = 0; index < arr->elements.size(); index++) {
			llvm::Value* gep = m_llvm_builder->CreateGEP(ll_arr_ptr, get_ll_uint32(index));
			m_llvm_builder->CreateStore(gen_expr_rvalue(arr->elements[index]), gep);
		}
	}

	return arr_alloca;
}

template<typename char_type, typename to_type>
void str_to_const_array(const std::basic_string<char_type>& str,
	                    std::vector<llvm::Constant*>& ll_element_values) {
	for (const char_type& c : str) {
		if constexpr (sizeof(to_type) == 1) {
			ll_element_values.push_back(nyla::get_ll_int8(c));
		}
		if constexpr (sizeof(to_type) == 2) {
			ll_element_values.push_back(nyla::get_ll_int16(c));
		}
		if constexpr (sizeof(to_type) == 4) {
			ll_element_values.push_back(nyla::get_ll_int32(c));
		}
	}
}

llvm::Value* nyla::llvm_generator::gen_string(nyla::astring* str) {

	// String arrays can always be stored in global memory

	std::vector<llvm::Constant*> ll_element_values;
	u32 array_size = str->lit8.length();
	switch (str->tag) {
	case AST_STRING8: {
		array_size = str->lit8.length();
		switch (str->type->element_type->tag) {
		case TYPE_CHAR8:
			str_to_const_array<c8, s8>(str->lit8, ll_element_values);
			break;
		case TYPE_CHAR16:
			str_to_const_array<c8, s16>(str->lit8, ll_element_values);
			break;
		case TYPE_CHAR32:
			str_to_const_array<c8, s32>(str->lit8, ll_element_values);
			break;
		}
		break;
	}
	case AST_STRING16: {
		array_size = str->lit16.length();
		switch (str->type->element_type->tag) {
		case TYPE_CHAR8:
			str_to_const_array<c16, s8>(str->lit16, ll_element_values);
			break;
		case TYPE_CHAR16:
			str_to_const_array<c16, s16>(str->lit16, ll_element_values);
			break;
		case TYPE_CHAR32:
			str_to_const_array<c16, s32>(str->lit16, ll_element_values);
			break;
		}
		break;
	}
	case AST_STRING32: {
		array_size = str->lit32.length();
		switch (str->type->element_type->tag) {
		case TYPE_CHAR8:
			str_to_const_array<c32, s8>(str->lit32, ll_element_values);
			break;
		case TYPE_CHAR16:
			str_to_const_array<c32, s16>(str->lit32, ll_element_values);
			break;
		case TYPE_CHAR32:
			str_to_const_array<c32, s32>(str->lit32, ll_element_values);
			break;
		}
		break;
	}
	default: assert(!"Should be unreachable");
	}

	if (str->type->is_arr()) {
		llvm::Value* arr_alloca = gen_array_alloca(str->type->element_type, array_size);
		llvm::Value* ll_arr_ptr = get_arr_ptr(str->type->element_type, arr_alloca);
		gen_global_const_array(str->type->element_type, ll_element_values, ll_arr_ptr);
		return arr_alloca;
	} else {
		// Interpreted as a character pointer
		llvm::Value* arr_alloca =
			m_llvm_builder->CreateAlloca(gen_type(str->type->element_type), get_ll_uint32(array_size));
		gen_global_const_array(str->type->element_type, ll_element_values, arr_alloca);
		return arr_alloca;
	}
}

void nyla::llvm_generator::gen_global_const_array(nyla::type* element_type,
	                                              const std::vector<llvm::Constant*>& ll_element_values,
	                                              llvm::Value* ll_arr_ptr) {


	llvm::ArrayType* ll_array_type =
		llvm::ArrayType::get(gen_type(element_type), ll_element_values.size());
	std::string global_name = "__gA.";
	global_name += std::to_string(m_compiler.get_num_global_const_array_count());

	m_llvm_module->getOrInsertGlobal(global_name, ll_array_type);

	llvm::GlobalVariable* ll_gvar =
		m_llvm_module->getNamedGlobal(global_name);

	ll_gvar->setInitializer(
		llvm::ConstantArray::get(ll_array_type, ll_element_values));

	if (m_print) {
		ll_gvar->print(llvm::outs());
		std::cout << '\n';
	}

	llvm::MaybeAlign alignment = llvm::MaybeAlign(element_type->mem_size());
	m_llvm_builder->CreateMemCpy(
		ll_arr_ptr, alignment,
		ll_gvar   , alignment,
		ll_element_values.size() * element_type->mem_size()
	);
}

llvm::Value* nyla::llvm_generator::gen_array_access(llvm::Value* ll_location, nyla::aarray_access* array_access) {
	
	sym_variable* sym_variable = array_access->ident->sym_variable;
	nyla::type* arr_type = sym_variable->type;
	
	llvm::Value* ll_arr_alloca = m_llvm_builder->CreateLoad(ll_location);
	llvm::Value* ll_arr_ptr = nullptr;
	if (arr_type->is_arr()) {
		ll_arr_ptr = get_arr_ptr(arr_type->element_type, ll_arr_alloca);
	} else {
		ll_arr_ptr = ll_arr_alloca;
	}
	
	llvm::Value* ll_element = nullptr;
	for (u32 i = 0; i < array_access->indexes.size(); i++) {
		nyla::aexpr* index = array_access->indexes[i];
		llvm::Value* ll_index = gen_expr_rvalue(index);

		ll_element = m_llvm_builder->CreateGEP(ll_arr_ptr, ll_index);

		if (arr_type->element_type->is_arr() || arr_type->element_type->is_ptr()) {
			ll_arr_alloca = m_llvm_builder->CreateLoad(ll_element); // Strip pointer
			if (arr_type->is_arr()) {
				ll_arr_ptr = get_arr_ptr(arr_type->element_type, ll_arr_alloca);
			} else {
				ll_arr_ptr = ll_arr_alloca;
			}
		}
		arr_type = arr_type->element_type;
	}

	return ll_element;
}

llvm::Value* nyla::llvm_generator::gen_type_cast(nyla::atype_cast* type_cast) {
	nyla::type*  val_type     = type_cast->value->type;
	nyla::type*  cast_to_type = type_cast->type;
	llvm::Value* ll_val       = gen_expr_rvalue(type_cast->value);
	llvm::Type*  ll_cast_type = gen_type(type_cast->type);

	if (val_type->is_int() && cast_to_type->is_int()) {
		if (cast_to_type->mem_size() < val_type->mem_size()) {
			// Signed and unsigned downcasting use trunc
			return m_llvm_builder->CreateTrunc(ll_val, ll_cast_type);
		} else {
			if (cast_to_type->is_signed()) {
				// Signed upcasting
				return m_llvm_builder->CreateSExt(ll_val, ll_cast_type);
			} else {
				// Unsigned upcasting
				return m_llvm_builder->CreateZExt(ll_val, ll_cast_type);
			}
		}
	} else if (cast_to_type->is_float() && val_type->is_int()) {
		// Int to floating point
		if (val_type->is_signed()) {
			return m_llvm_builder->CreateSIToFP(ll_val, ll_cast_type);
		} else {
			return m_llvm_builder->CreateUIToFP(ll_val, ll_cast_type);
		}
	} else if (cast_to_type->is_int() && val_type->is_float()) {
		// Floating point to Int
		if (cast_to_type->is_signed()) {
			return m_llvm_builder->CreateFPToSI(ll_val, ll_cast_type);
		} else {
			return m_llvm_builder->CreateFPToUI(ll_val, ll_cast_type);
		}
	} else if (cast_to_type->is_float() && val_type->is_float()) {
		// Float to Float
		if (cast_to_type->mem_size() > val_type->mem_size()) {
			// Upcasting float
			return m_llvm_builder->CreateFPExt(ll_val, ll_cast_type);
		} else {
			// Downcasting float
			return m_llvm_builder->CreateFPTrunc(ll_val, ll_cast_type);
		}
	} else if (cast_to_type->is_int() && val_type->is_ptr()) {
		return m_llvm_builder->CreatePtrToInt(ll_val, ll_cast_type);
	} else if (cast_to_type->is_ptr() && val_type->is_int()) {
		return m_llvm_builder->CreateIntToPtr(ll_val, ll_cast_type);
	} else if (cast_to_type->is_ptr() && val_type->is_arr()) {
		// Array to pointer
		// Arrays are already pointers so simply need to
		// get the pointer after the size

		return get_arr_ptr(val_type->element_type, ll_val);
	}
	assert(!"Unhandled cast case");
	return nullptr;
}

llvm::Value* nyla::llvm_generator::gen_var_object(llvm::Value* ptr_to_struct, nyla::avar_object* var_object) {
	
	// Initializing the values in the data structure
	m_initializing_module = true;
	for (nyla::avariable_decl* field : var_object->sym_module->fields) {
		
		// When initialzing globals not all the data has to be set.
		// Only that data that was not able to be set by the global
		// initialization has to be set
		bool initialize = true;
		if (m_initializing_globals) {
			switch (field->type->tag) {
			case TYPE_ARR:  break; // Always need to initialize array
			case TYPE_MODULE:
				// GEP instructions seem to be discarded by LLVM anyway if they are
				// not used so just go ahead and attempt allocation anyway
				break;
			default: {
				if (field->assignment) {
					if (field->assignment->literal_constant) {
						initialize = false; // Already initialized
					}
				}
				break;
			}
			}
		}

		if (initialize) {
			u32 field_index = field->sym_variable->field_index;

			field->sym_variable->ll_alloc = m_llvm_builder->CreateStructGEP(ptr_to_struct, field_index);
			if (field->assignment) {
				gen_expression(field->assignment);
			} else {
				gen_default_value(field->sym_variable, field->type, field->default_initialize);
			}
		}
	}
	m_initializing_module = false;

	if (!var_object->assumed_default_constructor) {
		gen_function_call(ptr_to_struct, var_object->constructor_call);
	}
	return ptr_to_struct;
}

llvm::Value* nyla::llvm_generator::gen_for_loop(nyla::afor_loop* for_loop) {
	
	// Generating declarations before entering the loop
	for (nyla::avariable_decl* var_decl : for_loop->declarations) {
		gen_variable_decl(var_decl, true);
	}

	return gen_loop(dynamic_cast<nyla::aloop_expr*>(for_loop));
}

llvm::Value* nyla::llvm_generator::gen_while_loop(nyla::awhile_loop* while_loop) {
	return gen_loop(dynamic_cast<nyla::aloop_expr*>(while_loop));
}

llvm::Value* nyla::llvm_generator::gen_loop(nyla::aloop_expr* loop_expr) {

	// The block that tells weather or not to continue looping
	llvm::BasicBlock* ll_cond_bb = llvm::BasicBlock::Create(*llvm_context, "loopcond", m_ll_function);

	// Jumping directly into the loop condition
	m_llvm_builder->CreateBr(ll_cond_bb);

	// If there are post loop statements they need to come first
	// even though they are jumped over the first loop to the
	// conditional block
	llvm::BasicBlock* ll_post_stmts_bb = nullptr;
	if (!loop_expr->post_exprs.empty()) {
		ll_post_stmts_bb = llvm::BasicBlock::Create(*llvm_context, "poststmts", m_ll_function);
		
		// Telling llvm we want to put code into the post stmts block
		m_llvm_builder->SetInsertPoint(ll_post_stmts_bb);
		
		for (nyla::aexpr* post_stmt : loop_expr->post_exprs) {
			gen_expression(post_stmt);
		}

		// Jumping directly into the loop condition
		m_llvm_builder->CreateBr(ll_cond_bb);
	}

	// Telling llvm we want to put code into the loop condition block
	m_llvm_builder->SetInsertPoint(ll_cond_bb);

	llvm::BasicBlock* ll_loop_body_bb = llvm::BasicBlock::Create(*llvm_context, "loopbody", m_ll_function);

	// Generating the condition and telling it to jump to the body or the finish point
	llvm::Value* ll_cond = gen_expr_rvalue(loop_expr->cond);
	llvm::BasicBlock* ll_finish_bb = llvm::BasicBlock::Create(*llvm_context, "finishloop", m_ll_function);
	m_llvm_builder->CreateCondBr(ll_cond, ll_loop_body_bb, ll_finish_bb);
	m_ll_loop_exit = ll_finish_bb;

	// Telling llvm we want to put code into the loop body
	m_llvm_builder->SetInsertPoint(ll_loop_body_bb);

	// Generating the body of the loop
	for (nyla::aexpr* stmt : loop_expr->body) {
		gen_expression(stmt);
	}

	// Unconditional branch back to the condition or post statements
	// to restart the loop
	if (ll_post_stmts_bb) {
		branch_if_not_term(ll_post_stmts_bb);
	} else {
		branch_if_not_term(ll_cond_bb);
	}

	// All new code will be inserted into the block after
	// the loop
	m_llvm_builder->SetInsertPoint(ll_finish_bb);

	return nullptr;
}

llvm::Value* nyla::llvm_generator::gen_dot_op(nyla::adot_op* dot_op) {
	llvm::Value* ll_location = nullptr;
	for (u32 idx = 0; idx < dot_op->factor_list.size(); idx++) {
		nyla::aexpr* factor = dot_op->factor_list[idx];
		switch (factor->tag) {
		case AST_IDENT: {
			nyla::aident* ident = dynamic_cast<nyla::aident*>(factor);
			if (ident->is_array_length) {

				llvm::Value* ll_arr_alloca = m_llvm_builder->CreateLoad(ll_location);
				
				// To i32* to store the length
				llvm::Value* ll_as_i32_ptr =
					m_llvm_builder->CreateBitCast(ll_arr_alloca, llvm::Type::getInt32PtrTy(*nyla::llvm_context));
				
				return m_llvm_builder->CreateGEP(ll_as_i32_ptr, get_ll_uint32(0));
			}

			if (!ll_location) {
				// First location means that instead of GEP'ing
				// a reference to the variable's memory is used
				// 
				// Ex. a.b
				//     ^- This
				if (!ident->references_module) {
					ll_location = ident->sym_variable->ll_alloc;
				}
			} else {
				// Any variable in the form   a.b, a[n].b, a().b  expects
				// b to be a field and a to have already set ll_location
				ll_location = m_llvm_builder->CreateStructGEP(
					                              ll_location,
					                              ident->sym_variable->field_index);
			}

			break;
		}
		case AST_ARRAY_ACCESS: {
			// a[n].b
			// a.b[n]

			nyla::aarray_access* array_access = dynamic_cast<nyla::aarray_access*>(factor);

			if (!ll_location) {
				ll_location = gen_array_access(array_access->ident->sym_variable->ll_alloc, array_access);
			} else {
				// Must be a member of a struct
				ll_location = m_llvm_builder->CreateStructGEP(
					ll_location,
					array_access->ident->sym_variable->field_index
				);
				ll_location = gen_array_access(ll_location, array_access);
			}

			break;
		}
		case AST_FUNCTION_CALL: {
			// location may be the ptr to the module in case of member functions
			ll_location = gen_function_call(ll_location, dynamic_cast<nyla::afunction_call*>(factor));
			break;
		}
		default: {
			assert(!"Should be unreachable");
			break;
		}
		}
	}
	return ll_location;
}

llvm::Value* nyla::llvm_generator::gen_if(nyla::aif* ifstmt) {

	// Basic block after any of the if statements. All if statement body's
	// finish by unconditionally jumping to this block.
	llvm::BasicBlock* ll_finish_ifs = llvm::BasicBlock::Create(*llvm_context, "finishifs", m_ll_function);

	nyla::aif* cur_if = ifstmt;
	while (cur_if) {
		// Body of if statement that is ran when the statement is true
		llvm::BasicBlock* ll_if_body_bb = llvm::BasicBlock::Create(*llvm_context, "ifbody", m_ll_function);
		// Optional else if condition block if there is an else if condition
		llvm::BasicBlock* ll_else_if_cond_bb = nullptr;
		if (cur_if->else_if) {
			ll_else_if_cond_bb = llvm::BasicBlock::Create(*llvm_context, "elseif", m_ll_function);
		}

		// Optional else block if there is an else scope
		llvm::BasicBlock* ll_else_bb = nullptr;
		if (cur_if->else_sym_scope) {
			ll_else_bb = llvm::BasicBlock::Create(*llvm_context, "elsebody", m_ll_function);
		}

		llvm::Value* ll_cond = gen_expr_rvalue(cur_if->cond);

		if (!cur_if->else_sym_scope) {
			if (cur_if->else_if) {
				// Jump either to the else if condition or the body
				m_llvm_builder->CreateCondBr(ll_cond, ll_if_body_bb, ll_else_if_cond_bb);
			} else {
				// Jump either to the finish point (end of all if statements) or the body
				m_llvm_builder->CreateCondBr(ll_cond, ll_if_body_bb, ll_finish_ifs);
			}
		} else {
			// Jump either to the body or else body
			m_llvm_builder->CreateCondBr(ll_cond, ll_if_body_bb, ll_else_bb);
		}

		// Telling llvm we want to put code into the loop body
		m_llvm_builder->SetInsertPoint(ll_if_body_bb);

		// Generating the body of the if stmt
		for (nyla::aexpr* stmt : cur_if->body) {
			gen_expression(stmt);
		}

		// Unconditionally branch out of the body of the if statement to
		// the end of all the if statements
		branch_if_not_term(ll_finish_ifs);

		// Else statement exist
		if (cur_if->else_sym_scope) {
			// Placing code in else block
			m_llvm_builder->SetInsertPoint(ll_else_bb);

			// Generating the body of the else stmt
			for (nyla::aexpr* stmt : cur_if->else_body) {
				gen_expression(stmt);
			}

			// Unconditionally branch out of the body of the else statement to
			// the end of all the if statements
			branch_if_not_term(ll_finish_ifs);
		}

		cur_if = cur_if->else_if;
		if (cur_if) {
			// Else if conditional block
			m_llvm_builder->SetInsertPoint(ll_else_if_cond_bb);
		}
	}

	// All new code goes in the block after the if statements
	m_llvm_builder->SetInsertPoint(ll_finish_ifs);

	return nullptr;
}

llvm::Type* nyla::llvm_generator::gen_type(nyla::type* type) {
	switch (type->tag) {
	case TYPE_BYTE:
	case TYPE_UBYTE:
	case TYPE_CHAR8:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_SHORT:
	case TYPE_USHORT:
	case TYPE_CHAR16:
		return llvm::Type::getInt16Ty(*nyla::llvm_context);
	case TYPE_INT:
	case TYPE_UINT:
	case TYPE_CHAR32:
		return llvm::Type::getInt32Ty(*nyla::llvm_context);
	case TYPE_LONG:
	case TYPE_ULONG:
		return llvm::Type::getInt64Ty(*nyla::llvm_context);
	case TYPE_FLOAT:
		return llvm::Type::getFloatTy(*nyla::llvm_context);
	case TYPE_DOUBLE:
		return llvm::Type::getDoubleTy(*nyla::llvm_context);
	case TYPE_BOOL:
		return llvm::Type::getInt1Ty(*nyla::llvm_context);
	case TYPE_VOID:
		return llvm::Type::getVoidTy(*nyla::llvm_context);
	case TYPE_PTR:
		// Arrays are just pointers
	case TYPE_ARR: {
		switch (type->element_type->tag) {
		case TYPE_PTR:
			return llvm::PointerType::get(gen_type(type->element_type), 0);
		case TYPE_BYTE:
		case TYPE_UBYTE:
		case TYPE_CHAR8:
			return llvm::Type::getInt8PtrTy(*nyla::llvm_context);
		case TYPE_SHORT:
		case TYPE_USHORT:
		case TYPE_CHAR16:
			return llvm::Type::getInt16PtrTy(*nyla::llvm_context);
		case TYPE_INT:
		case TYPE_UINT:
		case TYPE_CHAR32:
			return llvm::Type::getInt32PtrTy(*nyla::llvm_context);
		case TYPE_LONG:
		case TYPE_ULONG:
			return llvm::Type::getInt64PtrTy(*nyla::llvm_context);
		case TYPE_FLOAT:
			return llvm::Type::getFloatPtrTy(*nyla::llvm_context);
		case TYPE_DOUBLE:
			return llvm::Type::getDoublePtrTy(*nyla::llvm_context);
		case TYPE_BOOL:
			return llvm::Type::getInt8PtrTy(*nyla::llvm_context);
		default:
			return llvm::PointerType::get(gen_type(type->element_type), 0);
		}
	}
	case TYPE_MODULE:
		return type->sym_module->ll_struct_type;
	default:
		assert(!"Unimplemented type generator");
		return nullptr;
	}
}

llvm::Value* nyla::llvm_generator::gen_allocation(sym_variable* sym_variable) {
	llvm::Value* ll_alloca = m_llvm_builder->CreateAlloca(gen_type(sym_variable->type), nullptr);
	sym_variable->ll_alloc = ll_alloca;
	return ll_alloca;
}

llvm::Value* nyla::llvm_generator::gen_precomputed_array_alloca(nyla::type* type, const std::vector<u32>& dim_sizes, u32 depth) {
	// TODO: Need to create storage for the u32 dimension size
	
	llvm::Value* ll_alloca = gen_array_alloca(type->element_type, dim_sizes[depth]);
	// Need to allocate the space as well for the elements if
	// the elements are also arrays
	bool array_elements = dim_sizes.size() - 1 != depth;
	if (array_elements) {
		for (u32 i = 0; i < dim_sizes[depth]; i++) {
			llvm::Value* inner_array = gen_precomputed_array_alloca(type->element_type, dim_sizes, depth + 1);
			m_llvm_builder->CreateStore(
				inner_array,
				m_llvm_builder->CreateGEP(ll_alloca, get_ll_int32(i))
			);
		}
	}
	return ll_alloca;
}

llvm::Value* nyla::llvm_generator::gen_array_alloca(nyla::type* element_type, u32 num_elements) {
	u32 real_size = num_elements;
	
	// TODO: this should probably be alignment size not mem_size
	u32 mem_size = element_type->mem_size();
	if (mem_size >= 4) {
		// In case of longs only half the space is used
		real_size += 1;
	} else if (mem_size < 4) {
		real_size += (4 / mem_size);
	}

	llvm::Value* ll_alloca =
		m_llvm_builder->CreateAlloca(gen_type(element_type), get_ll_uint32(real_size));

	// To i32* to store the length
	llvm::Value* ll_as_i32_ptr =
		m_llvm_builder->CreateBitCast(ll_alloca, llvm::Type::getInt32PtrTy(*nyla::llvm_context));

	// Storing the size at zero
	m_llvm_builder->CreateStore(get_ll_uint32(num_elements), ll_as_i32_ptr);
	return ll_alloca;
}

llvm::Value* nyla::llvm_generator::get_arr_ptr(nyla::type* element_type, llvm::Value* arr_alloca) {
	// TODO: this should probably be alignment size not mem_size
	u32 mem_size = element_type->mem_size();
	u32 offset_to_data;
	if (mem_size >= 4) {
		offset_to_data = 1; // Either 1 i32 or i64
	} else {
		offset_to_data = 4 / mem_size;
	}

	return m_llvm_builder->CreateGEP(arr_alloca, get_ll_uint32(offset_to_data));
}

nyla::word nyla::llvm_generator::get_word(u32 word_key) {
	return nyla::g_word_table->get_word(word_key);
}

void nyla::llvm_generator::gen_default_value(sym_variable* sym_variable, nyla::type* type, bool default_initialize) {
	if (!type->is_arr()) {
		if (default_initialize) {
			llvm::Value* default_value = gen_default_value(type);
			m_llvm_builder->CreateStore(default_value, sym_variable->ll_alloc);
		}
	} else {
		if (default_initialize) {
			gen_default_array(sym_variable, type, m_llvm_builder->CreateLoad(sym_variable->ll_alloc));
		}
	}
}

llvm::Constant* nyla::llvm_generator::gen_default_value(nyla::type* type) {
	switch (type->tag) {
	case TYPE_ARR:
		assert(!"Should call gen_default_array");
		return nullptr;
	case TYPE_PTR:
		return llvm::ConstantPointerNull::get(
			llvm::cast<llvm::PointerType>(gen_type(type)));
	case TYPE_BYTE: case TYPE_CHAR8:   return get_ll_int8(0);
	case TYPE_UBYTE:                   return get_ll_uint8(0);
	case TYPE_SHORT: case TYPE_CHAR16: return get_ll_int16(0);
	case TYPE_USHORT:                  return get_ll_uint16(0);
	case TYPE_INT: case TYPE_CHAR32:   return get_ll_int32(0);
	case TYPE_UINT:                    return get_ll_uint32(0);
	case TYPE_LONG:                    return get_ll_int64(0);
	case TYPE_ULONG:                   return get_ll_uint64(0);
	case TYPE_FLOAT:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat((float) 0.0F));
	case TYPE_DOUBLE:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat((double) 0.0));
	default:
		assert(!"Failed to implement default value for type");
		return nullptr;
	}
	return nullptr;
}

void nyla::llvm_generator::gen_default_array(sym_variable* sym_variable,
	nyla::type* type,
	llvm::Value* ll_arr_alloca,
	u32 depth) {
	llvm::Value* ll_ptr_to_arr = get_arr_ptr(type->element_type, ll_arr_alloca);
	
	u32 num_elements = sym_variable->computed_arr_dim_sizes[depth];

	// if more depths GEP into the pointers and let them MemSet otherwise memset at this level
	if (depth + 1 == sym_variable->computed_arr_dim_sizes.size()) {
		m_llvm_builder->CreateMemSet(
			ll_ptr_to_arr,
			get_ll_uint8(0), // Memory sets with bytes
			get_ll_uint32(num_elements * type->element_type->mem_size()),
			llvm::MaybeAlign(type->element_type->mem_size())
		);
	} else {
		for (u32 i = 0; i < num_elements; i++) {
			llvm::Value* ll_element =
				m_llvm_builder->CreateLoad(m_llvm_builder->CreateGEP(ll_ptr_to_arr, get_ll_uint32(i)));
			gen_default_array(sym_variable, type->element_type, ll_element, depth + 1);
		}
	}
}

llvm::Constant* nyla::get_ll_int1(bool tof) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt1Ty(*nyla::llvm_context), tof ? 1 : 0, true);
}
llvm::Constant* nyla::get_ll_int8(s32 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt8Ty(*nyla::llvm_context), value, true);
}
llvm::Constant* nyla::get_ll_uint8(u32 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt8Ty(*nyla::llvm_context), value, false);
}
llvm::Constant* nyla::get_ll_int16(s32 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt16Ty(*nyla::llvm_context), value, true);
}
llvm::Constant* nyla::get_ll_uint16(u32 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt16Ty(*nyla::llvm_context), value, false);
}
llvm::Constant* nyla::get_ll_int32(s32 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt32Ty(*nyla::llvm_context), value, true);
}
llvm::Constant* nyla::get_ll_uint32(u32 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt32Ty(*nyla::llvm_context), value, false);;
}
llvm::Constant* nyla::get_ll_int64(s64 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt64Ty(*nyla::llvm_context), value, true);
}
llvm::Constant* nyla::get_ll_uint64(u64 value) {
	return llvm::ConstantInt::get(
		llvm::IntegerType::getInt64Ty(*nyla::llvm_context), value, false);
}

void nyla::llvm_generator::branch_if_not_term(llvm::BasicBlock* ll_bb) {
	// Avoiding back-to-back branching
	llvm::Instruction* instruction = &m_llvm_builder->GetInsertBlock()->back();
	if (!instruction->isTerminator()) {
		// Unconditional branch
		m_llvm_builder->CreateBr(ll_bb);
	}
}
