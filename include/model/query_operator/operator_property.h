/*-----------------------------------------------------------------------------
 *
 * operator_property.h
 *		
 *
 *		AUTHOR: lord_pretzel
 *
 *-----------------------------------------------------------------------------
 */

#ifndef OPERATOR_PROPERTY_H_
#define OPERATOR_PROPERTY_H_

/* define keys for properties used to annotate operators */
/* general operator properties */
#define PROP_MATERIALIZE "MATERIALIZE"                      // hint to materialize output of this operator

// provenance
#define PROP_SHOW_INTERMEDIATE_PROV "SHOW_INTERMEDIATE_PROV" // show provenance for this intermediate subquery result
#define PROP_USE_PROVENANCE "USE_PROVENANCE"                // duplicate user provided attributes as provenance
#define PROP_HAS_PROVENANCE "HAS_PROVENANCE"                // indicates the subquery already has associated provenance
#define PROP_USER_PROV_ATTRS "USER_PROV_ATTRS"              // list of user selected provenance attribtues
#define PROP_PROV_REL_NAME "PROVENANCE_REL_NAME"            // in provenance attributes refer to subquery as this name
#define PROP_PROV_ADD_REL_NAME "PROVENANCE_ADD_REL_NAME"            // in provenance attributes that are added refer to subquery as this name
#define PROP_ADD_PROVENANCE "ADD_PROVENANCE"                // add the following attribtues to as provenance attributes (but still rewrite and add normal provenance attrs too)
#define PROP_PROV_IGNORE_ATTRS "IGNORE_PROV_ATTRS"          // these attributes should be ignored during rewrite
#define PROP_TRANSLATE_AS "TRANSLATE AS"

// reenactment
#define PROP_REENACT_SCHEMA_INFO "REENACT_SCHEMA"           // store additional table schemas to support reenactment of DDL commands

// provenance PI-CS composable
#define PROP_RESULT_TID_ATTR "RESULT_TID_ATTR"              // result tid attribute for PI-CS composable
#define PROP_PROV_DUP_ATTR "PROV_DUP_ATTR"                  // provenance duplicate counter attribute for PI-CS composable
#define PROP_PROVENANCE_OPERATOR_TUPLE_AT_A_TIME "PROVENANCE_OPERATOR_TUPLE_AT_A_TIME" // is the operator not sensitive to duplication

// provenance of transaction
#define PROP_PROV_IS_UPDATE_ROOT "UPDATE_ROOT"              // root of a translated update reenactment query

/* Operator type specific properties */
/* provenance computation specific properties */
#define PROP_PC_TABLE "TRACK_TABLE"                         // updated table to trace proveance of transaction for
#define PROP_PC_UPDATE_COND "UPDATE_CONDS"                  // conditions of updates in transaction
#define PROP_PC_ONLY_UPDATED "ONLY_UPDATED"                 // show only provenance of updated rows in transaction
#define PROP_PC_SHOW_INTERMEDIATE "SHOW ALL INTERMEDIATE"   // show provenance of all intermediate statements in transaction
#define PROP_PC_TRANS_XID "TRANSACTION_XID"                 // stores transaction XID
#define PROP_PC_TUPLE_VERSIONS "TUPLE_VERSIONS"             // use rowid + scn pairs as provenance
#define PROP_PC_STATEMENT_ANNOTATIONS "STATEMENT_ANNOTATIONS" // statement annotations
#define PROP_PC_REENACT_METADATA "REENACT_METADATA"         // store table information for reenacting DDL commands
#define PROP_PC_SEMIRING_COMBINER "SEMIRING_COMBINER"       // use combiner in provenance computation
#define PROP_PC_SC_AGGR_OPT "SEMIRING_COMBINER_AGGR_OPT" //use aggregation optimization in semiring combiner

/* table access properties */
#define PROP_TABLE_IS_UPDATED "UPDATED_TABLE"               // is table access for the updated table in an DML translation
#define PROP_IS_READ_COMMITTED "TABLE_READ_COMMITTED"      // is table access for updated table in a READ COMMITTED transaction
#define PROP_USE_HISTORY_JOIN "USE_HISTORY_JOIN"            // get committed rows from history and join with table at transaction start to pre-filter updated rows
#define PROP_TABLE_USE_ROWID_VERSION "USE_ROW_ID_VERSION"   // use rowid and version as provenance

/* projection properties */
#define PROP_MERGE_ATTR_REF_CNTS "MERGE SAFE ATTRIBUTE COUNTS"                        // safe to merge this projection with its child
#define PROP_PROJ_PROV_ATTR_DUP "PROJECTION WITH PROVENANCE ATTRIBUTE DUPLICATION"    // needed by projection pull-up
#define PROP_PROJ_PROV_ATTR_DUP_PULLUP "PROJECTION WITH PROVENANCE ATTRIBUTE DUPLICATION PULL-UP"  //no need to pull-up if this op already pull-up

/* properties used to store list of set which used in selection move around */
#define PROP_STORE_LIST_SET_SELECTION_MOVE_AROUND "STORE LIST SET FOR SELECTION MOVE AROUND"
#define PROP_OPT_SELECTION_MOVE_AROUND_DONE "HAVE DONE SELECTION MOVE AROUND"
#define PROP_OPT_UNNECESSARY_COLS_REMOVED_DONE "HAVE REMOVED UNNECESSARY"
#define PROP_STORE_MERGE_DONE "MERGE IS DONE"
#define PROP_STORE_REMOVE_RED_PROJ_DONE "REMOVED RED PROJ"
#define PROP_STORE_REMOVE_RED_DUP_BY_KEY_DONE "REMOVED DUP BY KEY"
#define PROP_OPT_REMOVE_RED_DUP_BY_SET_DONE "REMOVED DUP BY SET"
#define PROP_OPT_REMOVE_RED_WIN_DONW "REMOVED WIN OP"
//#define PROP_MERGE_ATTR_REF_CNTS "MERGE SAFE ATTRIBUTE COUNTS"                        // safe to merge this projection with its child?

/* properties to store characteristics of operators for heuristic optimization */
#define PROP_STORE_LIST_KEY "STORE KEY LIST FOR REMOVE REDUNDANT DUPLICATE"
#define PROP_STORE_LIST_KEY_DONE "HAVE COMPUTED KEYS"
#define PROP_STORE_BOOL_SET "STORE SET PROPERTY FOR REMOVE REDUNDANT DUPLICATE"
#define PROP_STORE_BOOL_SET_ALL_PARENTS_DONE "SET PROPERTY ALL PARENT OPS HAVE BEEN PROCESSED"
#define PROP_STORE_BOOL_SET_DONE "SET PROPERTY COMPUTED"
#define PROP_STORE_SET_ICOLS "STORE ICOLS PROPERTY FOR REMOVE REDUNDANT DUPLICATE"
#define PROP_STORE_SET_ICOLS_DONE "DONE STORE ICOLS PROPERTY FOR REMOVE REDUNDANT DUPLICATE"
#define PROP_STORE_LIST_SCHEMA_NAMES "STORE SCHEMA NAMES PROPERTY FOR REMOVE REDUNDANT DUPLICATE"
#define PROP_STORE_SET_EC "STORE EC PROPERTY"
#define PROP_STORE_SET_EC_DONE_BU "STORE EC PROPERTY - DONE BOTTOM UP"
#define PROP_STORE_SET_EC_DONE_TD "STORE EC PROPERTY - DONE TOP DOWN"
#define PROP_STORE_DUP_MARK "STORE DUP PROPERTY"  //pull up dup op, avoid loop the same dup op two times

#endif /* OPERATOR_PROPERTY_H_ */
