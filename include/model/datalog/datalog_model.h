/*-----------------------------------------------------------------------------
 *
 * datalog_model.h
 *		
 *
 *		AUTHOR: lord_pretzel
 *
 *-----------------------------------------------------------------------------
 */

#ifndef INCLUDE_MODEL_DATALOG_DATALOG_MODEL_H_
#define INCLUDE_MODEL_DATALOG_DATALOG_MODEL_H_

#include "model/node/nodetype.h"
#include "model/expression/expression.h"
#include "model/set/hashmap.h"

// data types
typedef struct DLNode
{
    NodeTag type;
    HashMap *properties;
} DLNode;

typedef struct DLAtom
{
    DLNode n;
    char *rel;
    List *args;     // vars or consts
    boolean negated;
} DLAtom;

typedef struct DLVar
{
    DLNode n;
    char *name;
    DataType dt;
} DLVar;

typedef struct DLComparison
{
    DLNode n;
    Operator *opExpr;
} DLComparison;

typedef struct DLRule
{
    DLNode n;
    DLAtom *head;
    List *body;     // list of atoms and comparisons
} DLRule;

typedef struct DLProgram
{
    DLNode n;
    List *rules;
    List *facts;
    char *ans;
} DLProgram;

// provenance
#define IS_GP_PROV(prog) (DL_HAS_PROP(prog,DL_PROV_WHY) \
    || DL_HAS_PROP(prog,DL_PROV_WHYNOT) \
    || DL_HAS_PROP(prog,DL_PROV_FULL_GP))

// convenience functions
extern DLAtom *createDLAtom (char *rel, List *args, boolean negated);
extern DLVar *createDLVar (char *vName, DataType vType);
extern boolean isConstAtom (DLAtom *a);
extern DLRule *createDLRule (DLAtom *head, List *body);
extern DLProgram *createDLProgram (List *dlRules, List *facts, char *ans);
extern DLComparison *createDLComparison (char *op, Node *lArg, Node *rArg);

// get information about DL program elements
extern char *getHeadPredName(DLRule *r);
extern List *getRuleVars (DLRule *r);
extern List *getBodyVars (DLRule *r);
extern List *getBodyPredVars (DLRule *r);
extern List *getHeadVars (DLRule *r);

//extern List *getAtomVars (DLAtom *r);

extern List *getVarNames (List *vars);

// unification
extern DLRule *unifiyRule (DLRule *r, List *headBinds);

// properties
extern Node *getDLProp(DLNode *n, char *key);
extern void setDLProp(DLNode *n, char *key, Node *value);

#define DL_HAS_PROP(node,key) (getDLProp((DLNode *) node,key) != NULL)
#define DL_SET_BOOL_PROP(node,key) setDLProp((DLNode *) node, key, (Node *) createConstBool(TRUE));

// property keys
#define DL_IS_IDB_REL "IS_IDB_REL"

#define DL_PROV_WHY "WHY_PROV"
#define DL_PROV_WHYNOT "WHYNOT_PROV"
#define DL_PROV_FULL_GP "FULL_GP_PROV"

#define DL_MAP_RELNAME_TO_RULES "REL_TO_RULES"

#define DL_WON "WON"
#define DL_LOST "LOST"

#endif /* INCLUDE_MODEL_DATALOG_DATALOG_MODEL_H_ */
