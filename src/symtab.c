/**
 * @file symtab.c
 * @brief Implementation of the symbol table.
 * @author bennett
 * @date 2018-04-07
 */
#include "mCc/symtab.h"
#include "mCc/ast_statements.h"
#include <assert.h>
#include <stdio.h>

static void mCc_symtab_delete_scope(struct mCc_symtab_scope *scope);
static void mCc_symtab_delete_entry(struct mCc_symtab_entry *entry);

/*********************************** Global array of scopes for print and gc */

/// Block size by which to increase array when reallocating
static const unsigned int global_scope_gc_block_size = 15;
/// Number of entries for which memory was allocated
static unsigned int global_scope_gc_alloc_size = 0;
/// Number of entries currently in array
static unsigned int global_scope_gc_count = 0;

/// All scopes ever created, to use when printing or freeing
static struct mCc_symtab_scope **global_scope_gc_arr = NULL;

/*********************************** File-static helpers */

static int mCc_symtab_add_scope_to_gc(struct mCc_symtab_scope *scope)
{
	assert(scope);

	if (global_scope_gc_count < global_scope_gc_alloc_size) {
		global_scope_gc_arr[global_scope_gc_count++] = scope;
		return 0;
	}

	struct mCc_symtab_scope **tmp;
	global_scope_gc_alloc_size += global_scope_gc_block_size;
	if ((tmp = realloc(global_scope_gc_arr,
	                   global_scope_gc_alloc_size * sizeof(*tmp))) == NULL) {
		return 1; // Caller must delete all scopes if wanted
	}

	global_scope_gc_arr = tmp;
	global_scope_gc_arr[global_scope_gc_count++] = scope;
	return 0;
}

static inline void mCc_symtab_add_built_in_functions(struct mCc_symtab_scope *scope)
{
    assert(scope);

    struct mCc_ast_identifier func_id;
    struct mCc_ast_identifier param_id;
    struct mCc_ast_declaration decl;
    struct mCc_ast_parameters para;
    struct mCc_ast_function_def *built_in;

    func_id.id_value = "print";
    param_id.id_value = "msg";
    decl.decl_type = MCC_AST_TYPE_STRING;
    decl.decl_id = &param_id;
    para = *mCc_ast_new_parameters(&decl);

    built_in = mCc_ast_new_function_def_void(&func_id, &para, NULL);

    mCc_symtab_scope_add_func_def(scope, built_in);


  //  struct mCc_symtab_entry *found = mCc_symtab_scope_lookup_id(scope, &func_id);

//    assert(found);

  //  printf("\nParam Id: %s\n", found->params->decl[0]->decl_id->id_value);
  /*  mCc_ast_new_function_def_void();
    mCc_ast_new_function_def_void();
    mCc_ast_new_function_def_void();

    mCc_ast_new_function_def_type();
    mCc_ast_new_function_def_type();


    mCc_symtab_scope_add_func_def(scope, );
    mCc_symtab_scope_add_func_def(scope, );
    mCc_symtab_scope_add_func_def(scope, );
    mCc_symtab_scope_add_func_def(scope, );
    mCc_symtab_scope_add_func_def(scope, ); */
}


/**
 * @brief Symbol table (scope) constructor.
 *
 * @param parent The parent scope, to be inserted.
 * @param name The name suffix of the new scope
 *
 * @return The new scope
 */
static struct mCc_symtab_scope *
mCc_symtab_new_scope(struct mCc_symtab_scope *parent, char *name)
{
	assert(name);

	struct mCc_symtab_scope *new_scope = malloc(sizeof(*new_scope));
	if (!new_scope)
		return NULL;

	new_scope->parent = parent;
	new_scope->hash_table = NULL; // Important for uthash to function properly
	new_scope->name = name;

    if (!parent){
        mCc_symtab_add_built_in_functions(new_scope);
    }

	if (mCc_symtab_add_scope_to_gc(new_scope)) {
		mCc_symtab_delete_scope(new_scope);
		return NULL;
	}
	return new_scope;
}

/**
 * @brief Symbol table entry constructor.
 *
 * @param scope The containing scope
 * @param entry_type The type of the entry
 * @param sloc The source code location of the declaration
 * @param identifier The identifier (hash table key!)
 * @param decl_type The primitive type of the declaration
 * @param optarg Array size or #mCc_ast_parameters* if required by entry_type
 *
 * @return The new symtab entry, ready to be inserted
 */
