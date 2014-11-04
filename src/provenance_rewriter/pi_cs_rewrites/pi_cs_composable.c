/*-----------------------------------------------------------------------------
 *
 * pi_cs_composable.c
 *			  
 *		
 *		AUTHOR: lord_pretzel
 *
 *		
 *
 *-----------------------------------------------------------------------------
 */

#include "common.h"
#include "instrumentation/timing_instrumentation.h"
#include "log/logger.h"

#include "mem_manager/mem_mgr.h"
#include "model/expression/expression.h"
#include "model/query_operator/query_operator.h"
#include "model/query_operator/operator_property.h"
#include "model/list/list.h"
#include "provenance_rewriter/pi_cs_rewrites/pi_cs_composable.h"
#include "provenance_rewriter/prov_schema.h"
#include "provenance_rewriter/prov_utility.h"

// result tuple-id attribute and provenance duplicate counter attribute
#define RESULT_TID_ATTR "_result_tid"
#define PROV_DUPL_COUNT_ATTR "_prov_dup_count"

#define LOG_RESULT(mes,op) \
    do { \
    	INFO_LOG(mes,operatorToOverviewString((Node *) op)); \
    	DEBUG_LOG(mes,beatify(nodeToString((Node *) op))); \
    } while(0)

// data structures
static Node *asOf;
static RelCount *nameState;

// static methods
static boolean isTupleAtATimeSubtree(QueryOperator *op);

static QueryOperator *rewritePI_CSComposableOperator (QueryOperator *op);
static QueryOperator *rewritePI_CSComposableSelection (SelectionOperator *op);
static QueryOperator *rewritePI_CSComposableProjection (ProjectionOperator *op);
static QueryOperator *rewritePI_CSComposableJoin (JoinOperator *op);
static QueryOperator *rewritePI_CSComposableAggregation (AggregationOperator *op);
static QueryOperator *rewritePI_CSComposableSet (SetOperator *op);
static QueryOperator *rewritePI_CSComposableTableAccess(TableAccessOperator *op);
static QueryOperator *rewritePI_CSComposableConstRel(ConstRelOperator *op);
static QueryOperator *rewritePI_CSComposableDuplicateRemOp(DuplicateRemoval *op);
static QueryOperator *rewritePI_CSComposableOrderOp(OrderOperator *op);

static void addResultTIDAndProvDupAttrs (QueryOperator *op, boolean addToSchema);
static void addChildResultTIDAndProvDupAttrsToSchema (QueryOperator *op);
static List *getResultTidAndProvDupAttrsProjExprs(QueryOperator * op);
static void addNormalAttrsWithoutSpecialToSchema(QueryOperator *target, QueryOperator *source);
static List *getNormalAttrWithoutSpecial(QueryOperator *op);
static List *removeSpecialAttrsFromNormalProjectionExprs(List *projExpr);


static Node *replaceAttrWithCaseForProvDupRemoval (FunctionCall *f, Node *provDupAttrRef);


/*
 *
 */
QueryOperator *
rewritePI_CSComposable (ProvenanceComputation *op)
{
    QueryOperator *rewRoot;

    rewRoot = OP_LCHILD(op);
    rewRoot = rewritePI_CSComposableOperator(rewRoot);

    return (QueryOperator *) rewRoot;
}

/*
 * Figure out whether for a certain operator there will be at most one provenance tuple
 * per original result tuple. This can be used to use ROWNUM instead of window functions.
 * Store result as property to avoid recomputation.
 */
static boolean
isTupleAtATimeSubtree(QueryOperator *op)
{
    if (HAS_STRING_PROP(op, PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME))
        return GET_BOOL_STRING_PROP(op, PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME);

    switch(op->type)
    {
        case T_SelectionOperator:
        case T_ProjectionOperator:
        case T_JoinOperator:
        case T_TableAccessOperator:
            break;
        default:
        {
            SET_STRING_PROP(op, PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME, createConstBool(FALSE));
            return FALSE;
        }
    }

    FOREACH(QueryOperator,child,op->inputs)
        if (!isTupleAtATimeSubtree(child))
        {
            SET_STRING_PROP(op, PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME, createConstBool(FALSE));
            return FALSE;
        }


    SET_BOOL_STRING_PROP(op, PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME);
    return TRUE;
}

