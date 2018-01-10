/*-----------------------------------------------------------------------------
 *
 * exe_run_query.c
 *			  
 *		
 *		AUTHOR: lord_pretzel
 *
 *		
 *
 *-----------------------------------------------------------------------------
 */

#include "common.h"
#include "log/logger.h"
#include "model/relation/relation.h"
#include "metadata_lookup/metadata_lookup.h"
#include "execution/exe_run_query.h"
#include "utility/string_utils.h"
//#include "analysis_and_translate/translator_dl.h"
#include "provenance_rewriter/game_provenance/gp_bottom_up_program.h"
#include "configuration/option.h"

void
exeRunQuery (void *code)
{
    Relation *res;
    int i = 0;
    int l = 0;
    char *adaptedQuery;
    int *colSizes;
    int numCol;
//    int numRows;
    int totalSize;

    // list of statement
    List *stmtList = splitString(code, ";");

//    // remove semicolon
//    adaptedQuery = replaceSubstr(code, ";", ""); //TODO not safe if ; in strings

    for(int s = 0; s < LIST_LENGTH(stmtList)-1; s++)
    {
    	totalSize = 0;
//        res = executeQuery((char *) adaptedQuery);
    	adaptedQuery = (char *) getNthOfListP(stmtList,s);

    	// execute query
        INFO_LOG("run query:\n%s", (char *) adaptedQuery);

    	res = executeQuery((char *) adaptedQuery);

        // output table name for the input database
    	if (getBoolOption(OPTION_INPUTDB))
    	{
//    		FOREACH_HASH(char,r,taRel)
    		char *r = (char *) MAP_GET_STRING(edbRels,itoa(s+1));
   			printf("%s", r);
    		printf("\n");
    	}

        numCol = LIST_LENGTH(res->schema);
    //    numRows = LIST_LENGTH(res->tuples);
        colSizes = MALLOC(numCol * sizeof(int));

        // determine column sizes
        i = 0;
        FOREACH(char,a,res->schema)
        {
            colSizes[i++] = strlen(a) + 2;
        }

        FOREACH(List,t,res->tuples)
        {
            i = 0;
            FOREACH(char,a,t)
            {
                int len = a ? strlen(a) : 4;
                colSizes[i] = colSizes[i] < len + 2 ? len + 2 : colSizes[i];
                i++;
            }
        }

        for (i = 0; i < numCol; i++)
            totalSize += colSizes[i] + 1;

        // output columns
        i = 0;
        FOREACH(char,a,res->schema)
        {
            printf(" %s", a);
            for(int j = strlen(a) + 1; j < colSizes[i]; j++)
                printf(" ");
            printf("|");
            i++;
        }
        printf("\n");
        for (int j = 0; j < totalSize; j++)
            printf("-");
        printf("\n");

        // output results
        FOREACH(List,t,res->tuples)
        {
            i = 0;
            FOREACH(char,a,t)
            {
                char *out = a ? a : "NULL";
                out = strtrim(out);

//                // keep only string from the result
//                //TODO: check whether it can be associated with the function 'strtrim' in string_utils.c
//                StringInfo strimOut = makeStringInfo();
//                while(*out != '\0')
//				{
//					if (*out != '\t' && *out != '\r' && *out != '\n')
//						appendStringInfoChar(strimOut, *out);
//					out++;
//				}
//                out = strimOut->data;

                printf(" %s", out);
                for(int j = strlen(out) + 1; j < colSizes[i]; j++)
                    printf(" ");
                printf("|");
                i++;
            }
            printf("\n");

            if ((l++ % 1000) == 0)
                fflush(stdout);
        }
    }
}
