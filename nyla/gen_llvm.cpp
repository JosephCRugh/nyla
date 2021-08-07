#include "gen_llvm.h"

#include "type.h"

using namespace nyla;

llvm::LLVMContext* nyla::llvm_context;
llvm::IRBuilder<>* nyla::llvm_builder;
llvm::Module*      nyla::working_llvm_module;

void nyla::init_llvm() {
	nyla::llvm_context = new llvm::LLVMContext;
	nyla::llvm_builder = new llvm::IRBuilder<>(*llvm_context);
}

void nyla::init_native_target() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetAsmPrinter();
}

void nyla::clean_llvm() {
	delete nyla::llvm_context;
	delete nyla::llvm_builder;
}

bool nyla::write_obj_file(c_string fname) {

	auto target_triple = llvm::sys::getDefaultTargetTriple();
	nyla::working_llvm_module->setTargetTriple(target_triple);

	std::string target_error;
	auto target = llvm::TargetRegistry::lookupTarget(target_triple, target_error);

	if (!target) {
		llvm::errs() << target_error;
		return false;
	}

	auto CPU = "generic";
	auto features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	auto target_machine =
		target->createTargetMachine(target_triple, CPU, features, opt, RM);

	nyla::working_llvm_module->setDataLayout(target_machine->createDataLayout());

	std::error_code err;
	llvm::raw_fd_ostream dest(fname, err, llvm::sys::fs::OF_None);

	if (err) {
		llvm::errs() << "Could not open file: " << err.message();
		return false;
	}

	llvm::legacy::PassManager pass;
	if (target_machine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
		llvm::errs() << "TheTargetMachine can't emit a file of this type";
		return false;
	}

	pass.run(*nyla::working_llvm_module);
	dest.flush();

	llvm::outs() << "Wrote " << fname << "\n";

	return true;
}

void llvm_generator::gen_file_unit(nyla::afile_unit* file_unit, bool print_functions) {
	// Seperating generation of the function declaration
	// out so that the function exist for use later.
	std::vector<llvm::Function*> ll_functions;
	for (nyla::afunction* function : file_unit->functions) {
		gen_function(function);
	}

	for (nyla::afunction* function : file_unit->functions) {
		llvm::Function* ll_function = function->ll_function;
		if (!function->is_external) {
			m_bb = &ll_function->getEntryBlock();
			nyla::llvm_builder->SetInsertPoint(m_bb);
		}
		m_ll_function = ll_function;

		// scope may be nullptr if all that was parsed was
		// a function declaration.
		if (function->scope != nullptr) {
			m_scope = function->scope;
			for (nyla::aexpr* stmt : function->scope->stmts) {
				gen_expression(stmt);
			}
			m_scope = m_scope->parent;
		}

		if (print_functions) {
			ll_function->print(llvm::errs());
		}
	}
}

llvm::Function* llvm_generator::gen_function(nyla::afunction* function) {
	llvm::Type* ll_return_type = gen_type(function->return_type);

	std::vector<llvm::Type*> ll_parameter_types;
	for (nyla::avariable_decl* param : function->parameters) {
		ll_parameter_types.push_back(gen_type(param->variable->checked_type));
	}

	bool is_var_args = false;
	llvm::FunctionType* ll_function_type = llvm::FunctionType::get(ll_return_type, ll_parameter_types, is_var_args);

	llvm::Function* ll_function = llvm::Function::Create(
		ll_function_type,
		llvm::Function::ExternalLinkage, // publically visible
		function->name.c_str(),
		*nyla::working_llvm_module
	);

	if (function->is_external) {
		ll_function->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
		// TODO: the user should be allowed to set the calling convention in code.
		ll_function->setCallingConv(llvm::CallingConv::X86_StdCall); // TODO Windows only!
	}

	// Setting the names of the variables.
	u64 param_index = 0;
	for (auto& llvm_param : ll_function->args())
		llvm_param.setName(function->parameters[param_index++]->variable->name.c_str());

	if (!function->is_external) {
		// The Main block for the function.
		llvm::BasicBlock* ll_basic_block = llvm::BasicBlock::Create(*nyla::llvm_context, "function block", ll_function);
		nyla::llvm_builder->SetInsertPoint(ll_basic_block);
		m_bb = ll_basic_block;

		// Allocating memory for the parameters
		param_index = 0;
		for (auto& llvm_param : ll_function->args()) {
			llvm::AllocaInst* var_alloca = gen_alloca(function->parameters[param_index++]->variable);
			nyla::llvm_builder->CreateStore(&llvm_param, var_alloca); // Storing the incoming value
		}
	}

	function->ll_function = ll_function;
	return ll_function;
}

