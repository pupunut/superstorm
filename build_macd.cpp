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
static vector<int/*param for dea*/> lg_pdea_list;

void _build_ema_single(int sn, int pema, vector<day_price_t> &dp_list, int last_pema_date, vector<ema_t> &ema_list)
{
    //compute for every valid day
    for(int i = 0; (i+pema) <= dp_list.size(); i++){
        if (dp_list[i].date < last_pema_date) //already in db
            continue;

        //assume ma_list in DESC order
        //compute ema for this day: ma[i]
        int sum = 0;
        int value = 0;
        for (int j = pema; j; j--){
            sum += j;
            value += dp_list[i+pema-j].close * j;
        }

        value = (value*10 / sum + 5) / 10;

        ema_t ema(sn, dp_list[i].date, pema, value);
        ema_list.push_back(ema);

        if (i > 30) //we only build ma in 30 trading days
            break;
    }
}

void build_ema_single(int sn, vector<int/*pema*/> &pema_list, vector<ema_t> &ema_list)
{
    vector<day_price_t> dp_list;
    lg_db->get_dp_desc(sn, dp_list);

    foreach_itt(itt, &pema_list){
        int pema = *itt;
        int last_pema_date = lg_db->get_last_pema_date(sn, pema);
        _build_ema_single(sn, pema, dp_list, last_pema_date, ema_list);
    }
}

void build_ema(vector<int/*ema param*/> &pema_list)
{
    //get sn list
    vector<int/*sn*/> snlist;
    lg_db->get_all_sn(snlist);

    INFO("Begin Build EMA\n");
    map<int/*sn*/, vector<ema_t> >ema_map;
    int total = snlist.size();
    int num_print = total / 10;
    int count = 0;
    foreach_itt(itt, &snlist){
        int sn = *itt;
        build_ema_single(sn, pema_list, ema_map[sn]);
        if (!(++count % num_print))
            fprintf(stderr, "Build EMA No.%d/%d\r", count, total);
    }
    INFO("End Build EMA No.%d/%d\n", count, total);

    INFO("Dump EMA...\n");
    lg_db->dump_ema(ema_map);
}

void _build_macd(map<int/*sn*/, vector<macd_t> > &macd_map)
{
    foreach_itt(itt, &macd_map){
        foreach_itt(itm, &itt->second){
            macd_t *macd = &*itm;
            macd->macd = macd->diff - macd->dea;
        }
    }
}

void _build_dea_single(int sn, int pdea, int last_pdea_date, vector<macd_t> &macd_list)
{
    //compute for every valid day
    for(int i = 0; (i+pdea) <= macd_list.size(); i++){
        if (macd_list[i].date < last_pdea_date) //already in db
            continue;

        //assume ma_list in DESC order
        //compute dea for this day: ma[i]
        int sum = 0;
        int value = 0;
        for (int j = pdea; j; j--){
            sum += j;
            value += macd_list[i+pdea-j].diff * j;
        }

        value = (value*10 / sum + 5) / 10;
        macd_list[i].dea = value;

        if (i > 30) //we only build ma in 30 trading days
            break;
    }
}

void build_dea_single(int sn, vector<int/*pdea*/> &pdea_list, vector<macd_t> &macd_list)
{
    foreach_itt(itt, &pdea_list){
        int pdea = *itt;
        int last_pdea_date = lg_db->get_last_pdea_date(sn);
        if (!last_pdea_date)
            continue; //no this diff data about this sn in table macd
        _build_dea_single(sn, pdea, last_pdea_date, macd_list);
    }
}


void build_macd(vector<int/*dea param*/> &pdea_list)
{
    INFO("Reset macd\n");
    lg_db->reset_macd();

    INFO("Build DIFF\n");
    lg_db->build_diff();

    INFO("Begin Build DEA\n");
    //get sn list
    vector<int/*sn*/> snlist;
    lg_db->get_all_sn(snlist);

    //get diff
    map<int/*sn*/, vector<macd_t> >macd_map;
    lg_db->get_macd(macd_map);

    int total = snlist.size();
    int num_print = total / 10;
    int count = 0;
    foreach_itt(itt, &snlist){
        int sn = *itt;
        build_dea_single(sn, pdea_list, macd_map[sn]);
        if (!(++count % num_print))
            fprintf(stderr, "Build DEA No.%d/%d\r", count, total);
    }
    INFO("End Build DEA No.%d/%d\n", count, total);

    INFO("Build MACD\n");
    _build_macd(macd_map);

    INFO("Dump MACD...\n");
    lg_db->dump_macd(macd_map);
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
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "d:",
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

    if (!db_path){
        fprintf(stderr, "Usage: %s -d db_path\n", argv[0]);
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

//    build_ema(lg_pema_list);

    if (lg_pema_list.size() == 2 && lg_pema_list[0] == 12 && lg_pema_list[1] == 26){
        lg_pdea_list.push_back(9);
        build_macd(lg_pdea_list);
    }
}
