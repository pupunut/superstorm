#include <stdio.h>
#include "Comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>

#include "Comm.h"
#include "CBackData.h"
#include "CPos.h"
#include "CPosMaster.h"
#include "CPosSlave.h"
#include "COperation.h"
#include "CStock.h"

using namespace std;


/* Flag set by ‘--verbose’. */
static int verbose_flag;
int main_comm(int argc, char *argv[], simulator_func *func)
{
    string db_path;
    int stock_sn, begin_date, end_date, policy;

    int c;

    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"stock",  required_argument, 0, 's'},
            {"begin",  required_argument, 0, 'b'},
            {"end",  required_argument, 0, 'e'},
            {"policy",    required_argument, 0, 'p'},
            {"db",    required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "s:b:e:p:d:",
                long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 'b':
                printf ("begin date: %s\n", optarg);
                begin_date = atoi(optarg);
                break;

            case 'd':
                printf ("database: %s\n", optarg);
                db_path.assign(optarg);
                break;

            case 'e':
                printf ("end date: %s\n", optarg);
                end_date = atoi(optarg);
                break;

            case 'p':
                printf ("policy is: %s\n", optarg);
                break;

            case 's':
                printf("stock is: %s\n", optarg);
                stock_sn = atoi(optarg);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }
    }

    /* Instead of reporting ‘--verbose’
       and ‘--brief’ as they are encountered,
       we report the final status resulting from them. */
    if (verbose_flag)
        puts ("verbose flag is set");

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    //OK , let's begin
    CBackData *db = new CBackData(ENUM_BACKDATA_DB, db_path);
    assert(db);

    (*func)(db, stock_sn, begin_date, end_date, policy);

    return 0;
}