static struct mCc_symtab_entry *mCc_symtab_new_entry(
    struct mCc_symtab_scope *scope, enum mCc_symtab_entry_type entry_type,
    struct mCc_ast_source_location sloc, struct mCc_ast_identifier *identifier,
    enum mCc_ast_type primitive_type, void *optarg)
{
	assert(scope);
	assert(identifier);

	struct mCc_symtab_entry *new_entry = malloc(sizeof(*new_entry));
	if (!new_entry)
		return NULL;

	new_entry->scope = scope;
	new_entry->entry_type = entry_type;
	new_entry->sloc = sloc;
	new_entry->identifier = identifier;
	new_entry->primitive_type = primitive_type;

	switch (entry_type) {
	case MCC_SYMTAB_ENTRY_TYPE_ARR:
		new_entry->arr_size = (unsigned int)optarg;
		break;
	case MCC_SYMTAB_ENTRY_TYPE_FUNC: new_entry->params = optarg; break;
	case MCC_SYMTAB_ENTRY_TYPE_VAR: break;
	}

	return new_entry;
}

/**
 * @brief Add an entry to a symbol table.
 *
 * Wraps around #HASH_ADD_KEYPTR
 *
 * @param self The scope to whose hash table the entry will be added
 * @param entry The entry to add
 */
static inline void mCc_symtab_scope_add_entry(struct mCc_symtab_scope *self,
                                              struct mCc_symtab_entry *entry)
{
	HASH_ADD_KEYPTR(hh, self->hash_table, entry->identifier->id_value,
	                strlen(entry->identifier->id_value), entry);
}

struct mCc_symtab_entry *
mCc_symtab_scope_lookup_id(struct mCc_symtab_scope *scope,
                           struct mCc_ast_identifier *id)
{
	struct mCc_symtab_entry *entry = NULL;
	HASH_FIND(hh, scope->hash_table, id->id_value, strlen(id->id_value), entry);

	// Recursively lookup until top scope
	if (!entry && scope->parent)
		return mCc_symtab_scope_lookup_id(scope->parent, id);
	return entry;
}

enum MCC_SYMTAB_SCOPE_LINK_ERROR *mCc_symtab_check_main_properties(struct mCc_symtab_scope *scope )
{
    struct mCc_symtab_entry *entry = NULL;
    HASH_FIND(hh, scope->hash_table, "main", strlen("main"), entry);

    if (entry){
        if(entry->primitive_type==MCC_AST_TYPE_INT){
            if (!entry->params) {
                return 0;   /// if all properties are full filled
            }
        }
    }
    return -1;
}

/******************************* Public Functions */

struct mCc_symtab_scope *mCc_symtab_new_scope_in(struct mCc_symtab_scope *self,
                                                 const char *childscope_name)
{
	assert(childscope_name);

	// create the scope name by concatenating it to the parent scope's name
	char *name;
	if (!self) {
		name = malloc(strlen(childscope_name) + 1);
		strcpy(name, childscope_name);
	} else {
		name = malloc(strlen(self->name) + strlen(childscope_name) +
		              2); // _ + null byte
		if (!name)
			return NULL;
		strcpy(name, self->name);
		strcat(name, "_");
		strcat(name, childscope_name);
	}

	// create scope
	return mCc_symtab_new_scope(self, name);
}

int mCc_symtab_scope_add_decl(struct mCc_symtab_scope *self,
                              struct mCc_ast_declaration *decl)
{
	enum mCc_symtab_entry_type entry_type = MCC_SYMTAB_ENTRY_TYPE_VAR;
	void *array_size = NULL;

	if (decl->decl_array_size) {
		entry_type = MCC_SYMTAB_ENTRY_TYPE_ARR;
		array_size = (void *)decl->decl_array_size->i_value;
	}

	struct mCc_symtab_entry *entry =
	    mCc_symtab_new_entry(self, entry_type, decl->node.sloc, decl->decl_id,
	                         decl->decl_type, array_size);
	if (!entry)
		return -1;

	// Check whether the ID was declared in the same scope
	struct mCc_symtab_entry *tmp = NULL;
	HASH_FIND(hh, self->hash_table, decl->decl_id->id_value,
	          strlen(decl->decl_id->id_value), tmp);
	if (tmp) {
		mCc_symtab_delete_entry(entry);
		return 1;
	}

