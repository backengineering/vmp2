%{
#include <stdlib.h>
#include <parser.hpp>
int yylex(void);
void yyerror(char *);
extern int yylineno;
%}


%union {
char vinstr_name[20];
char label_name[256];
unsigned long long imm_val;
}

%token <label_name> LABEL
%token <vinstr_name> VINSTR
%token <imm_val> IMM

%%
PROGRAM:
	LABEL  					{ parse_t::get_instance()->add_label($1); }
	| PROGRAM LABEL  					{ parse_t::get_instance()->add_label($2); }
	| PROGRAM VINSTRS
	|
	;

VINSTRS:
	VINSTR					{ parse_t::get_instance()->add_vinstr($1); }
	| VINSTR IMM			{ parse_t::get_instance()->add_vinstr($1, $2); }
	| VINSTRS VINSTR		{ parse_t::get_instance()->add_vinstr($2); }
	| VINSTRS VINSTR IMM	{ parse_t::get_instance()->add_vinstr($2, $3); }
	;
%%