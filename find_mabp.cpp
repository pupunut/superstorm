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
static int lg_small_pma = 50, lg_large_pma = 200;

void find_mabp_single(int sn, int small_pma, int large_pma, vector<mabp_t> &mabp_list)
{
    map<int/*pma*/, vector<ma_t> >ma_map;
    int range = 30;//get and search ma_list in latest 30 trading days

    //
    lg_db->get_ma(sn, ma_map);
    if (ma_map[small_pma].size() < range ||
            ma_map[large_pma].size() < range){
        INFO("sn:%d small_pma:%d large_pma:%d range:%d not enough\n",
                sn, small_pma, large_pma, range);
        return;
    }

    //test today's small_pma and large_pma
    vector<ma_t> *small_list = &ma_map[small_pma];
    vector<ma_t> *large_list = &ma_map[large_pma];

    //small_ma should higher than large_ma now
    ma_t *small_ma = &((*small_list)[0]);
    ma_t *large_ma = &((*large_list)[0]);
    if (small_ma->avg < large_ma->avg)
        return;

    for (int i = 1; i < range; i++){
        small_ma = &((*small_list)[i]);
        large_ma = &((*large_list)[i]);
        if (small_ma->avg < large_ma->avg){
            mabp_t mabp(sn, small_ma->date, small_pma, large_pma);
            mabp_list.push_back(mabp);
            return;
        }
    }
}

void find_mabp(int small_pma, int large_pma)
{

    //get sn list
    vector<int/*sn*/> snlist;
    lg_db->get_all_sn(snlist);

    vector<mabp_t> mabp_list;

    foreach_itt(itt, &snlist){
        int sn = *itt;
        find_mabp_single(sn, small_pma, large_pma, mabp_list);
    }

    lg_db->dump_mabp(mabp_list);
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
            {"small",  required_argument, 0, 's'},
            {"large",  required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "d:l:s:",
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
            case 's':
                printf ("small pma: %s\n", optarg);
                lg_small_pma = atoi(optarg);
                break;
            case 'l':
                printf ("large pma: %s\n", optarg);
                lg_large_pma = atoi(optarg);
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
        fprintf(stderr, "Usage: %s -d db_path [-s small_pma(default %d) -l large_pma(default %d)]\n",
                argv[0], lg_small_pma, lg_large_pma);
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

    lg_db->reset_mabp();
    find_mabp(lg_small_pma, lg_large_pma);
}