	mCc_symtab_scope_add_entry(self, entry);

	return 0;
}

int mCc_symtab_scope_add_func_def(struct mCc_symtab_scope *self,
                                  struct mCc_ast_function_def *func_def)
{

	enum mCc_symtab_entry_type entry_type = MCC_SYMTAB_ENTRY_TYPE_FUNC;
	struct mCc_symtab_entry *entry = mCc_symtab_new_entry(
	    self, entry_type, func_def->node.sloc, func_def->identifier,
	    func_def->func_type, func_def->para);
	if (!entry)
		return -1;

	// Check whether the ID was declared in the same scope
	struct mCc_symtab_entry *tmp = NULL;
	HASH_FIND(hh, self->hash_table, func_def->identifier->id_value,
	          strlen(func_def->identifier->id_value), tmp);
	if (tmp) {
		mCc_symtab_delete_entry(entry);
		return 1;
	}

	mCc_symtab_scope_add_entry(self, entry);

	return 0;
}

enum MCC_SYMTAB_SCOPE_LINK_ERROR
mCc_symtab_scope_link_ref_expression(struct mCc_symtab_scope *self,
                                     struct mCc_ast_expression *expr)
{
	// Get the ID to link
	struct mCc_ast_identifier *id;
	switch(expr->type) {
		case MCC_AST_EXPRESSION_TYPE_IDENTIFIER:
			id = expr->identifier;
			break;
		case MCC_AST_EXPRESSION_TYPE_ARR_SUBSCR:
			id = expr->array_id;
			break;
		case MCC_AST_EXPRESSION_TYPE_CALL_EXPR:
			id = expr->f_name;
			break;
		default:
			return MCC_SYMTAB_SCOPE_LINK_ERROR_INVALID_AST_OBJECT;
	}

	struct mCc_symtab_entry *entry = mCc_symtab_scope_lookup_id(self, id);
	if (!entry)
		return MCC_SYMTAB_SCOPE_LINK_ERR_UNDECLARED_ID;

	// Basic error checking, though not full type checking
	switch (entry->entry_type) {
		case MCC_SYMTAB_ENTRY_TYPE_FUNC:
			if (expr->type != MCC_AST_EXPRESSION_TYPE_CALL_EXPR)
				return MCC_SYMTAB_SCOPE_LINK_ERR_FUN_WITHOUT_CALL;
			break;
		case MCC_SYMTAB_ENTRY_TYPE_ARR:
			if (expr->type != MCC_AST_EXPRESSION_TYPE_ARR_SUBSCR)
				return MCC_SYMTAB_SCOPE_LINK_ERR_ARR_WITHOUT_BRACKS;
			break;
		case MCC_SYMTAB_ENTRY_TYPE_VAR:
			if (expr->type != MCC_AST_EXPRESSION_TYPE_IDENTIFIER)
				return MCC_SYMTAB_SCOPE_LINK_ERR_VAR;
			break;

	// TODO: Link in identifier
	// id->symtab_ref = entry;
	// return MCC_SYMTAB_SCOPE_LINK_ERR_OK;
	}

}

enum MCC_SYMTAB_SCOPE_LINK_ERROR
mCc_symtab_scope_link_ref_assignment(struct mCc_symtab_scope *self,
                                     struct mCc_ast_statement *stmt)
{
}

/******************************* Destructors */
static void mCc_symtab_delete_scope(struct mCc_symtab_scope *scope)
{
	// Free all entries in the hash table
	struct mCc_symtab_entry *e, *tmp;
	HASH_ITER(hh, scope->hash_table, e, tmp) {
		HASH_DEL(scope->hash_table, e);
		mCc_symtab_delete_entry(e);
	}

	free(scope->name);
	free(scope);
}

static void mCc_symtab_delete_entry(struct mCc_symtab_entry *entry)
{
	free(entry);
}

void mCc_symtab_delete_all_scopes(void)
{
	for (unsigned int i = 0; i < global_scope_gc_count; ++i) {
		mCc_symtab_delete_scope(global_scope_gc_arr[i]);
	}

	if (global_scope_gc_arr) {
		free(global_scope_gc_arr);
		global_scope_gc_arr = NULL;
	}

	global_scope_gc_count = 0;
	global_scope_gc_alloc_size = 0;
}