static QueryOperator *
rewritePI_CSComposableOperator (QueryOperator *op)
{
    switch(op->type)
    {
        case T_SelectionOperator:
            return rewritePI_CSComposableSelection((SelectionOperator *) op);
        case T_ProjectionOperator:
            return rewritePI_CSComposableProjection((ProjectionOperator *) op);
        case T_JoinOperator:
            return rewritePI_CSComposableJoin((JoinOperator *) op);
        case T_AggregationOperator:
            return rewritePI_CSComposableAggregation((AggregationOperator *) op);
        case T_Set:
            return rewritePI_CSComposableSet((SetOperator *) op);
        case T_TableAccessOperator:
            return rewritePI_CSComposableTableAccess((TableAccessOperator *) op);
        case T_ConstRelOperator:
            return rewritePI_CSComposableConstRel((ConstRelOperator *) op);
        case T_DuplicateRemoval:
            return rewritePI_CSComposableDuplicateRemOp((DuplicateRemoval *) op);
        case T_OrderOperator:
            return rewritePI_CSComposableOrderOp((OrderOperator *) op);
        default:
            FATAL_LOG("rewrite for %u not implemented", op->type);
            return NULL;
    }
}

static QueryOperator *
rewritePI_CSComposableSelection (SelectionOperator *op)
{
    ASSERT(OP_LCHILD(op));

    DEBUG_LOG("REWRITE-PICS-Composable - Selection");
    DEBUG_LOG("Operator tree \n%s", beatify(nodeToString(op)));

    // rewrite child first
    rewritePI_CSComposableOperator(OP_LCHILD(op));

    // adapt schema
    addProvenanceAttrsToSchema((QueryOperator *) op, OP_LCHILD(op));

    // add result TID and prov duplicate attributes
    addResultTIDAndProvDupAttrs((QueryOperator *) op, TRUE);

    if (isTupleAtATimeSubtree(OP_LCHILD(op)))
        SET_BOOL_STRING_PROP(op,PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME);

    LOG_RESULT("Selection - Rewritten Operator tree \n%s", op);
    return (QueryOperator *) op;
}

static QueryOperator *
rewritePI_CSComposableProjection (ProjectionOperator *op)
{
    ASSERT(OP_LCHILD(op));

    DEBUG_LOG("REWRITE-PICS-Composable - Projection");
    DEBUG_LOG("Operator tree \n%s", nodeToString(op));

    // rewrite child
    rewritePI_CSComposableOperator(OP_LCHILD(op));

    // add projection expressions for provenance attrs
    QueryOperator *child = OP_LCHILD(op);
    FOREACH_INT(a, child->provAttrs)
    {
        AttributeDef *att = getAttrDef(child,a);
        DEBUG_LOG("attr: %s", nodeToString(att));
        op->projExprs = appendToTailOfList(op->projExprs,
                createFullAttrReference(att->attrName, 0, a, 0, att->dataType));
    }

    // add projection expressions for result TID and prov dup attrs
    op->projExprs = appendToTailOfList(op->projExprs,
            createFullAttrReference(RESULT_TID_ATTR, 0,
                    INT_VALUE(GET_STRING_PROP(child,PROP_RESULT_TID_ATTR)), 0, DT_INT));

    op->projExprs = appendToTailOfList(op->projExprs,
            createFullAttrReference(PROV_DUPL_COUNT_ATTR, 0,
                    INT_VALUE(GET_STRING_PROP(child,PROP_PROV_DUP_ATTR)), 0, DT_INT));
    // adapt schema
    addProvenanceAttrsToSchema((QueryOperator *) op, OP_LCHILD(op));
    addResultTIDAndProvDupAttrs((QueryOperator *) op, TRUE);

    LOG_RESULT("Projection - Rewritten Operator tree \n%s", op);
    return (QueryOperator *) op;
}