llvm::Value* nyla::llvm_generator::gen_function_call(nyla::afunction_call* function_call) {
	llvm::Function* ll_callee = function_call->called_function->ll_function;
	std::vector<llvm::Value*> ll_parameter_values;
	for (nyla::aexpr* parameter_value : function_call->parameter_values) {
		ll_parameter_values.push_back(gen_expression(parameter_value));
	}
	return nyla::llvm_builder->CreateCall(ll_callee, ll_parameter_values, "callt");
}

llvm::Type* llvm_generator::gen_type(nyla::type* type) {
	switch (type->tag) {
	case TYPE_BYTE:
	case TYPE_UBYTE:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_SHORT:
	case TYPE_USHORT:
		return llvm::Type::getInt16Ty(*nyla::llvm_context);
	case TYPE_INT:
	case TYPE_UINT:
		return llvm::Type::getInt32Ty(*nyla::llvm_context);
	case TYPE_LONG:
	case TYPE_ULONG:
		return llvm::Type::getInt64Ty(*nyla::llvm_context);
	case TYPE_FLOAT:
		return llvm::Type::getFloatTy(*nyla::llvm_context);
	case TYPE_DOUBLE:
		return llvm::Type::getDoubleTy(*nyla::llvm_context);
	case TYPE_BOOL:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_VOID:
		return llvm::Type::getVoidTy(*nyla::llvm_context);
	case TYPE_CHAR16:
		return llvm::Type::getInt16Ty(*nyla::llvm_context);
	case TYPE_PTR: {
		// Pointers
		switch (type->elem_tag) {
		case TYPE_BYTE:
		case TYPE_UBYTE:
			return llvm::Type::getInt8PtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_SHORT:
		case TYPE_USHORT:
			return llvm::Type::getInt16PtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_INT:
		case TYPE_UINT:
			return llvm::Type::getInt32PtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_LONG:
		case TYPE_ULONG:
			return llvm::Type::getInt64PtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_FLOAT:
			return llvm::Type::getFloatPtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_DOUBLE:
			return llvm::Type::getDoublePtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_BOOL:
			return llvm::Type::getInt8PtrTy(*nyla::llvm_context, type->ptr_depth);
		case TYPE_CHAR16:
			return llvm::Type::getInt16PtrTy(*nyla::llvm_context, type->ptr_depth);
		}
	}
	case TYPE_ARR: {
		return gen_type(type->get_element_type());
	}
	return nullptr;
	}
}

llvm::Value* llvm_generator::gen_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_RETURN:
		return gen_return(dynamic_cast<nyla::areturn*>(expr));
	case AST_VARIABLE_DECL:
		return gen_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr));
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
		return gen_number(dynamic_cast<nyla::anumber*>(expr));
	case AST_VALUE_STRING:
		return gen_string(dynamic_cast<nyla::astring*>(expr));
	case AST_BINARY_OP:
		return gen_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
	case AST_UNARY_OP:
		return gen_unary_op(dynamic_cast<nyla::aunary_op*>(expr));
	case AST_VARIABLE:
		return gen_variable(dynamic_cast<nyla::avariable*>(expr));
	case AST_FOR_LOOP:
		return gen_for_loop(dynamic_cast<nyla::afor_loop*>(expr));
	case AST_TYPE_CAST:
		return gen_type_cast(dynamic_cast<nyla::atype_cast*>(expr));
	case AST_FUNCTION_CALL:
		return gen_function_call(dynamic_cast<nyla::afunction_call*>(expr));
	case AST_ARRAY_ACCESS:
		return gen_array_access(dynamic_cast<nyla::aarray_access*>(expr));
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_return(nyla::areturn* ret) {
	if (ret->value == nullptr) {
		// Void return.
		nyla::llvm_builder->CreateRet(
			llvm::UndefValue::get(llvm::Type::getVoidTy(*nyla::llvm_context)));
	}
	return nyla::llvm_builder->CreateRet(gen_expression(ret->value));
}

