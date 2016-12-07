/*-----------------------------------------------------------------------------
 *
 * expression.c
 *			  
 *		
 *		AUTHOR: lord_pretzel
 *
 *		
 *
 *-----------------------------------------------------------------------------
 */

#include "common.h"
#include "mem_manager/mem_mgr.h"
#include "log/logger.h"
#include "metadata_lookup/metadata_lookup.h"
#include "model/node/nodetype.h"
#include "model/list/list.h"
#include "model/expression/expression.h"
#include "model/datalog/datalog_model.h"
#include "utility/string_utils.h"

typedef struct FindNotesContext
{
    NodeTag type;
    List **result;
} FindNotesContext;

static boolean findAllNodesVisitor(Node *node, FindNotesContext *context);
static boolean findAttrReferences (Node *node, List **state);
static boolean findDLVars (Node *node, List **state);
static boolean findDLVarsIgnoreProps (Node *node, List **state);
static DataType typeOfOp (Operator *op);
static DataType typeOfFunc (FunctionCall *f);


AttributeReference *
createAttributeReference (char *name)
{
    AttributeReference *result = makeNode(AttributeReference);

    result->name = strdup(name);
    result->fromClauseItem = INVALID_FROM_ITEM;
    result->attrPosition = INVALID_ATTR;
    result->outerLevelsUp = INVALID_ATTR;
    result->attrType= DT_STRING;

    return result;
}

AttributeReference *
createFullAttrReference (char *name, int fromClause, int attrPos,
        int outerLevelsUp, DataType attrType)
{
    AttributeReference *result = makeNode(AttributeReference);

    result->name = strdup(name);
    result->fromClauseItem = fromClause;
    result->attrPosition = attrPos;
    result->outerLevelsUp = outerLevelsUp;
    result->attrType= attrType;

    return result;
}

CastExpr *
createCastExpr (Node *expr, DataType resultDt)
{
    CastExpr *result = makeNode(CastExpr);

    result->expr = expr;
    result->resultDT = resultDt;

    return result;
}

SQLParameter *
createSQLParameter (char *name)
{
    SQLParameter *result = makeNode(SQLParameter);

    result->name = strdup(name);
    result->position = INVALID_PARAM;
    result->parType = DT_STRING;//TODO

    return result;
}

CaseExpr *
createCaseExpr (Node *expr, List *whenClauses, Node *elseRes)
{
    CaseExpr *result = makeNode(CaseExpr);

    result->expr = expr;
    result->whenClauses = whenClauses;
    result->elseRes = elseRes;

    return result;
}

CaseWhen *
createCaseWhen (Node *when, Node *then)
{
    CaseWhen *result = makeNode(CaseWhen);

    result->when = when;
    result->then = then;

    return result;
}

WindowBound *
createWindowBound (WindowBoundType bType, Node *expr)
{
    WindowBound *result = makeNode(WindowBound);

    result->bType = bType;
    result->expr = expr;

    return result;
}

WindowFrame *
createWindowFrame (WinFrameType winType, WindowBound *lower, WindowBound *upper)
{
    WindowFrame *result = makeNode(WindowFrame);

    result->frameType = winType;
    result->lower = lower;
    result->higher = upper;

    return result;
}

WindowDef *
createWindowDef (List *partitionBy, List *orderBy, WindowFrame *frame)
{
    WindowDef *result = makeNode(WindowDef);

    result->partitionBy = partitionBy;
    result->orderBy = orderBy;
    result->frame = frame;

    return result;
}

WindowFunction *
createWindowFunction (FunctionCall *f, WindowDef *win)
{
    WindowFunction *result = makeNode(WindowFunction);

    result->f = f;
    result->win = win;

    return result;
}

OrderExpr *
createOrderExpr (Node *expr, SortOrder order, SortNullOrder nullOrder)
{
    OrderExpr *result = makeNode(OrderExpr);

    result->expr = expr;
    result->order = order;
    result->nullOrder = nullOrder;

    return result;
}