static QueryOperator *
rewritePI_CSComposableJoin (JoinOperator *op)
{
    DEBUG_LOG("REWRITE-PICS-Composable - Join");
    WindowOperator *wOp = NULL;
    QueryOperator *lChild = OP_LCHILD(op);
    QueryOperator *rChild = OP_RCHILD(op);
    QueryOperator *prev = NULL;
    boolean noDupInput = isTupleAtATimeSubtree((QueryOperator *) op);
    boolean lChildNoDup = isTupleAtATimeSubtree(lChild);
    boolean rChildNoDup = isTupleAtATimeSubtree(rChild);

    // rewrite children
    lChild = rewritePI_CSComposableOperator(lChild);
    rChild = rewritePI_CSComposableOperator(rChild);

    // adapt schema for join op
    clearAttrsFromSchema((QueryOperator *) op);
    addNormalAttrsWithoutSpecialToSchema((QueryOperator *) op, lChild);
    addProvenanceAttrsToSchema((QueryOperator *) op, lChild);
    addChildResultTIDAndProvDupAttrsToSchema((QueryOperator *) op);

    addNormalAttrsWithoutSpecialToSchema((QueryOperator *) op, rChild);
    addProvenanceAttrsToSchema((QueryOperator *) op, rChild);
    addChildResultTIDAndProvDupAttrsToSchema((QueryOperator *) op);

    // add window functions for result TID and prov dup columns
    if (!lChildNoDup || !rChildNoDup)
    {
        List *orderBy = NIL;
        List *partitionBy = NIL;
        wOp = NULL;

        if (lChildNoDup)
        {
            AttributeReference *childResultTidAttr = (AttributeReference *)
                    getHeadOfListP(getResultTidAndProvDupAttrsProjExprs(lChild));
            orderBy = appendToTailOfList(orderBy, copyObject(childResultTidAttr));
            partitionBy = appendToTailOfList(partitionBy, copyObject(childResultTidAttr));
        }
        if (rChildNoDup)
        {
            AttributeReference *childResultTidAttr = (AttributeReference *)
                    getHeadOfListP(getResultTidAndProvDupAttrsProjExprs(rChild));
            childResultTidAttr->attrPosition += getNumAttrs(lChild);
            orderBy = appendToTailOfList(orderBy, copyObject(childResultTidAttr));
            partitionBy = appendToTailOfList(partitionBy, copyObject(childResultTidAttr));
        }

        // add window functions for result TID attr
        Node *tidFunc = (Node *) createFunctionCall(strdup("DENSE_RANK"), NIL);

        wOp = createWindowOp(tidFunc,
                NIL,
                orderBy,
                NULL,
                strdup(RESULT_TID_ATTR),
                (QueryOperator *) op,
                NIL
        );
        wOp->op.provAttrs = copyObject(op->op.provAttrs);

        // add window function for prov dup attr
        prev = (QueryOperator *) wOp;
        Node *provDupFunc = (Node *) createFunctionCall(strdup("ROW_NUMBER"), NIL);

        wOp = createWindowOp(provDupFunc,
                partitionBy,
                orderBy,
                NULL,
                strdup(PROV_DUPL_COUNT_ATTR),
                prev,
                NIL
        );
        wOp->op.provAttrs = copyObject(prev->provAttrs);
        addParent(prev, (QueryOperator *) wOp);
        SET_STRING_PROP(wOp, PROP_RESULT_TID_ATTR, createConstInt(LIST_LENGTH(wOp->op.schema->attrDefs) - 2));
        SET_STRING_PROP(wOp, PROP_PROV_DUP_ATTR, createConstInt(LIST_LENGTH(wOp->op.schema->attrDefs) - 1));

        LOG_RESULT("Added result TID and prov duplicate window ops:\n%s", wOp);
    }

    // add projection to put attributes into order on top of join op
    List *resultTidAndProvCount = NIL;
    List *projExpr;
    ProjectionOperator *proj;
    QueryOperator *projInput = (noDupInput) ?
            (QueryOperator *) op :
            (QueryOperator *) wOp;

    // get special attributes from window op or create projection expression for them
    if (!noDupInput)
        resultTidAndProvCount = getResultTidAndProvDupAttrsProjExprs((QueryOperator *) wOp);
    else
    {
        resultTidAndProvCount = LIST_MAKE(
                makeNode(RowNumExpr),
                createConstInt(1)
        );
    }
    projExpr = CONCAT_LISTS(
            removeSpecialAttrsFromNormalProjectionExprs(
                    getNormalAttrProjectionExprs((QueryOperator *) projInput)),
            getProvAttrProjectionExprs((QueryOperator *) projInput),
            resultTidAndProvCount);
    proj = createProjectionOp(projExpr, projInput, NIL, NIL);

    addNormalAttrsWithoutSpecialToSchema((QueryOperator *) proj, (QueryOperator *) projInput);
    addProvenanceAttrsToSchema((QueryOperator *) proj, (QueryOperator *) projInput);
    addChildResultTIDAndProvDupAttrsToSchema((QueryOperator *) proj);
    SET_STRING_PROP(proj, PROP_RESULT_TID_ATTR, createConstInt(LIST_LENGTH(projExpr) - 2));
    SET_STRING_PROP(proj, PROP_PROV_DUP_ATTR, createConstInt(LIST_LENGTH(projExpr) - 1));

    // switch projection with join in tree
    switchSubtrees((QueryOperator *) op, (QueryOperator *) proj);
    if (noDupInput)
        addParent((QueryOperator *) op, (QueryOperator *) proj);
    else
    {
        addParent((QueryOperator *) wOp, (QueryOperator *) proj);
        addParent((QueryOperator *) op, (QueryOperator *) prev);
    }

    LOG_RESULT("Join - Rewritten Operator tree \n%s", proj);
    return (QueryOperator *) proj;
}

