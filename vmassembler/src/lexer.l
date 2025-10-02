%{
#include <parser.tab.h>
#include <stdlib.h>
%}

%%
0[xX][0-9a-fA-F]+		{ yylval.imm_val = strtoull(yytext, nullptr, 16); return IMM; }
[a-zA-Z0-9_]+:			{ strcpy(yylval.label_name, yytext); return LABEL; }
[a-zA-Z0-9_]+			{ strcpy(yylval.vinstr_name, yytext); return VINSTR; }
"\n"                    { yylineno++; }
[ ?\t\n\r]
%%