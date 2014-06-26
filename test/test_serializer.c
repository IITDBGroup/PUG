/*
 * test_translate.c
 *
 *      Author: lordpretzel
 */

#include "common.h"

#include "mem_manager/mem_mgr.h"
#include "log/logger.h"
#include "configuration/option.h"
#include "model/list/list.h"
#include "model/node/nodetype.h"
#include "parser/parser.h"
#include "model/query_operator/query_operator.h"
#include "analysis_and_translate/translator.h"
#include "sql_serializer/sql_serializer.h"
#include "rewriter.h"

/* if OCI is not available then add dummy versions */
#if HAVE_A_BACKEND

int
main (int argc, char* argv[])
{
    Node *result;
    Node *qoModel;
    char *sql;

    READ_OPTIONS_AND_INIT("testtranslate", "Run all stages on input except provenance rewrite and output rewritten SQL code.");

    // read from terminal
    if (getStringOption("input.sql") == NULL)
    {
        result = parseStream(stdin);

        DEBUG_LOG("Address of returned node is <%p>", result);
        ERROR_LOG("PARSE RESULT FROM STREAM IS <%s>", beatify(nodeToString(result)));
    }
    // parse input string
    else
    {
        result = parseFromString(getStringOption("input.sql"));

        DEBUG_LOG("Address of returned node is <%p>", result);
        ERROR_LOG("PARSE RESULT FROM STRING IS:\n%s", beatify(nodeToString(result)));
    }

    qoModel = translateParse(result);
    INFO_LOG("TRANSLATION RESULT FROM STRING IS:\n%s", beatify(nodeToString(qoModel)));
    ERROR_LOG("SIMPLIFIED OPERATOR TREE:\n%s", operatorToOverviewString(qoModel));

    sql = serializeOperatorModel(qoModel);
    ERROR_LOG("SERIALIZED SQL:\n%s", sql);

    return shutdownApplication();
}



/* if OCI or OCILIB are not avaible replace with dummy test */
#else

int main()
{
    return EXIT_SUCCESS;
}

#endif