Node *
andExprs (Node *expr, ...)
{
    Node *result = NULL;
    Node *curArg = NULL;
    List *argList = singleton(expr);

    va_list args;

    va_start(args, expr);

    while((curArg = va_arg(args,Node*)))
        argList = appendToTailOfList(argList, copyObject(curArg));

    va_end(args);

    if (LIST_LENGTH(argList) == 1)
        return expr;

    result = (Node *) createOpExpr("AND", argList);

    return result;
}

Node *
andExprList (List *exprs)
{
    Node *result = popHeadOfListP(exprs);

    FOREACH(Node,e,exprs)
        result = AND_EXPRS(result,e);

    return result;
}

Node *
orExprs (Node *expr, ...)
{
    Node *result = NULL;
    Node *curArg = NULL;
    List *argList = singleton(expr);

    va_list args;

    va_start(args, expr);

    while((curArg = va_arg(args,Node*)))
        argList = appendToTailOfList(argList, copyObject(curArg));

    va_end(args);

    if (LIST_LENGTH(argList) == 1)
        return expr;

    result = (Node *) createOpExpr("OR", argList);

    return result;
}

FunctionCall *
createFunctionCall(char *fName, List *args)
{
    FunctionCall *result = makeNode(FunctionCall);
    
    if(fName != NULL)
    {
        result->functionname = (char *) CALLOC(1,strlen(fName) + 1);
        strcpy(result->functionname, fName);
    }
    else
        result->functionname = NULL;

    result->args = args; //should we copy?
    result->isAgg = FALSE;

    return result; 
}

Operator *
createOpExpr(char *name, List *args)
{
    Operator *result = makeNode(Operator);

    if(name != NULL)
    {
        result->name = (char *) CALLOC(1, strlen(name) + 1);
        strcpy(result->name, name);
    }
    else
        result->name = NULL;

    result->args = args;

    return result;
}

IsNullExpr *
createIsNullExpr (Node *expr)
{
    IsNullExpr *result = makeNode(IsNullExpr);

    result->expr = expr;

    return result;
}

Node *
createIsNotDistinctExpr (Node *lArg, Node *rArg)
{
    Operator *eq; //, *nullTest;

    eq = createOpExpr("=", LIST_MAKE(
            createFunctionCall("sys_op_map_nonnull", singleton(copyObject(lArg))),
            createFunctionCall("sys_op_map_nonnull", singleton(copyObject(rArg)))));
    return (Node *) eq;

//    eq = createOpExpr("=", LIST_MAKE(copyObject(lArg), copyObject(rArg)));
//    nullTest = createOpExpr("AND", LIST_MAKE(
//            createIsNullExpr(copyObject(lArg)),
//            createIsNullExpr(copyObject(rArg))));
//
//    return (Node *) createOpExpr("OR", LIST_MAKE(eq, nullTest));
}

Constant *
createConstInt (int value)
{
    Constant *result = makeNode(Constant);
    int *v = NEW(int);

    *v = value;
    result->constType = DT_INT;
    result->value = v;
    result->isNull = FALSE;

    return result;
}

Constant *
createConstLong (long value)
{
    Constant *result = makeNode(Constant);
    long *v = NEW(long);

    *v = value;
    result->constType  = DT_LONG;
    result->value = v;
    result->isNull = FALSE;

    return result;
}

Constant *
createConstString (char *value)
{
    Constant *result = makeNode(Constant);

    result->constType = DT_STRING;
    result->value = strdup(value);
    result->isNull = FALSE;

    return result;
}

Constant *
createConstFloat (double value)
{
    Constant *result = makeNode(Constant);
    double *v = NEW(double);

    *v = value;
    result->constType = DT_FLOAT;
    result->value = v;
    result->isNull = FALSE;

    return result;
}

