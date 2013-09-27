/*
 *This is the lexer file which defines tokens
 *
 *
 */


/*******************************************************************************
 * options section 
 ******************************************************************************/

/* set lexer options */
%option noyywrap nodefault yylineno

/* includes */
%{
#include <stdio.h>
#include "log/logger.h"
#include "model/expression/expression.h"
#include "model/list/list.h"
#include "model/node/nodetype.h"
#include "parser/parse_internal.h"
#include "sql_parser.tab.h"

#define TOKSELF() { TRACE_LOG("Lexed TOKENSELF <%c> with VALUE <%c>", yytext[0], yytext[0]); return yytext[0]; }
#define TOK(name) { TRACE_LOG("Lexed TOKEN <%s> with VALUE <%s>", #name, yytext); yylval.stringVal = strdup(yytext); return name; }
#define TOKSAVE(name,field,function) { TRACE_LOG("Lexed TOKEN <%s> of TYPE <%s>", #name, #field); yylval.field = function(yytext); return name; }
%}

/********************************************************************
 * lexer states 
 */

%s SQL

/********************************************************************
 * predefined regular expressions 
 */

/* whitespace */
space			[\t\f\n\r ]+
newline			[\n\r]
nonewline		[^\n\r]

comment			("--"{nonewline}*)

whitespace		({space}+|{comment})

/* numbers */
integer 		[0-9]+
decimal 		({integer}*\.{integer}+|{integer}+\.{integer}*)
float 			(({integer}|{decimal})([eE][+-]?{integer})?)

/* names (e.g., tables, attributes) */
identStart		[A-Za-z]
identContinue	[A-Za-z0-9_]
identifiers		({identStart}{identContinue}*)

/* operators */
comparisonops	("="|"<>"|"!="|"<"|">"|"<="|">=")
arithmeticops	("+"|"-"|"/"|"*"|"%"|"&"|"|"|"^")

/*******************************************************************************
 * Token definitions and actions 
 ******************************************************************************/
%%

	/* ignore all whitespace */
{whitespace}	 { TRACE_LOG("Lexed whitespace <%s>", yytext); /* ignore it */ }

	/* literal keyword tokens */
SELECT      TOK(SELECT)
PROVENANCE  TOK(PROVENANCE)
OF          TOK(OF)
FROM        TOK(FROM)
AS          TOK(AS)
WHERE       TOK(WHERE)
DISTINCT    TOK(DISTINCT)
ON          TOK(ON)
STARLL      TOK(STARALL)
ALL		 TOK(ALL)
AND		 TOK(AND)
AVG		 TOK(AMMSC)
MIN		 TOK(AMMSC)
MAX		 TOK(AMMSC)
SUM		 TOK(AMMSC)
COUNT		 TOK(AMMSC)
ANY		 TOK(ANY)
BETWEEN	 TOK(BETWEEN)
BY			 TOK(BY)
UPDATE		 TOK(UPDATE)
DELETE	     TOK(DELETE)
IS		     TOK(IS)
NULL	     TOK(NULLVAL)

	/* punctuation */
[(),.;] TOKSELF()

	/* operators */
{comparisonops}		TOK(comparisonop)
{arithmeticops}		TOK(arithmeticop)
	
	/* names */
{identifiers}	TOK(identifier)

	/* numbers */
{integer}	TOKSAVE(intConst,intVal,atoi)
{float}	TOKSAVE(floatConst,floatVal,atof)

	/* strings */
'[^'\n]*'	{
		int c = input();
		unput(c);	
		if(c != '\'') {
			yylval.stringVal = strdup(yytext);
			return stringConst;
		} else
			yymore();
	}

		
'[^'\n]*$	{yyerror("Unterminated string"); }

	/* failure - no token matched */
.		{ yyerror("Unknown symbol"); }	/* random non-SQL text */

%%

/*******************************************************************************
 * Literal C code to include
 ******************************************************************************/

/* use logging framework in the future */
void yyerror(char *s)
{
	ERROR_LOG("%s at %s\n", s, yytext);
}