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

//1. dplist is ordering in des
//2. the test do not include the first day in dplist
bool test_drop_3d(vector<day_price_t> &dp_desc)
{
    bool ret = true;

    typeof(dp_desc.begin()) it_begin = dp_desc.begin() + 2; //curr day->nrb day->3 drop days
    typeof(dp_desc.begin()) it_end = it_begin + 3 - 1; //3 days defore the current day
    typeof(dp_desc.begin()) it;

    //must drop every day
    for (it = it_begin; it <= it_end; it++){
        day_price_t *curr_dp = &*it;
        if (curr_dp->open <= curr_dp->close) //go up
            return false;
    }

    //today's open must be lower than that of last day
    //today's close must be lower than that of last day
    for (it = it_begin; it <= it_end; it++){
        day_price_t *curr_dp = &*it;
        day_price_t *priv_dp = &*(it+1);
        if (curr_dp->open >= priv_dp->open)
            return false;
        if (curr_dp->close >= priv_dp->close)
            return false;
    }

    return ret;
}

bool test_low_nrb(point_policy_coarse_t type, vector<day_price_t> &dp_desc)
{
    if (type != ENUM_PPC_3DROP) //only support 3DROP coarse filter now
        return false;

    //(lastest -1) day is nrb
    day_price_t *c = &*(dp_desc.begin() + 1);
    day_price_t *p = &*(dp_desc.begin() + 2);

    //the body of curr_dp is shadowed by priv_dp
    //and the range of curr_dp is less than 1/2 of that of priv_dp
    //for debug
    if (c->get_range() >= (p->get_range()*0.67))
        return false;

    if (c->get_body() >= p->get_body())
        return false;

    if (c->open < c->close){ //red
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
    if (type != ENUM_PPC_3DROP) //only support 3DROP coarse filter now
        return false;

    //(lastest -1) day is nrb
    day_price_t *c = &*(dp_desc.begin() + 1);
    day_price_t *p = &*dp_desc.begin();

    //curr_dp is going up
    if (c->open >= c->close)
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

point_policy_t filter_in_fine(point_policy_coarse_t type, vector<day_price_t> &dp_desc)
{
    //test if the date is a valid trading date
    //alldp is in descending order

    if (test_low_nrb(type, dp_desc))
        return ENUM_PP_LOW_NRB;
    if (test_low_rb(type, dp_desc))
        return ENUM_PP_LOW_RB;
    if (test_low_gap(type, dp_desc))
        return ENUM_PP_LOW_GAP;
    if (test_low_md(type, dp_desc))
        return ENUM_PP_LOW_MD;

    return ENUM_PP_SIZE;
}

bool filter_in_coarse(vector<day_price_t> &dp_desc)
{
    return test_drop_3d(dp_desc);
}

void find_low_once(CBackData *db, int begin_date, int end_date, int day_range)
{
    assert(begin_date);
    assert(end_date);


    map<int/*sn*/, vector<day_price_t> > dp_desc;
    db->get_dp_desc(begin_date, end_date, dp_desc);

    foreach_itt(itt, &dp_desc){
        int sn = itt->first;
        vector<day_price_t> *dplist = &itt->second;
        if (dplist->size() < day_range){
            //INFO("dplist.size:%zd < day_range:%d\n", dplist->size(), day_range);
            continue;
        }

        if (!filter_in_coarse(*dplist)){
            //INFO("failed to pass coarse filter, sn:%d\n", sn);
            continue;
        }

        day_price_t *dp = &*dplist->begin();
        point_t p(sn, end_date, dp->open, ENUM_PT_IN, ENUM_PPC_3DROP);
        db->dump_point(&p);

        point_policy_t policy = filter_in_fine(ENUM_PPC_3DROP, *dplist);
        if (policy != ENUM_PP_SIZE){
            day_price_t *dp = &*dplist->begin();
            point_t p(sn, end_date, dp->open, ENUM_PT_IN, ENUM_PPC_3DROP, policy);
            db->dump_point(&p);
        }
    }
}

void find_low(CBackData *db, int head_date, int tail_date)
{
    //get last date
    if (!tail_date){
        tail_date = db->get_latest_date();
        head_date = 0; //if end_date equals zero, then begin_date is invalid
    }

    //prepare for find_low_once
    int day_range = 5; //search range, curr day->nrb day->3 drop days
    vector<point_t> low_points;
    int end_date = tail_date;
    int begin_date = db->get_latest_range(end_date, day_range);
    INFO("end_date:%d, begin_date:%d\n", end_date, begin_date);
    do {
        find_low_once(db, begin_date, end_date, day_range);
        end_date = db->get_prev_date(end_date);
        begin_date = db->get_latest_range(end_date, day_range);
        INFO("next end_date:%d, begin_date:%d\n", end_date, begin_date);
    }while (begin_date > head_date);
}

static int verbose_flag;
static int lg_head_date = 0;
static int lg_tail_date = 0;
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
            {"head",  required_argument, 0, 'a'},
            {"tail",  required_argument, 0, 't'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "a:d:t:",
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
            case 'a':
                printf ("head date: %s\n", optarg);
                lg_head_date = atoi(optarg);
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
    CBackData *db = setup_db(argc, argv);
    assert(db);

    db->reset_point();
    find_low(db, lg_head_date, lg_tail_date);
}