Constant *
createConstBoolFromString (char *v)
{
    if (streq(v,"TRUE") || streq(v,"t") || streq(v,"true"))
        return createConstBool(TRUE);
    if (streq(v,"FALSE") || streq(v,"f") || streq(v,"false"))
        return createConstBool(FALSE);
    FATAL_LOG("not a boolean value %s", v);
    return FALSE; //keep compiler quiet
}

Constant *
createConstBool (boolean value)
{
    Constant *result = makeNode(Constant);
    boolean *v = NEW(boolean);

    *v = value;
    result->constType = DT_BOOL;
    result->value = v;
    result->isNull = FALSE;

    return result;
}

Constant *
createNullConst (DataType dt)
{
    Constant *result = makeNode(Constant);

    result->constType = dt;
    result->value = NULL;
    result->isNull = TRUE;

    return result;
}


DataType
typeOf (Node *expr)
{
    switch(expr->type)
    {
    	case T_AttributeReference:
        {
            AttributeReference *a = (AttributeReference *) expr;
            return a->attrType;
        }
        case T_Constant:
        {
            Constant *c = (Constant *) expr;
            return c->constType;
        }
        case T_FunctionCall:
        {
            FunctionCall *f = (FunctionCall *) expr;
            return typeOfFunc(f);
        }
        case T_WindowFunction:
            return typeOf((Node *) ((WindowFunction *) expr)->f);
        case T_Operator:
        {
            Operator *o = (Operator *) expr;
            return typeOfOp(o);
        }
        case T_CaseExpr:
        {
            CaseExpr *c = (CaseExpr *) expr;
            CaseWhen *w = (CaseWhen *) getNthOfListP(c->whenClauses, 0);
            return typeOf(w->then);
        }
        case T_IsNullExpr:
            return DT_BOOL;
        case T_RowNumExpr:
            return DT_INT;
        case T_SQLParameter:
            return ((SQLParameter *) expr)->parType;
        case T_OrderExpr:
            return DT_INT;//TODO should use something else?
        case T_DLVar:
            return ((DLVar *) expr)->dt;
        case T_CastExpr:
            return ((CastExpr *) expr)->resultDT;
        default:
             FATAL_LOG("unknown expression type for node: %s", nodeToString(expr));
             break;
    }
    return DT_STRING;
}

boolean
isConstExpr (Node *expr)
{
    if (expr == NULL)
        return TRUE;

    switch(expr->type)
    {
        case T_List:
        {
            FOREACH(Node,e,(List *) expr)
            {
                if (!isConstExpr(e))
                    return FALSE;
            }
            return TRUE;
        }
        case T_AttributeReference:
            return FALSE;
        case T_Constant:
            return TRUE;
        case T_FunctionCall:
        {
            FunctionCall *f = (FunctionCall *) expr;
            FOREACH(Node,arg,f->args)
            {
                if (!isConstExpr(arg))
                    return FALSE;
            }
            return TRUE;
        }
        case T_WindowFunction:
        {
            //TODO
            return FALSE;
        }
        case T_Operator:
        {
            Operator *o = (Operator *) expr;
            FOREACH(Node,arg,o->args)
            {
                if (!isConstExpr(arg))
                    return FALSE;
            }
            return TRUE;
        }
        case T_CaseExpr:
        {
            CaseExpr *c = (CaseExpr *) expr;
            if (!isConstExpr(c->expr))
                return FALSE;
            FOREACH(Node,w,c->whenClauses)
            {
                if (!isConstExpr(w))
                    return FALSE;
            }
            return isConstExpr(c->elseRes);
        }
        case T_CaseWhen:
        {
            CaseWhen *w = (CaseWhen *) expr;
            if (!isConstExpr(w->when))
                return FALSE;
            return isConstExpr(w->then);
        }
        case T_IsNullExpr:
        {
            IsNullExpr *n = (IsNullExpr *) expr;
            return isConstExpr(n->expr);
        }
        case T_RowNumExpr:
            return FALSE;
        case T_SQLParameter:
            return FALSE;
        case T_OrderExpr:
        {
            OrderExpr *o = (OrderExpr *) expr;
            return isConstExpr(o->expr);
        }
        case T_DLVar:
            return FALSE;
        case T_CastExpr:
        {
            CastExpr *c = (CastExpr *) expr;
            return isConstExpr(c->expr);
        }
        default:
             FATAL_LOG("unknown expression type for node: %s", nodeToString(expr));
             break;
    }
    return FALSE;
}