static QueryOperator *
rewritePI_CSComposableAggregation (AggregationOperator *op)
{
    boolean groupBy = (op->groupBy != NIL);
    WindowOperator *curWindow = NULL;
    QueryOperator *firstChild;
    QueryOperator *curChild;
    ProjectionOperator *proj;
    Node *provDupAttrRef;
    boolean noDupInput;
    List *projExprs = NIL;
    List *finalAttrs = NIL;
    List *orderBy = NIL;
    List *partitionBy = NIL;
    List *groupByExprs = copyObject(op->groupBy);
    List *aggNames = aggOpGetAggAttrNames(op);
    int pos;

    DEBUG_LOG("REWRITE-PICS-Composable - Aggregation");
    DEBUG_LOG("Operator tree \n%s", beatify(nodeToString(op)));

    // rewrite child
    curChild = rewritePI_CSComposableOperator(OP_LCHILD(op));
    firstChild = curChild;
    removeParentFromOps(singleton(firstChild), (QueryOperator *) op);
    noDupInput = isTupleAtATimeSubtree(curChild);

    // create partition clause and order by clauses
    if (groupBy)
    {
        FOREACH(AttributeReference, a, op->groupBy)
            partitionBy = appendToTailOfList(partitionBy,
                                copyObject(a));

        orderBy = copyObject(partitionBy);
    }

    // get input prov dup attribute
    if (!noDupInput)
    {
        provDupAttrRef = (Node *) createFullAttrReference(PROV_DUPL_COUNT_ATTR,
                0,
                INT_VALUE(GET_STRING_PROP(curChild, PROP_PROV_DUP_ATTR)),
                INVALID_ATTR,
                DT_INT);
    }
    else
        provDupAttrRef = NULL;

    // create window op for each aggregation
    pos = 0;
    FOREACH(Node,agg,op->aggrs)
    {
        Node *aggForWindow = replaceAttrWithCaseForProvDupRemoval(
                copyObject(agg), provDupAttrRef);
        char *attrName = getNthOfListP(aggNames, pos);

        curWindow = createWindowOp(aggForWindow,
                partitionBy,
                NIL,
                NULL,
                attrName,
                curChild,
                NIL
                );
        curWindow->op.provAttrs = copyObject(curChild->provAttrs);
        addParent(curChild, (QueryOperator *) curWindow);

        curChild = (QueryOperator *) curWindow;
        pos++;
        DEBUG_LOG("Translated aggregation function <%s> into window op:\n%s",
                beatify(nodeToString(agg)), operatorToOverviewString((Node *) curWindow));
    }

    // add result TID attr and prov dup attr, if group by then use window function, otherwise use projection
    if (groupBy)
    {
        // add window functions for result TID attr
        Node *tidFunc = (Node *) createFunctionCall(strdup("DENSE_RANK"), NIL);

        curWindow = createWindowOp(tidFunc,
                NIL,
                orderBy,
                NULL,
                strdup(RESULT_TID_ATTR),
                curChild,
                NIL
        );
        curWindow->op.provAttrs = copyObject(curChild->provAttrs);
        addParent(curChild, (QueryOperator *) curWindow);
        curChild = (QueryOperator *) curWindow;

        // add window function for prov dup attr
        Node *provDupFunc = (Node *) createFunctionCall(strdup("ROW_NUMBER"), NIL);

        curWindow = createWindowOp(provDupFunc,
                partitionBy,
                orderBy,
                NULL,
                strdup(PROV_DUPL_COUNT_ATTR),
                curChild,
                NIL
        );
        curWindow->op.provAttrs = copyObject(curChild->provAttrs);
        addParent(curChild, (QueryOperator *) curWindow);
        curChild = (QueryOperator *) curWindow;

        DEBUG_LOG("Added result TID and prov duplicate window ops:\n%s",
                       operatorToOverviewString((Node *) curWindow));
    }

    // create final projection: normal attributes + provenance attribute + result TID and prov dup attr
    List *normalAttrs = getNormalAttrProjectionExprs((QueryOperator *) curWindow);
    List *provAttrs = getProvAttrProjectionExprs((QueryOperator *) curWindow);
    List *aggAttrNames = aggOpGetAggAttrNames(op);
    List *groupByAttrNames = groupBy ? aggOpGetGroupByAttrNames(op) : NIL;
    List *provAttrNames = getOpProvenanceAttrNames((QueryOperator *) curWindow);

    // no group by, add result TID and prov dup attributes to projection
    if (!groupBy)
    {
        normalAttrs = sublist(normalAttrs,
                LIST_LENGTH(normalAttrs) - LIST_LENGTH(op->aggrs),
                LIST_LENGTH(normalAttrs));
        projExprs = CONCAT_LISTS(normalAttrs, provAttrs,
                LIST_MAKE(createConstInt(1),
                        makeNode(RowNumExpr)));

        finalAttrs = CONCAT_LISTS(aggAttrNames,
                            provAttrNames,
                            LIST_MAKE(strdup(RESULT_TID_ATTR),strdup(PROV_DUPL_COUNT_ATTR)));
    }
    // else move result TID and prov dup attribute to end of list
    else
    {
//        List *tidAndDupAttrs = sublist(copyList(normalAttrs),
//                LIST_LENGTH(normalAttrs) - 3,
//                LIST_LENGTH(normalAttrs));
        normalAttrs = sublist(normalAttrs,
                LIST_LENGTH(normalAttrs) - LIST_LENGTH(op->aggrs) - 2,
                LIST_LENGTH(normalAttrs) - 2);

        projExprs = CONCAT_LISTS(normalAttrs, groupByExprs, provAttrs,
                LIST_MAKE(createFullAttrReference(
                            strdup(RESULT_TID_ATTR),
                            0,
                            getNumAttrs((QueryOperator *) curWindow) - 2,
                            INVALID_ATTR,
                            DT_INT),
                        createFullAttrReference(
                            strdup(PROV_DUPL_COUNT_ATTR),
                            0,
                            getNumAttrs((QueryOperator *) curWindow) - 1,
                            INVALID_ATTR,
                            DT_INT)));

        finalAttrs = CONCAT_LISTS(aggAttrNames,
                            groupByAttrNames,
                            provAttrNames,
                            LIST_MAKE(strdup(RESULT_TID_ATTR),strdup(PROV_DUPL_COUNT_ATTR)));
    }

    proj = createProjectionOp(projExprs, curChild, NIL, finalAttrs);
    CREATE_INT_SEQ(proj->op.provAttrs,
            LIST_LENGTH(op->aggrs) + LIST_LENGTH(op->groupBy),
            getNumAttrs((QueryOperator *) proj) - 3, 1);
    addParent((QueryOperator *) curWindow, (QueryOperator *) proj);

    SET_STRING_PROP(proj, PROP_RESULT_TID_ATTR, createConstInt(LIST_LENGTH(finalAttrs) - 2));
    SET_STRING_PROP(proj, PROP_PROV_DUP_ATTR, createConstInt(LIST_LENGTH(finalAttrs) - 1));

    // switch aggregation and rewritten
    switchSubtrees((QueryOperator *) op, (QueryOperator *) proj);
    DEBUG_LOG("projection is:\n%s", operatorToOverviewString((Node *) proj));

    // return projection
    LOG_RESULT("Aggregation - Rewritten Operator tree \n%s", op);
    return (QueryOperator *) proj;
}