llvm::Value* llvm_generator::gen_number(nyla::anumber* number) {
	switch (number->tag) {
	case AST_VALUE_BYTE:
	case AST_VALUE_UBYTE:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt8Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_SHORT:
	case AST_VALUE_USHORT:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt16Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_INT:
	case AST_VALUE_UINT:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt32Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_LONG:
	case AST_VALUE_ULONG:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt64Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_FLOAT:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat(number->value_float));
	case AST_VALUE_DOUBLE:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat(number->value_double));
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_string(nyla::astring* str) {
	std::vector<llvm::Constant*> chars_as_16bits;
	u32 array_size = str->lit.length();
	for (u32 i = 0; i < array_size; i++) {
		u16 ch = str->lit[i];
		chars_as_16bits.push_back(llvm::ConstantInt::get(
			llvm::IntegerType::getInt16Ty(*nyla::llvm_context), ch, false
		));
	}

	llvm::Type* elements_type = llvm::Type::getInt16Ty(*nyla::llvm_context);
	llvm::ArrayType* array_type = llvm::ArrayType::get(elements_type, array_size);

	llvm::Constant* arr = llvm::ConstantArray::get(
		array_type,
		chars_as_16bits
	);
	
	llvm::IRBuilder<> tmp_builder(m_bb, m_bb->begin());
	llvm::AllocaInst* var_alloca;
	var_alloca = tmp_builder.CreateAlloca(
		array_type,
		llvm::ConstantInt::get(
			llvm::IntegerType::getInt32Ty(*nyla::llvm_context), array_size, false),
		"arrt");
	tmp_builder.CreateStore(arr, var_alloca);
	
	// [n x i16*] to i16*
	return nyla::llvm_builder->CreateAddrSpaceCast(var_alloca,
		llvm::IntegerType::getInt16PtrTy(*nyla::llvm_context, 1));
}

