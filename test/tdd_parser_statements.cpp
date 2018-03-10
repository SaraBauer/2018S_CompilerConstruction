#include <gtest/gtest.h>

#include "mCc/ast.h"
#include "mCc/parser.h"

TEST(TDD_PARSER_STATEMENTS, EXPRESSION)
{
	const char str[] = "true < false;";
	auto result = mCc_parser_parse_string(str);

	ASSERT_EQ(MCC_PARSER_STATUS_OK, result.status);
	auto stmt = result.statement;

	// root
	ASSERT_EQ(MCC_AST_STATEMENT_TYPE_EXPR, stmt->type);

	ASSERT_EQ(MCC_AST_EXPRESSION_TYPE_BINARY_OP, stmt->expression->type);

	mCc_ast_delete_statement(stmt);
}

TEST(TDD_PARSER_STATEMENTS, IF)
{
	const char str[] = "if (true) 1;";
	auto result = mCc_parser_parse_string(str);

	ASSERT_EQ(MCC_PARSER_STATUS_OK, result.status);
	auto stmt = result.statement;

	// root
	ASSERT_EQ(MCC_AST_STATEMENT_TYPE_IF, stmt->type);

	// () expression
	ASSERT_EQ(MCC_AST_EXPRESSION_TYPE_LITERAL, stmt->if_cond->type);
	ASSERT_EQ(true, stmt->if_cond->literal->b_value);

	// if body
	ASSERT_EQ(MCC_AST_STATEMENT_TYPE_EXPR, stmt->if_stmt->type);
	ASSERT_EQ(MCC_AST_EXPRESSION_TYPE_LITERAL, stmt->if_stmt->expression->type);
	ASSERT_EQ(1, stmt->if_stmt->expression->literal->i_value);

	mCc_ast_delete_statement(stmt);
}


TEST(TDD_PARSER_STATEMENTS, IFELSE)
{
	const char str[] = "if (true) 1; else 0;";
	auto result = mCc_parser_parse_string(str);

	ASSERT_EQ(MCC_PARSER_STATUS_OK, result.status);
	auto stmt = result.statement;

	// root
	ASSERT_EQ(MCC_AST_STATEMENT_TYPE_IFELSE, stmt->type);

	// () expression
	ASSERT_EQ(MCC_AST_EXPRESSION_TYPE_LITERAL, stmt->if_cond->type);
	ASSERT_EQ(true, stmt->if_cond->literal->b_value);

	// if body
	ASSERT_EQ(MCC_AST_STATEMENT_TYPE_EXPR, stmt->if_stmt->type);
	ASSERT_EQ(MCC_AST_EXPRESSION_TYPE_LITERAL, stmt->if_stmt->expression->type);
	ASSERT_EQ(1, stmt->if_stmt->expression->literal->i_value);

	// else body
	ASSERT_EQ(MCC_AST_STATEMENT_TYPE_EXPR, stmt->else_stmt->type);
	ASSERT_EQ(MCC_AST_EXPRESSION_TYPE_LITERAL, stmt->else_stmt->expression->type);
	ASSERT_EQ(0, stmt->else_stmt->expression->literal->i_value);

	mCc_ast_delete_statement(stmt);
}