List *
createCasts(Node *lExpr, Node *rExpr)
{
	DataType lType, rType;
    lType = typeOf(lExpr);
    rType = typeOf(rExpr);

    // same expr return inputs
    if (lType == rType)
        return LIST_MAKE(lExpr,rExpr);

    // one is DT_STRING, cast the other one
    if (lType == DT_STRING)
        return LIST_MAKE(lExpr, createCastExpr(rExpr, DT_STRING));
    if (rType == DT_STRING)
        return LIST_MAKE(createCastExpr(lExpr, DT_STRING), rExpr);

    return LIST_MAKE(createCastExpr(lExpr, DT_STRING), createCastExpr(rExpr, DT_STRING));
}

DataType
SQLdataTypeToDataType (char *dt)
{
    //TODO outsource to metadatalookup for now does only Oracle
    if (isPrefix(dt, "NUMERIC") || streq(dt, "NUMBER"))
        return DT_INT; //TODO may also be float
    if (isPrefix(dt, "VARCHAR"))
        return DT_STRING;
    FATAL_LOG("unkown SQL datatype %s", dt);
    return DT_INT;
}

DataType
typeOfInOpModel (Node *expr, List *inputOperators)
{
    switch(expr->type)
    {
        case T_AttributeReference:
            return ((AttributeReference *) expr)->attrType;
        case T_FunctionCall:
            return typeOfFunc((FunctionCall *) expr);
        case T_Operator:
            return typeOfOp((Operator *) expr);
        case T_Constant:
            return typeOf(expr);
        case T_CaseExpr:
        {
            CaseExpr *c = (CaseExpr *) expr;
            CaseWhen *w = (CaseWhen *) getNthOfListP(c->whenClauses, 0);
            return typeOf(w->then);
        }
        case T_RowNumExpr:
            return DT_INT;
        case T_CastExpr:
            return ((CastExpr *) expr)->resultDT;
        default:
            ERROR_LOG("unknown expression type for node: %s", nodeToString(expr));
            break;
    }
    return DT_STRING;
}

/* figure out operator return type */
static DataType
typeOfOp (Operator *op)
{
    List *argDTs = NIL;

    FOREACH(Node,arg,op->args)
        argDTs = appendToTailOfListInt(argDTs,typeOf(arg));

    // logical operators
    if (streq(op->name,"OR")
            || streq(op->name,"AND")
            || streq(op->name,"NOT")
            )
        return DT_BOOL;
    // standard arithmetic operators
    if (streq(op->name,"+")
            || streq(op->name,"*")
            || streq(op->name,"/")
            || streq(op->name,"-")
            )
    {
        DataType dLeft = getNthOfListInt(argDTs,0);
        DataType dRight = getNthOfListInt(argDTs,1);

        // if the same input data types then we can safely assume that we get the same return data type
        // otherwise we use the metadata lookup plugin to make sure we get the right type
        if(dLeft == dRight)
            return dLeft;
    }

    // string ops
    if (streq(op->name,"||"))
        return DT_STRING;

    // comparison operators
    if (streq(op->name,"<")
            || streq(op->name,">")
            || streq(op->name,"<=")
            || streq(op->name,"=>")
            || streq(op->name,"<>")
            || streq(op->name,"^=")
            || streq(op->name,"=")
            || streq(op->name,"!=")
                )
        return DT_BOOL;

    return getOpReturnType(op->name, argDTs);
}