static Node *
replaceAttrWithCaseForProvDupRemoval (FunctionCall *f, Node *provDupAttrRef)
{
    if (provDupAttrRef == NULL)
        return (Node *) f;

    FOREACH_LC(lc,f->args)
    {
        Node *arg = LC_P_VAL(lc);
        LC_P_VAL(lc) = createCaseExpr(
                NULL,
                singleton(createCaseWhen((Node *) createOpExpr("=",
                        LIST_MAKE(createConstInt(1), copyObject(provDupAttrRef))),
                        arg)),
                (Node *) createNullConst(DT_INT)
                );
    }

    DEBUG_LOG("modified agg function call: <%s>", beatify(nodeToString(f)));
    return (Node *) f;
}

static QueryOperator *
rewritePI_CSComposableSet (SetOperator *op)
{
    FATAL_LOG("not implemented yet");
    return NULL;
}

static QueryOperator *
rewritePI_CSComposableTableAccess(TableAccessOperator *op)
{
//    List *tableAttr;
    List *provAttr = NIL;
    List *projExpr = NIL;
    char *newAttrName;

    int relAccessCount = getRelNameCount(&nameState, op->tableName);
    int cnt = 0;

    DEBUG_LOG("REWRITE-PICS-Composable - Table Access <%s> <%u>", op->tableName, relAccessCount);

    // copy any as of clause if there
    if (asOf)
        op->asOf = copyObject(asOf);

    // Get the povenance name for each attribute
    FOREACH(AttributeDef, attr, op->op.schema->attrDefs)
    {
        provAttr = appendToTailOfList(provAttr, strdup(attr->attrName));
        projExpr = appendToTailOfList(projExpr, createFullAttrReference(attr->attrName, 0, cnt, 0, attr->dataType));
        cnt++;
    }

    cnt = 0;
    FOREACH(AttributeDef, attr, op->op.schema->attrDefs)
    {
        newAttrName = getProvenanceAttrName(op->tableName, attr->attrName, relAccessCount);
        provAttr = appendToTailOfList(provAttr, newAttrName);
        projExpr = appendToTailOfList(projExpr, createFullAttrReference(attr->attrName, 0, cnt, 0, attr->dataType));
        cnt++;
    }

    // result tuple ID attribute
    newAttrName = strdup(RESULT_TID_ATTR);
    provAttr = appendToTailOfList(provAttr, newAttrName);
    projExpr = appendToTailOfList(projExpr, makeNode(RowNumExpr));

    // provenance duplicate attribute
    newAttrName = strdup(PROV_DUPL_COUNT_ATTR);
    provAttr = appendToTailOfList(provAttr, newAttrName);
    projExpr = appendToTailOfList(projExpr, createConstInt(1));

    List *newProvPosList = NIL;
    CREATE_INT_SEQ(newProvPosList, cnt, (cnt * 2) - 1, 1);

    DEBUG_LOG("rewrite table access, \n\nattrs <%s> and \n\nprojExprs <%s> and \n\nprovAttrs <%s>",
            stringListToString(provAttr),
            nodeToString(projExpr),
            nodeToString(newProvPosList));

    // Create a new projection operator with these new attributes
    ProjectionOperator *newpo = createProjectionOp(projExpr, NULL, NIL, provAttr);
    newpo->op.provAttrs = newProvPosList;

    // set properties to mark result TID and prov duplicate attrs
    SET_STRING_PROP(newpo, PROP_RESULT_TID_ATTR, createConstInt(cnt * 2));
    SET_STRING_PROP(newpo, PROP_PROV_DUP_ATTR, createConstInt((cnt * 2) + 1));

    // Switch the subtree with this newly created projection operator.
    switchSubtrees((QueryOperator *) op, (QueryOperator *) newpo);

    // Add child to the newly created projections operator,
    addChildOperator((QueryOperator *) newpo, (QueryOperator *) op);
    SET_BOOL_STRING_PROP(newpo,PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME);

    LOG_RESULT("Table Access - Rewritten Operator tree \n%s", newpo);
    return (QueryOperator *) newpo;
}

