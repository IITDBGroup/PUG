#ifndef OPTION_H
#define OPTION_H

#include "common.h"
#include "model/node/nodetype.h"
#include "model/list/list.h"

typedef enum OptionType {
    OPTION_BOOL,
    OPTION_STRING,
    OPTION_INT,
    OPTION_FLOAT
} OptionType;

/* define rewrite methods */
#define OPTION_TIMING "timing"
#define OPTION_MEMMEASURE "memdebug"
#define OPTION_GRAPHVIZ "graphviz"
#define OPTION_AGGRESSIVE_MODEL_CHECKING "aggressive_model_checking"

#define OPTION_UPDATE_ONLY_USE_CONDS "only_updated_use_conditions"
#define OPTION_UPDATE_ONLY_USE_HISTORY_JOIN "only_updated_use_history"
#define OPTION_TREEIFY_OPERATOR_MODEL "treefiy_prov_rewrite_input"
#define OPTION_PI_CS_USE_COMPOSABLE "pi_cs_use_composable"
#define OPTION_OPTIMIZE_OPERATOR_MODEL "optimize_operator_model"
#define OPTION_TRANSLATE_UPDATE_WITH_CASE "translate_update_with_case"
//#define OPTION_

/* define optimization options */
#define OPTIMIZATION_SELECTION_PUSHING "optimization.push_selections"
#define OPTIMIZATION_MERGE_OPERATORS "optimization.merge_operators"
#define OPTIMIZATION_FACTOR_ATTR_IN_PROJ_EXPR "optimization.factor_proj_attr_in_expr"
#define OPTIMIZATION_MATERIALIZE_MERGE_UNSAFE_PROJ "optimization.materialize_merge_unsafe_proj"
//#define OPTIMIZATION_ "optimization_"

/* define model checking options */
#define CHECK_OM_UNIQUE_ATTR_NAMES "check.unique_attrs"
#define CHECK_OM_PARENT_CHILD_LINKS "check.parent_child_links"
#define CHECK_OM_SCHEMA_CONSISTENCY "check.schema_consistency"
#define CHECK_OM_ATTR_REF "check.attr_ref_consistency"

// declare option fields
// show help only
extern boolean opt_show_help;

// connection options
extern char *connection_host;
extern char *connection_db;
extern char *connection_user;
extern char *connection_passwd;
extern int connection_port;

// logging options
extern int logLevel;
extern boolean logActive;

// input options
extern char *sql;

// database backend
extern char *backend;
extern char *plugin_metadata;
extern char *plugin_parser;
extern char *plugin_sqlcodegen;
extern char *plugin_executor;

// instrumentation options
extern boolean opt_timing;
extern boolean opt_memmeasure;

// rewrite options
extern boolean opt_aggressive_model_checking;
extern boolean opt_update_only_conditions;
extern boolean opt_treeify_opterator_model;
extern boolean opt_only_updated_use_history;
extern boolean opt_pi_cs_composable;
extern boolean opt_optimize_operator_model;
extern boolean opt_translate_update_with_case;

// optimization options
extern boolean opt_optimization_push_selections;
extern boolean opt_optimization_merge_ops;
extern boolean opt_optimization_factor_attrs;
extern boolean opt_materialize_unsafe_proj;


// new option interface
extern char *getStringOption (char *name);
extern int getIntOption (char *name);
extern boolean getBoolOption (char *name);
extern double getFloatOption (char *name);

extern void setStringOption (char *name, char *value);
extern void setIntOption(char *name, int value);
extern void setBoolOption(char *name, boolean value);
extern void setFloatOption(char *name, double value);

extern boolean hasOption(char *name);
extern boolean hasCommandOption(char *name);
extern char *commandOptionGetOption(char *name);
extern OptionType getOptionType(char *name);
extern boolean optionSet(char *name);

extern void printOptionsHelp(FILE *stream, char *progName, char *description,
        boolean showValues);
extern void printCurrentOptions(FILE *stream);

extern void mallocOptions();
extern void freeOptions();
extern boolean isRewriteOptionActivated(char *name);

#endif
