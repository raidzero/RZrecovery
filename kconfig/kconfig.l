%{
#define YYSTYPE char *
#include <stdlib.h>
#include "kconfig.tab.h"
extern YYSTYPE yylval;
extern int yylineno;
%}

%option yylineno

%%
#.*			return COMMENT;
\"[^\"]*\"		yylval=(char*)(yytext+1); yylval[strlen(yylval)-1]='\0'; return STRING;
menuconfig		return TOK_MENUCONFIG;
mainmenu		return TOK_MAINMENU;
config			return TOK_CONFIG;
endchoice		return TOK_ENDCHOICE;
choice			return TOK_CHOICE;
comment			return TOK_COMMENT;
endif			return TOK_ENDIF;
if			return TOK_IF;
source			return TOK_SOURCE;
bool			return TYPE_BOOL;
tristate		return TYPE_TRISTATE;
string			return TYPE_STRING;
hex			return TYPE_HEX;
int			return TYPE_INT;
depends\ on		return TOK_DEPENDS;
prompt			return TOK_PROMPT;
default			return TOK_DEFAULT;
def_bool		return TOK_DEF_BOOL;
def_tristate		return TOK_DEF_TRISTATE;
select			return TOK_SELECT;
range			return TOK_RANGE;
(---)?help(---)?	return TOK_HELP; // yes, this also matches "---help" and "help---".  I don't care.
option			return TOK_OPTION;
defconfig_list		return TOK_DEFCONFIG_LIST;
modules			return TOK_MODULES;
env			return TOK_ENV;
menu			return TOK_MENU;
endmenu			return TOK_ENDMENU;
optional		return TOK_OPTIONAL;

[a-zA-Z0-9_]*		yylval=strdup(yytext); return SYMBOL;
\"			return QUOTE;
#			return HASH;
!=			return NE;
&&			return AND;
\|\|			return OR;
.			return yytext[0];
\n			return (int)'\n';
%%
#ifndef yywrap
yywrap() {return 1;}
#endif