static QueryOperator *
rewritePI_CSComposableConstRel(ConstRelOperator *op)
{
    FATAL_LOG("not implemented yet");
    return NULL;
}

static QueryOperator *
rewritePI_CSComposableDuplicateRemOp(DuplicateRemoval *op)
{
    FATAL_LOG("not implemented yet");
    return NULL;
}

static void
addResultTIDAndProvDupAttrs (QueryOperator *op, boolean addToSchema)
{
    int numAttrs = getNumAttrs(op);
    QueryOperator *child = OP_LCHILD(op);

    if (addToSchema)
    {
        op->schema->attrDefs = appendToTailOfList(op->schema->attrDefs,
                createAttributeDef(strdup(RESULT_TID_ATTR), DT_INT));
        op->schema->attrDefs = appendToTailOfList(op->schema->attrDefs,
                    createAttributeDef(strdup(PROV_DUPL_COUNT_ATTR), DT_INT));

        // set properties to mark result TID and prov duplicate attrs
        SET_STRING_PROP(op, PROP_RESULT_TID_ATTR, createConstInt(numAttrs));
        SET_STRING_PROP(op, PROP_PROV_DUP_ATTR, createConstInt(numAttrs + 1));
    }
    else
    {
        SET_STRING_PROP(op, PROP_RESULT_TID_ATTR,
                copyObject(GET_STRING_PROP(child, PROP_RESULT_TID_ATTR)));
        SET_STRING_PROP(op, PROP_PROV_DUP_ATTR,
                copyObject(GET_STRING_PROP(child, PROP_PROV_DUP_ATTR)));
    }
}

