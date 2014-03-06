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

void run_t0_simulator(CBackData *db, int stock_sn, int begin_date, int end_date, int policy)
{
    //create_main_pos
    int cost = db->get_open_price(stock_sn, begin_date);
    if (cost <= 0){
        fprintf(stderr, "FATAL: raw stock cost should no <= 0, but we get:%d\n", cost);
        assert(0);
    }

    string name("unknown");
    CStock stock(db, stock_sn, name, begin_date, COUNT_INIT_MAIN, cost, \
            cost*2, 1.05f, 0.95f, 0.5f);

    //from begin_date to end_date run_t0_operation day by day
    vector<struct day_price_t> trade_days;
    db->get_trade_days(stock_sn, begin_date, end_date, trade_days);

    if (trade_days.size() > 0)
        trade_days.erase (trade_days.begin()); //T0 from the second day of creating main pos
    fprintf(stderr, "There are %lu trade days between %d and %d\n", trade_days.size(), begin_date, end_date);

    if (!trade_days.size()){
        fprintf(stderr, "No trade_days, done.\n");
        return;
    }

    foreach_itt(itt, &trade_days){
        stock.run_t0(*itt, policy);
    }
}

/* Flag set by ‘--verbose’. */
static int verbose_flag;
int main (int argc, char **argv)
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

    run_t0_simulator(db, stock_sn, begin_date, end_date, policy);

    return 0;
}