llvm::Value* llvm_generator::gen_binary_op(nyla::abinary_op* binary_op) {
	switch (binary_op->op) {
	case '=': {
		nyla::avariable* variable = dynamic_cast<nyla::avariable*>(binary_op->lhs);
		llvm::AllocaInst* ll_variable =
			m_sym_table.get_declared_variable(m_scope, variable)->ll_alloca;
		if (binary_op->rhs->tag == AST_ARRAY) {
			return gen_array(dynamic_cast<nyla::aarray*>(binary_op->rhs), ll_variable);
		} else {
			llvm::Value* ll_value = gen_expression(binary_op->rhs);
			nyla::llvm_builder->CreateStore(ll_value, ll_variable);
			return ll_value; // In case of multiple assignments they
							 // may assign the value assigned lower on the tree.
							 // Ex.     a = b = c + 5;    c + 5 Value is returned
							 //                           which is then used to assign
							 //                           a and b.
		}
	}
	case TK_ARRAY_LENGTH: {
		nyla::avariable* declared_variable = m_sym_table.get_declared_variable(m_scope,
			dynamic_cast<nyla::avariable*>(binary_op->lhs));
		nyla::anumber* dimension_index = dynamic_cast<nyla::anumber*>(binary_op->rhs);
		return declared_variable->ll_arr_sizes[declared_variable->ll_arr_sizes.size() - dimension_index->value_int - 1];
		break;
	}
	case '+': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateAdd(ll_lhs, ll_rhs, "addt");
		}
		return nyla::llvm_builder->CreateFAdd(ll_lhs, ll_rhs, "addft");
	}
	case '-': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateSub(ll_lhs, ll_rhs, "subt");
		}
		return nyla::llvm_builder->CreateFSub(ll_lhs, ll_rhs, "subft");
	}
	case '*': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateMul(ll_lhs, ll_rhs, "mult");
		}
		return nyla::llvm_builder->CreateFMul(ll_lhs, ll_rhs, "mulft");
	}
	case '/': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			if (binary_op->checked_type->is_signed()) {
				return nyla::llvm_builder->CreateSDiv(ll_lhs, ll_rhs, "divt");
			} else {
				return nyla::llvm_builder->CreateUDiv(ll_lhs, ll_rhs, "divt");
			}
		}
		return nyla::llvm_builder->CreateFDiv(ll_lhs, ll_rhs, "divft");
	}
	case '<': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->lhs->checked_type->is_int()) {
			return nyla::llvm_builder->CreateICmpULT(ll_lhs, ll_rhs, "cmplt");
		}
		return nyla::llvm_builder->CreateFCmpULT(ll_lhs, ll_rhs, "cmplt");
	}
	case '>': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->lhs->checked_type->is_int()) {
			return nyla::llvm_builder->CreateICmpUGT(ll_lhs, ll_rhs, "cmpgt");
		}
		return nyla::llvm_builder->CreateFCmpUGT(ll_lhs, ll_rhs, "cmpgt");
	}
	default:
		assert(!"An operator has not been handled!");
		break;
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_unary_op(nyla::aunary_op* unary_op) {
	switch (unary_op->op) {
	case '-':
		llvm::Value * ll_factor = gen_expression(unary_op->factor);
		if (unary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateNeg(ll_factor, "negt");
		}
		return nyla::llvm_builder->CreateFNeg(ll_factor, "fnegt");
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_variable_decl(nyla::avariable_decl* variable_decl) {
	llvm::Value* var_alloca = gen_alloca(variable_decl->variable);
	if (variable_decl->assignment != nullptr) {
		gen_expression(variable_decl->assignment);
	}
	return var_alloca;
}

llvm::Value* llvm_generator::gen_for_loop(nyla::afor_loop* for_loop) {
	
	m_scope = for_loop->scope;

	// Generating declarations before entering the loop
	for (nyla::avariable_decl* var_decl : for_loop->declarations) {
		gen_variable_decl(var_decl);
	}

	// Top of loop with condition
	llvm::BasicBlock* ll_cond_bb = llvm::BasicBlock::Create(*llvm_context, "loopcond", m_ll_function);

	// Jumping directly into the loop condition
	nyla::llvm_builder->CreateBr(ll_cond_bb);
	// Telling llvm we want to put code into the loop condition block
	nyla::llvm_builder->SetInsertPoint(ll_cond_bb);

	llvm::BasicBlock* ll_loop_body_bb = llvm::BasicBlock::Create(*llvm_context, "loopbody", m_ll_function);

	// Generating the condition and telling it to jump to the body or the finish point
	llvm::Value* ll_cond = gen_expression(for_loop->cond);
	llvm::BasicBlock* ll_finish_bb = llvm::BasicBlock::Create(*llvm_context, "finishloop", m_ll_function);
	nyla::llvm_builder->CreateCondBr(ll_cond, ll_loop_body_bb, ll_finish_bb);
	
	// Telling llvm we want to put code into the loop body
	nyla::llvm_builder->SetInsertPoint(ll_loop_body_bb);

	// Generating the body of the loop
	for (nyla::aexpr* stmt : for_loop->scope->stmts) {
		gen_expression(stmt);
	}
	
	// Unconditional branch back to the condition
	nyla::llvm_builder->CreateBr(ll_cond_bb);

	// All new code will be inserted into the block after
	// the loop
	nyla::llvm_builder->SetInsertPoint(ll_finish_bb);
	
	m_scope = m_scope->parent;

	return nullptr;
}

llvm::Value* llvm_generator::gen_variable(nyla::avariable* variable) {
	// Assumed that the variable is already loaded and in scope!
	return nyla::llvm_builder->CreateLoad(
		m_sym_table.get_declared_variable(m_scope, variable)->ll_alloca,
		variable->name.c_str());;
}

llvm::Value* llvm_generator::gen_type_cast(nyla::atype_cast* type_cast) {
	nyla::type*  val_type     = type_cast->value->checked_type;
	nyla::type*  cast_to_type = type_cast->checked_type;
	llvm::Value* ll_val       = gen_expression(type_cast->value);
	llvm::Type*  ll_cast_type = gen_type(type_cast->checked_type);
	
	if (val_type->is_int() && cast_to_type->is_int()) {
		if (cast_to_type->get_mem_size() < val_type->get_mem_size()) {
			// Signed and unsigned downcasting use trunc
			return nyla::llvm_builder->CreateTrunc(ll_val, ll_cast_type, "trunctt");
		} else {
			if (cast_to_type->is_signed()) {
				// Signed upcasting
				return nyla::llvm_builder->CreateSExt(ll_val, ll_cast_type, "supcastt");
			} else {
				// Unsigned upcasting
				return nyla::llvm_builder->CreateZExt(ll_val, ll_cast_type, "uuocastt");
			}
		}
	} else if (cast_to_type->is_float() && val_type->is_int()) {
		// Int to floating point
		if (val_type->is_signed()) {
			return nyla::llvm_builder->CreateSIToFP(ll_val, ll_cast_type, "sitofpt");
		} else {
			return nyla::llvm_builder->CreateUIToFP(ll_val, ll_cast_type, "uitofpt");
		}
	} else if (cast_to_type->is_int() && val_type->is_float()) {
		// Floating point to Int
		if (cast_to_type->is_signed()) {
			return nyla::llvm_builder->CreateFPToSI(ll_val, ll_cast_type, "fptosit");
		} else {
			return nyla::llvm_builder->CreateFPToUI(ll_val, ll_cast_type, "fptouit");
		}
	} else if (cast_to_type->is_float() && val_type->is_float()) {
		// Float to Float
		if (cast_to_type->get_mem_size() > val_type->get_mem_size()) {
			// Upcasting float
			return nyla::llvm_builder->CreateFPExt(ll_val, ll_cast_type, "fpupcastt");
		} else {
			// Downcasting float
			return nyla::llvm_builder->CreateFPTrunc(ll_val, ll_cast_type, "fptrunctt");
		}
	} else if (cast_to_type->is_int() && val_type->is_ptr()) {
		return nyla::llvm_builder->CreatePtrToInt(ll_val, ll_cast_type, "itoptrt");
	} else if (cast_to_type->is_ptr() && val_type->is_int()) {
		return nyla::llvm_builder->CreateIntToPtr(ll_val, ll_cast_type, "ptrtoit");
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_array(nyla::aarray* arr, llvm::AllocaInst* llvm_arr) {
	// TODO: might want to use memcpy or something to move the values into the array instead
	for (u64 index = 0; index < arr->elements.size(); index++) {
		llvm::Value* gep = nyla::llvm_builder->CreateGEP(llvm_arr, llvm::ConstantInt::get(
			llvm::IntegerType::getInt64Ty(*nyla::llvm_context), index, false));	
		nyla::llvm_builder->CreateStore(gen_expression(arr->elements[index]), gep);
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_array_access(nyla::aarray_access* array_access) {
	nyla::avariable* decl_variable = m_sym_table.get_declared_variable(m_scope, array_access->variable);
	// Have to reverse indexes since original order is [fst][snd][...] but
	// the offsets are the opposite.
	std::vector<nyla::aexpr*> indexes = array_access->indexes;
	llvm::Value* total_index_offset =
		gen_expression(indexes[indexes.size()-1]);
	for (s32 i = indexes.size() - 2; i >= 0; i--) {
		llvm::Value* arr_index = gen_expression(array_access->indexes[i]);
		llvm::Value* index_offset =
			nyla::llvm_builder->CreateMul(arr_index, decl_variable->ll_arr_mem_offsets[i]);
		total_index_offset = nyla::llvm_builder->CreateAdd(total_index_offset, index_offset);
	}
	llvm::Value* gep = nyla::llvm_builder->CreateGEP(decl_variable->ll_alloca, total_index_offset);
	return nyla::llvm_builder->CreateLoad(gep);
}

llvm::AllocaInst* llvm_generator::gen_alloca(nyla::avariable* variable) {
	llvm::IRBuilder<> tmp_builder(m_bb, m_bb->begin());
	llvm::AllocaInst* var_alloca;
	llvm::Value* array_size = nullptr;
	std::vector<nyla::aexpr*> array_depths = variable->checked_type->array_depths;
	if (array_depths.size() > 1)
		variable->ll_arr_mem_offsets.reserve(array_depths.size()-1);
	// The size of the array is the sum of multiples
	// of it's depths
	if (variable->checked_type->is_arr()) {
		array_size = gen_expression(array_depths[array_depths.size() -1]);
		variable->ll_arr_sizes.push_back(array_size);
		for (s32 i = array_depths.size()-2; i >= 0; i--) {
			llvm::Value* dimension_size = gen_expression(variable->checked_type->array_depths[i]);
			variable->ll_arr_sizes.push_back(dimension_size);
			variable->ll_arr_mem_offsets.push_back(array_size);
			array_size = nyla::llvm_builder->CreateMul(array_size, dimension_size);
		}
	}
	var_alloca = tmp_builder.CreateAlloca(
		gen_type(variable->checked_type),
		array_size,
		variable->name.c_str());
	variable->ll_alloca = var_alloca;
	return var_alloca;
}
