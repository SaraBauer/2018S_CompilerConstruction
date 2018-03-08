# Assignment 1

## Tasks 1 and 2: AST, lexer, parser

```
###Primitives

alpha            = /[a-zA-Z_]/

alpha_num        = /[a-zA-Z0-9_]/

digit            = /[0-9]/

identifier       = alpha , [ { alpha_num } ]

bool_literal     = "true" | "false"

int_literal      = { digit }

float_literal    = { digit } , "." , { digit }

string_literal   = /"[^"]*"/


### Operators

unary_op         = "-" | "!"

binary_op        = "+"  | "-" | "*" | "/"
                 | "<"  | ">" | "<=" | ">="
                 | "&&" | "||"
                 | "==" | "!="


### Types

type             = "bool" | "int" | "float" | "string"


### Declaration / Assignment

declaration      = type , [ "[" , int_literal , "]" ] , identifier

assignment       = identifier , [ "[" , expression , "]" ] , "=" , expression


### Expressions

expression       = single_expr , [ binary_op , expression ]

single_expr      = literal
                 | identifier , [ "[" , expression , "]" ]
                 | call_expr
                 | unary_op , expression
                 | "(" , expression , ")"

literal          = bool_literal
                 | int_literal
                 | float_literal
                 | string_literal


### Statements

statement        = if_stmt
                 | while_stmt
                 | ret_stmt
                 | declaration , ";"
                 | assignment  , ";"
                 | expression  , ";"
                 | compound_stmt

if_stmt          = "if" , "(" , expression , ")" , statement , [ "else" , statement ]

while_stmt       = "while" , "(" , expression , ")" , statement

ret_stmt         = "return" , [ expression ] , ";"

compound_stmt    = "{" , [ { statement } ] , "}"


### Function Definition / Call

function_def     = ( "void" | type ) , identifier , "(" , [ parameters ] , ")" , compound_stmt

parameters       = declaration , [ { "," , declaration } ]

call_expr        = identifier , "(" , [ arguments ] , ")"

arguments        = expression , [ { "," expression } ]


### Program

program          = [ { function_def } ]
```

## Task 3: Error handling

## Task 4: Examples