static void
addChildResultTIDAndProvDupAttrsToSchema (QueryOperator *op)
{
    op->schema->attrDefs = appendToTailOfList(op->schema->attrDefs,
            createAttributeDef(strdup(RESULT_TID_ATTR), DT_INT));
    op->schema->attrDefs = appendToTailOfList(op->schema->attrDefs,
            createAttributeDef(strdup(PROV_DUPL_COUNT_ATTR), DT_INT));
}

static void
addNormalAttrsWithoutSpecialToSchema(QueryOperator *target, QueryOperator *source)
{
    List *newAttrs = (List *) copyObject(getNormalAttrWithoutSpecial(source));
    target->schema->attrDefs = concatTwoLists(target->schema->attrDefs, newAttrs);
}

static List *
getNormalAttrWithoutSpecial(QueryOperator *op)
{
    List *norm = getNormalAttrs(op);
    List *result = NIL;

    FOREACH(AttributeDef,a,norm)
    {
        if (strcmp(a->attrName, RESULT_TID_ATTR) != 0
                && strcmp(a->attrName, PROV_DUPL_COUNT_ATTR) != 0)
            result = appendToTailOfList(result, a);
    }

    return result;
}


static List *
getResultTidAndProvDupAttrsProjExprs(QueryOperator *op)
{
    List *result = NIL;

    result = LIST_MAKE(
            createFullAttrReference(RESULT_TID_ATTR,
                    0,
                    INT_VALUE(GET_STRING_PROP(op, PROP_RESULT_TID_ATTR)),
                    INVALID_ATTR,
                    DT_INT),
            createFullAttrReference(PROV_DUPL_COUNT_ATTR,
                    0,
                    INT_VALUE(GET_STRING_PROP(op, PROP_PROV_DUP_ATTR)),
                    INVALID_ATTR,
                    DT_INT)
    );

    return result;
}

static List *
removeSpecialAttrsFromNormalProjectionExprs(List *projExpr)
{
    List *result = NIL;

    FOREACH(AttributeReference,a,projExpr)
    {
        if (strcmp(a->name, RESULT_TID_ATTR) != 0
            && strcmp(a->name, PROV_DUPL_COUNT_ATTR) != 0)
            result = appendToTailOfList(result, a);
    }

    return result;
}


static QueryOperator *
rewritePI_CSComposableOrderOp(OrderOperator *op)
{
    QueryOperator *child = OP_LCHILD(op);

    // rewrite child
    rewritePI_CSComposableOperator(child);

    // adapt provenance attr list and schema
    addProvenanceAttrsToSchema((QueryOperator *) op, child);
    addResultTIDAndProvDupAttrs((QueryOperator *) op, TRUE);

    LOG_RESULT("Order Operator - Rewritten Operator tree \n%s", op);
    return (QueryOperator *) op;
}
