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
static int lg_tail_date = 0;

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

void find_bpn_once(int begin_date, int end_date)
{
    assert(begin_date);
    assert(end_date);
    int day_range = 2;

    map<int/*sn*/, vector<day_price_t> > dp_desc;
    lg_db->get_dp_desc(begin_date, end_date, dp_desc);

    foreach_itt(itt, &dp_desc){ //NOTE: dp_desc order by date in DESC
        int sn = itt->first;
        vector<day_price_t> *dplist = &itt->second;
        if (dplist->size() != day_range){
            //INFO("dplist.size:%zd != day_range:%d\n", dplist->size(), day_range);
            continue;
        }

        //the prev day must be blue
        day_price_t *dp_prev = &(*dplist)[1];
        if (! dp_prev->is_blue())
            continue;

        point_policy_t policy = ENUM_PP_NONE;
        if (test_low_nrb(ENUM_PPC_NONE, *dplist))
            dump_bpn(sn, (*dplist)[0], ENUM_PP_LOW_NRB);
        if (test_low_rb(ENUM_PPC_NONE, *dplist))
            dump_bpn(sn, (*dplist)[0], ENUM_PP_LOW_RB);
    }
}

void find_bpn(int tail_date)
{

    //get last date
    if (!tail_date){
        tail_date = lg_db->get_latest_date(); //always a valid date
    }

    int prev_date = lg_db->get_latest_date(tail_date);
    if (!lg_db->is_valid_date(tail_date)){
        ERROR("The tail date:%d is not a valid date. The lastest valid date is:%d\n",
                tail_date, prev_date);
        return;
    }

    //prepare for find_low_once
    INFO("end_date:%d, begin_date:%d\n", tail_date, prev_date);
    find_bpn_once(prev_date, tail_date);
}

CBackData *setup_db(int argc, char *argv[])
{
    char *db_path = NULL;
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
            {"db",    required_argument, 0, 'd'},
            {"tail",  required_argument, 0, 't'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "d:t:",
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
            case 't':
                printf ("tail date: %s\n", optarg);
                lg_tail_date = atoi(optarg);
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
        fprintf(stderr, "Usage: %s -d db_path [-t tail_date [-a head_date]]\n", argv[0]);
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

    lg_db->reset_bpn();
    find_bpn(lg_tail_date);
}
