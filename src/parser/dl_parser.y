%{
#include "common.h"
#include "mem_manager/mem_mgr.h"
#include "model/expression/expression.h"
#include "model/list/list.h"
#include "model/node/nodetype.h"
#include "model/datalog/datalog_model.h"
#include "model/query_block/query_block.h"
#include "parser/parse_internal_dl.h"
#include "log/logger.h"
#include "model/query_operator/operator_property.h"

#define RULELOG(grule) \
    { \
        TRACE_LOG("Parsing grammer rule <%s>", #grule); \
    }
    
#undef free
#undef malloc

Node *dlParseResult = NULL;
%}

%name-prefix "dl"

%union {
    /* 
     * Declare some C structure those will be used as data type
     * for various tokens used in grammar rules.
     */
     Node *node;
     List *list;
     char *stringVal;
     int intVal;
     double floatVal;
}

/*
 * Declare tokens for name and literal values
 * Declare tokens for user variables
 */
%token <stringVal> IDENT
%token <stringVal> VARIDENT

/* 
 * Functions and operators 
 */ 
%token <stringVal> AMMSC
%token <stringVal> '+' '-' '*' '/' '%' '^' '&' '|' '!' ')' '('
%token <stringVal> STRCONCAT 

/*
 * Tokens for in-built keywords
 *        Currently keywords related to basic query are considered.
 *        Later on other keywords will be added.
 */
%token <stringVal> NEGATION RULE_IMPLICATION ANS WHYPROV WHYNOTPROV GP

/* tokens for constant and idents */
%token <intVal> intConst
%token <floatVal> floatConst
%token <stringVal> stringConst

/* comparison and arithmetic operators */
%token <stringVal> comparisonOp

/*
 * Declare token for operators specify their associativity and precedence
 *
%left NEGATION

/* Arithmetic operators : FOR TESTING */
%left '+' '-'
%left '*' '/' '%'
%left STRCONCAT
%nonassoc '(' ')'

/*
 * Types of non-terminal symbols
 */
/* statements and their parts */ 
%type <stringVal> name
%type <list> stmtList
%type <node> statement program

%type <node> rule fact rulehead headatom relAtom bodyAtom arg comparison ansrelation provStatement
%type <node> variable constant expression functionCall binaryOperatorExpression 
%type <list> bodyAtomList argList exprList rulebody 

/* start symbol */
%start program

/*************************************************************/
/* RULE SECTION												 */
/*************************************************************/
%%

program:
		stmtList 
			{ 
				RULELOG("program::stmtList");
				$$ = (Node *) createDLProgram ($1, NULL, NULL);
				dlParseResult = (Node *) $$;
				DEBUG_LOG("parsed %s", nodeToString($$));
			}
		;

/* Rule for all types of statements */
stmtList: 
		statement
			{ 
				RULELOG("stmtList::statement"); 
				$$ = singleton($1);
			}
		| stmtList statement 
			{
				RULELOG("stmtlist::stmtList::statement");
				$$ = appendToTailOfList($1, $2); 
			}
	;

/*
 * Statements can be:
 *
 * 	- rules, e.g., Q(X) :- R(X,Y);
 *  - facts, e.g., R(1,2);
 * 	- answer relation declarations, e.g., ANS : Q;
 * 	- provenance requests, e.g., WHY(Q(1));
 */
statement:
		rule { RULELOG("statement::rule"); $$ = $1; }
		| fact { RULELOG("statement::fact"); $$ = $1; }
		| ansrelation { RULELOG("statement::ansrelation"); $$ = $1; }
		| provStatement { RULELOG("statement::prov"); $$ = $1; }
	;
	
provStatement:
		WHYPROV '(' relAtom ')' '.'
		{
			RULELOG("provStatement::WHY");
			$$ = (Node *) createNodeKeyValue(
					(Node *) createConstString("WHY_PROV"), 
					(Node *) $3);
		}
		| WHYNOTPROV '(' relAtom ')' '.'
		{
			RULELOG("provStatement::WHYNOT");
			$$ = (Node *) createNodeKeyValue(
					(Node *) createConstString("WHYNOT_PROV"),
					(Node *) $3);
		}
		| GP '.'
		{
			RULELOG("provStatement::GP");
			$$ = (Node *) createNodeKeyValue(
					(Node *) createConstString("FULL_GP_PROV"), 
					(Node *) createConstBool(TRUE));
		}
	;	
	
rule:
		rulehead RULE_IMPLICATION rulebody '.' 
			{ 
				RULELOG("rule::head::body"); 
				$$ = (Node *) createDLRule((DLAtom *) $1,$3); 
			}
	;
	
fact:
		rulehead '.' { RULELOG("fact::rulehead"); $$ = $1; }
	;

ansrelation:
		ANS ':' name '.'
		{
			RULELOG("ansrelation");
			$$ = (Node *) createConstString($3);
		}
	;

rulehead:
		headatom  { RULELOG("rulehead::atom"); $$ = $1; }
	;
	
rulebody:
		bodyAtomList { RULELOG("rulebody::atomList"); $$ = $1; }
	;
	
/* we allow for expressions in the head atom, e.g., Q(X + 1) :- R(X,Y); */
headatom:
		name '(' exprList ')'
			{
				RULELOG("");
				$$ = (Node *) createDLAtom($1, $3, FALSE);
			}
	;
	
bodyAtomList:
		bodyAtomList ',' bodyAtom
			{
				RULELOG("atomList::bodyAtom");
				$$ = appendToTailOfList($1,$3); 
			}
		| bodyAtom
			{
				RULELOG("atomList::atom");
				$$ = singleton($1); 
			}
	;

bodyAtom:
 		relAtom { RULELOG("bodyAtom::relAtom"); $$ = $1; }
		| comparison
			{
				RULELOG("atom::comparison");
				$$ = (Node *) $1;
			}
 	;

relAtom:
 		NEGATION name '(' argList ')' 
 			{ 
 				RULELOG("relAtom::negative");
 				$$ = (Node *) createDLAtom($2, $4, TRUE); 
			}
 		| name '(' argList ')' 
 			{
 				RULELOG("relAtom::positive");
 				$$ = (Node *) createDLAtom($1, $3, FALSE); 
			}		
	;
	
/*
constAtom:
		IDENT '(' constList ')' 
			{
				RULELOG("constAtom");
				$$ = createDLAtom($1,$3,FALSE); 
			}
	;
*/

argList:
 		argList ',' arg 
 			{
 				RULELOG("argList::argList::arg");
 				$$ = appendToTailOfList($1,$3); 
			}
 		| arg		
 			{
 				RULELOG("argList::arg");
 				$$ = singleton($1); 
			}
 	;

/* 	
constList:
		constList ',' constant { $$ = appendToTailOfList($1,$2); }
		| constant { $$ = singleton($1); }
	;
*/

comparison:
		arg comparisonOp arg
			{
				RULELOG("comparison::arg::op::arg");
				$$ = (Node *) createDLComparison($2, $1, $3);
			}
	;

/* args */ 	
arg:
 		variable 
 			{
 				RULELOG("arg:variable");
 		 		$$ = $1; 
	 		}
 		| constant 
 			{ 
 				RULELOG("arg:constant");
 				$$ = $1; 
			}
	;

/*
 * Rule to parse an expression list
 */
exprList: 
        expression        { RULELOG("exprList::SINGLETON"); $$ = singleton($1); }
        | exprList ',' expression
             {
                  RULELOG("exprList::exprList::expression");
                  $$ = appendToTailOfList($1, $3);
             }
    ;

	
/*
 * Rule to parse expressions used in various lists
 */
expression:
		'(' expression ')'				{ RULELOG("expression::bracked"); $$ = $2; } 
		| constant     				   	{ RULELOG("expression::constant"); }
        | variable 	        		  	{ RULELOG("expression::variable"); }
        | binaryOperatorExpression		{ RULELOG("expression::binaryOperatorExpression"); } 
        | functionCall	        		{ RULELOG("expression::functionCall"); }
    ;

/*
 * Parse operator expression
 */
 
binaryOperatorExpression: 

    /* Arithmatic Operations */
        expression '+' expression
            {
                RULELOG("binaryOperatorExpression:: '+' ");
                List *expr = singleton($1);
                expr = appendToTailOfList(expr, $3);
                $$ = (Node *) createOpExpr($2, expr);
            }
        | expression '-' expression
            {
                RULELOG("binaryOperatorExpression:: '-' ");
                List *expr = singleton($1);
                expr = appendToTailOfList(expr, $3);
                $$ = (Node *) createOpExpr($2, expr);
            }
        | expression '*' expression
            {
                RULELOG("binaryOperatorExpression:: '*' ");
                List *expr = singleton($1);
                expr = appendToTailOfList(expr, $3);
                $$ = (Node *) createOpExpr($2, expr);
            }
        | expression '/' expression
            {
                RULELOG("binaryOperatorExpression:: '/' ");
                List *expr = singleton($1);
                expr = appendToTailOfList(expr, $3);
                $$ = (Node *) createOpExpr($2, expr);
            }
        | expression '%' expression
            {
                RULELOG("binaryOperatorExpression:: '%' ");
                List *expr = singleton($1);
                expr = appendToTailOfList(expr, $3);
                $$ = (Node *) createOpExpr($2, expr);
            }
    /* String Operators */
        | expression STRCONCAT expression
            {
                RULELOG("binaryOperatorExpression::comparisonOps");
                List *expr = singleton($1);
                expr = appendToTailOfList(expr, $3);
                $$ = (Node *) createOpExpr("||", expr);
            }
    ;


/*
 * Rule to parse function calls (provision for aggregation)
 */
functionCall: 
        name '(' exprList ')'          
            {
                RULELOG("functionCall::IDENTIFIER::exprList");
				FunctionCall *f = createFunctionCall($1, $3);
            }
		| AMMSC '(' exprList ')'          
            {
                RULELOG("functionCall::AMMSC::exprList");
				FunctionCall *f = createFunctionCall($1, $3); 
            }
    ;

	
variable:
		VARIDENT 
			{
				RULELOG("variable"); 
				$$ = (Node *) createDLVar($1, DT_STRING); 
			}
	;
	
constant: 
		intConst 
			{
				RULELOG("constant::intConst"); 
				$$ = (Node *) createConstInt($1); 
			}
		| floatConst
			{
				RULELOG("constant::floatConst"); 
				$$ = (Node *) createConstFloat($1); 
			}
		| stringConst 
			{
				RULELOG("constant::stringConst");
				$$ = (Node *) createConstString($1); 
			}
	;
	
name:
		IDENT { RULELOG("name::IDENT"); $$ = $1; }
		| VARIDENT { RULELOG("name::VARIDENT"); $$ = $1; }
	;	