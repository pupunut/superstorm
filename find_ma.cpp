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

bool test_low_nrb(point_policy_coarse_t type, vector<day_price_t> &dp_desc)
{
    //(lastest -1) day is nrb
    day_price_t *c = &dp_desc[0];
    day_price_t *p = &dp_desc[1];

    //the body of curr_dp is shadowed by priv_dp
    //and the range of curr_dp is less than 1/2 of that of priv_dp
    //for debug
    if (c->get_range() >= (p->get_range()*0.67))
        return false;

    /*
    if (c->get_body() >= p->get_body())
        return false;
    */

    if (c->is_red()){ //red
        if (c->high >= p->high || c->close <= p->close)
            return false;
    }else { //blue
        if (c->high >= p->high || c->low <= p->low)
        return false;
    }

    return true;
}

bool test_low_rb(point_policy_coarse_t type, vector<day_price_t> &dp_desc)
{
    //(lastest -1) day is nrb
    day_price_t *c = &dp_desc[0];
    day_price_t *p = &dp_desc[1];

    //curr_dp is red
    if (! c->is_red())
        return false;

    //curr_dp has very short top tail, NOTE: must be very very short
    if (c->get_top_tail() >= c->get_body()/10)
        return false;

    //curr_dp has a long bottom tail
    if (c->get_bottom_tail() <= c->get_body())
        return false;

    //open of curr_pd must less than open of prev_pd, cite: 002576 20140313
    if (c->open >= p->open)
        return false;


    return true;
}

bool test_low_gap(point_policy_coarse_t type, vector<day_price_t> &dp_desc)
{
    return false; //TBD
}

bool test_low_md(point_policy_coarse_t type, vector<day_price_t> &dp_desc)
{
    return false; //TBD
}

void dump_bpn(int sn, day_price_t &dp, point_policy_t policy)
{
    point_t p(sn, dp.date, dp.open, ENUM_PT_IN, ENUM_PPC_NONE, policy);
    lg_db->dump_bpn(&p);
}

void find_mabp_single(int sn, int small_pma, int large_pma)
{
    map<int/*pma*/, vector<ma_t> >ma_map;
    int range = 30;//get and search ma_list in latest 30 trading days
    ma_t mabp;

    //
    lg_db->get_ma(sn, ma_list, range);
    if (ma_map[small_pma].size() != range ||
            ma_map[large_pma].size() != range){
        INFO("sn:%d small_pma:%d large_pma:%d range:%d not enough\n",
                sn, small_pma, large_pma, range);
        goto out;
    }

    //test today's small_pma and large_pma
    vector<ma_t> *small_list = &ma_map[small_pma];
    vector<ma_t> *large_list = &ma_map[large_pma];

    //small_ma should higher than large_ma now
    if ((*small_list)[0] < (*large_list)[0])
        goto out;

    for (int i = 1; i < range; i++){
        if ((*small_list)[i] < (*large_list)[i])

            goto found;
    }

out:
    return;

found:
    lg_db->dump_mabp(
}

void find_mabp(int small_pma, int large_pma)
{

    //get sn list
    vector<int/*sn*/> snlist;
    lg_db->get_all_sn(snlist);

    foreach_itt(itt, &snlist){
        int sn = *itt;
        find_mabp_single(sn, small_pma, large_pma);
    }
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
                lg_small_pma = optarg;
                break;
            case 'l':
                printf ("large pma: %s\n", optarg);
                lg_large_pma = optarg;
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

    lg_db->reset_mabp();
    find_mabp(lg_small_pma, lg_large_pma);
}
