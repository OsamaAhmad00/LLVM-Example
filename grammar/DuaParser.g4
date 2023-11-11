parser grammar DuaParser;

options {
    tokenVocab = DuaLexer;
}

@parser::postinclude {
#include <string>
#include <ModuleCompiler.h>
}

@parser::members
{
private:

    ModuleCompiler* module_compiler;

public:

    void set_module_compiler(ModuleCompiler* module_compiler) {
        this->module_compiler = module_compiler;
    }

    template <typename T, typename ...Args>
    T* create(Args... args) {
        return module_compiler->create_node<T>(args...);
    }
}

starting_symbol
returns [TranslationUnitNode* result]
@init{ $result = create<TranslationUnitNode>(); }
    : module EOF
    ;

module
    : global_elements
    | /* empty */
    ;

global_elements
    : global_elements global_element
    | /* empty */
    ;

global_element
    : variable_decl_or_def
    | function_decl_or_def
    ;

variable_decl_or_def
    : variable_decl_or_def_no_simicolon ';'
    ;

variable_decl_or_def_no_simicolon
    : variable_decl_no_simicolon
    | variable_def_no_simicolon
    ;

function_decl_or_def
    : function_declaration
    | function_definition
    ;

variable_decl_no_simicolon
    : type Identifier
    ;

variable_def_no_simicolon
    : variable_decl_no_simicolon '=' expression
    ;

function_decl_no_simicolon
    : type Identifier '(' param_list ')'
    ;

function_declaration
    : function_decl_no_simicolon ';'
    ;

function_definition
    : function_decl_no_simicolon block_statement
    | function_decl_no_simicolon '=' expression ';'
    ;

param_list
    : comma_separated_var_decls var_arg_or_none
    | /* empty */
    ;

var_arg_or_none
    : ',' '...'
    | /* empty */
    ;

comma_separated_var_decls
    : variable_decl_no_simicolon
    | comma_separated_var_decls ',' variable_decl_no_simicolon
    ;

block_statement
    : scope_begin statements scope_end
    ;

statements
    : statements statement
    | /* empty */
    ;

statement
    : if
    | for
    | while
    | do_while
    | block_statement
    | expression_statement
    | variable_decl_or_def
    | return_statement
    | Break
    | Continue
    | ';'  // empty statement
    ;

expression
    : number
    | String
    | lvalue
    | block_expression
    | function_call
    | if_expression
    | when_expression
    | '(' expression ')'
    | expression '++'
    | expression '--'
    | '++' expression
    | '--' expression
    | '-'  expression
    | '+'  expression  // do nothing
    | '!'  expression
    | '~'  expression
    | '&' lvalue
    | expression '*' expression
    | expression '/' expression
    | expression '%' expression
    | expression '+' expression
    | expression '-' expression
    | expression '<<' expression
    | expression '>>' expression
    | expression '<'  expression
    | expression '>'  expression
    | expression '<=' expression
    | expression '>=' expression
    | expression '==' expression
    | expression '!=' expression
    | expression '&'  expression
    | expression '^'  expression
    | expression '|'  expression
    | expression '&&' expression
    | expression '||' expression
    | expression '?' expression ':' expression  // ternary conditional
    | lvalue '='   expression
    | lvalue '+='  expression
    | lvalue '-='  expression
    | lvalue '*='  expression
    | lvalue '/='  expression
    | lvalue '%='  expression
    | lvalue '<<=' expression
    | lvalue '>>=' expression
    | lvalue '&='  expression
    | lvalue '^='  expression
    | lvalue '|='  expression
    ;

return_statement
    : Return expression ';'
    ;

expression_statement
    : expression ';'
    ;

if
    : 'if' '(' expression ')' statement else_if_else_statement
    ;

else_if_else_statement
    : 'else' 'if' '(' expression ')' statement else_if_else_statement
    | else_statement
    ;

else_statement
    : 'else' statement
    | /* empty */
    ;

if_expression
    : 'if' '(' expression ')' expression else_if_else_expression
    ;

else_if_else_expression
    : 'else' 'if' '(' expression ')' expression else_if_else_expression
    | else_expression
    ;

else_expression
    : 'else' expression
    | /* empty */
    ;

when_expression
    : 'when' scope_begin when_list scope_end
    ;

when_list
    : when_list_no_else
    | when_list_no_else ',' 'else' '->' expression
    ;

when_list_no_else
    : when_item
    | when_list_no_else ',' when_item
    ;

when_item
    : expression '->' expression
    ;

comma_separated_multi_variable_decl_or_def
    : comma_separated_multi_variable_decl_or_def
      ',' variable_decl_or_def_no_simicolon
    | variable_decl_or_def_no_simicolon
    ;


comma_separated_multi_variable_decl_or_def_or_none
    : comma_separated_multi_variable_decl_or_def
    | /* empty */
    ;

for
    : 'for'
      '(' comma_separated_multi_variable_decl_or_def_or_none
      ';' expression_or_none
      ';' expression_or_none
      ')' statement
    ;

while
    : 'while' '(' expression_or_none ')' statement
    ;

do_while
    : 'do' statement 'while' '(' expression ')' ';'
    ;

expressions_list
    : comma_separated_expressions
    | /* empty */
    ;

comma_separated_expressions
    : expression
    | comma_separated_expressions ',' expression
    ;

expression_or_none
    : expression
    | /* empty */
    ;

block_expression
    : scope_begin statements expression scope_end
    ;

function_call
    : Identifier '(' expressions_list ')'
    ;

lvalue
    : Identifier
    | '*' expression  // lvalue or unary_expression?
    ;

number
    : integer
    | float
    | True
    | False
    | Null
    ;

integer
    : I64Val
    | I32Val
    | I16Val
    | I8Val
    ;

float
    : F64Val
    | F32Val
    ;

type
    : primitive_type
    | Identifier            // User-defined types
    | type '[' integer ']'  // array types
    | type '*'              // pointer types
    ;

primitive_type
    : I64
    | I32
    | I16
    | I8
    | F64
    | F32
    | Void
    ;

scope_begin: '{';

scope_end: '}';