/* figure out function return type */
static DataType
typeOfFunc (FunctionCall *f)
{
    List *argDTs = NIL;

    FOREACH(Node,arg,f->args)
            argDTs = appendToTailOfListInt(argDTs,typeOf(arg));

    return getFuncReturnType(f->functionname, argDTs);
}


/* search functions */
List *
getAttrReferences (Node *node)
{
    List *result = NIL;

    findAttrReferences(node, &result);

    return result;
}

static boolean
findAttrReferences (Node *node, List **state)
{
    if (node == NULL)
        return TRUE;

    if (isA(node, AttributeReference))
    {
        *state = appendToTailOfList(*state, node);
    }

    return visit(node, findAttrReferences, state);
}

List *
getDLVars (Node *node)
{
    List *result = NIL;

    findDLVars(node, &result);

    return result;
}

static boolean
findDLVars (Node *node, List **state)
{
    if (node == NULL)
        return TRUE;

    if (isA(node, DLVar))
        *state = appendToTailOfList(*state, node);

    return visit(node, findDLVars, state);
}

List *
getDLVarsIgnoreProps (Node *node)
{
    List *result = NIL;

    findDLVarsIgnoreProps(node, &result);

    return result;
}

static boolean
findDLVarsIgnoreProps (Node *node, List **state)
{
    if (node == NULL)
        return TRUE;

    if (isA(node, HashMap))
        return TRUE;

    if (isA(node, DLVar))
        *state = appendToTailOfList(*state, node);

    return visit(node, findDLVars, state);
}

//TODO this is unsafe, callers are passing op as an operator even though it may not be one.
void
getSelectionCondOperatorList(Node *expr, List **opList)
{
    // only are interested in operators here
	if (isA(expr,Operator)) {
	    Operator *op = (Operator *) copyObject(expr);
	    if(streq(op->name,"AND"))
	    {
	        FOREACH(Node,arg,op->args)
                getSelectionCondOperatorList(arg,opList);
	    }
	    else
	        *opList = appendToTailOfList(*opList, op);
	}
}

Node *
changeListOpToAnOpNode(List *l1)
{
    List *helpList;
    Node *opNode1;

    if (LIST_LENGTH(l1) == 2)
        opNode1 = (Node *) createOpExpr("AND", (List *) l1);
    else if(LIST_LENGTH(l1) > 2)
    {
        int i;
        helpList = NIL;
        Operator *helpO1 = getHeadOfListP(l1);
        l1 = REMOVE_FROM_LIST_PTR(l1, helpO1);
        Operator *helpO2 = getHeadOfListP(l1);
        l1 = REMOVE_FROM_LIST_PTR(l1, helpO2);
        helpList = appendToTailOfList(helpList, helpO1);
        helpList = appendToTailOfList(helpList, helpO2);

	Operator *helpO = createOpExpr("AND", (List *) helpList);
        int length_l1 = LIST_LENGTH(l1);

        for(i=0; i<length_l1; i++)
        {
            helpList = NIL;
            helpList = appendToTailOfList(helpList, helpO);
            helpO = getHeadOfListP(l1);
            l1 = REMOVE_FROM_LIST_PTR(l1, helpO);
            helpList = appendToTailOfList(helpList, helpO);
            helpO =  createOpExpr("AND", (List *) helpList);
        }
        opNode1 = (Node *)helpO;
    }
    else
        opNode1 = (Node *) getHeadOfListP(l1);

    return opNode1;
}

List *
findAllNodes(Node *node, NodeTag type)
{
    FindNotesContext *context = NEW(FindNotesContext);
    List *result = NULL;

    context->type = type;
    context->result = &result;

    findAllNodesVisitor(node, context);

    FREE(context);

    return result;
}


static boolean
findAllNodesVisitor(Node *node, FindNotesContext *context)
{
    if (node == NULL)
       return TRUE;

    if ((node->type) == context->type)
    {
        *(context->result) = appendToTailOfList(*(context->result), node);
        return TRUE;
    }

    return visit(node, findAllNodesVisitor, context);
}
