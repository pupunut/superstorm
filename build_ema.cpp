#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "Comm.h"
#include "CBackData.h"

using namespace std;

static int verbose_flag;
static CBackData *lg_db = NULL;
static vector<int/*param for ema*/> lg_pema_list;

void _build_ema_single(int sn, int pema, vector<ma_t> &ma_list, int last_pema_date, vector<ema_t> &ema_list)
{
    for(int i = 0; (i+pema) <= ma_list.size(); i++){
        if (ma_list[i].date < last_pema_date) //already in db
            continue;

        //assume ma_list in DESC order
        int sum = 0;
        for(int j = 0; j < pema; j++){
            sum += ma_list[i+j].avg;
        }

        int avg = ((sum*10 / pma) + 5) / 10;
        ma_t ma(sn, dp_list[i].date, pma, avg);
        ma_list.push_back(ma);

        if (i > 30) //we only build ma in 30 trading days
            break;
    }
}

void build_ema_single(int sn, vector<int/*pema*/> &pema_list, vector<ema_t> &ema_list)
{
    vector<ma_t> ma_list;
    lg_db->get_ma(sn, ma_list);

    foreach_itt(itt, &pma_list){
        int pema = *itt;
        int last_pema_date = lg_db->get_last_pema_date(sn, pema);
        _build_ema_single(sn, pema, ma_list, last_pema_date, ema_list);
    }
}

void build_ema(vector<int/*ema param*/> &pema_list)
{

    //get sn list
    vector<int/*sn*/> snlist;
    lg_db->get_all_sn(snlist);

    map<int/*sn*/, vector<ema_t> >ema_map;
    foreach_itt(itt, &snlist){
        int sn = *itt;
        build_ema_single(sn, pema_list, ema_map[sn]);
    }

    lg_db->dump_ema(ema_map);
}

CBackData *setup_db(int argc, char *argv[])
{
    char *db_path = NULL;
    int stock_sn, begin_date, end_date, policy;

    int c;

    while (1)
    {
        char delims[] = ",";
        char *result = NULL;
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"db",    required_argument, 0, 'd'},
            {"ema",  required_argument, 0, 'n'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "d:n:",
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
            case 'd':
                printf ("database: %s\n", optarg);
                db_path = optarg;
                break;
            case 'n':
                printf ("target ema: %s\n", optarg);
                result = strtok(optarg, delims);
                while (result != NULL){
                    printf( "dl is \"%d\"\n", atoi(result));
                    lg_pma_list.push_back(atoi(result));
                    result = strtok(NULL, delims);
                }
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

    if (!db_path || !lg_pma_list.size()){
        fprintf(stderr, "Usage: %s -d db_path -n num1,num2,num3...\n", argv[0]);
        exit(-1);
    }

    //OK , let's begin
    CBackData *db = new CBackData(ENUM_BACKDATA_DB, db_path);
    return db;
}

int main (int argc, char **argv)
{
    lg_db = setup_db(argc, argv);
    assert(lg_db);

    lg_db->reset_ema();
    if (!lg_pema_list.size()){
        lg_pema_list.push_back(12);
        lg_pema_list.push_back(26);
    }

    build_ema(lg_pema_list);
}
