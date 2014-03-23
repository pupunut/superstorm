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
bool test_drop_3d(vector<day_price_t> *dp_desc)
{
    bool ret = true;

    typeof(dp_desc->begin()) it_begin = dp_desc->begin() + 1;
    typeof(dp_desc->begin()) it_end = it_begin + 3 - 1; //3 days defore the current day
    typeof(dp_desc->begin()) it;

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

bool test_low_nrb(vector<day_price_t> *dp_desc)
{
    if (false == test_drop_3d(dp_desc))
        return false;

    //lastest day is nrb
    day_price_t *c = &(*dp_desc->begin());
    day_price_t *p = &(*(dp_desc->begin() + 1));

    //the body of curr_dp is shadowed by priv_dp
    //and the range of curr_dp is less than 1/2 of that of priv_dp
    //for debug
    if (c->get_range() >= p->get_range())
        return false;

    if (c->open < c->close){ //red
        if (c->open < p->close)
            return false;
    }else { //blue
        if (c->low <= p->low)
        return false;
    }

    return true;
}

bool test_low_rb(vector<day_price_t> *dp_desc)
{
    if (false == test_drop_3d(dp_desc))
        return false;

    //lastest day is rb
    day_price_t *c = &(*dp_desc->begin());

    //curr_dp is going up
    if (c->open >= c->close)
        return false;

    //curr_dp has very short top tail, NOTE: must be very very short
    if (c->get_top_tail() >= c->get_body()/10)
        return false;

    //curr_dp has a long bottom tail
    if (c->get_bottom_tail() <= c->get_body()/2)
        return false;

    return true;
}

bool test_low_gap(vector<day_price_t> *dp_desc)
{
    return false; //TBD
}

bool test_low_md(vector<day_price_t> *dp_desc)
{
    return false; //TBD
}

point_policy_t _find_current_low(vector<day_price_t> *dp_desc)
{
    //test if the date is a valid trading date
    //alldp is in descending order

    if (test_low_nrb(dp_desc))
        return ENUM_PP_LOW_NRB;
    if (test_low_rb(dp_desc))
        return ENUM_PP_LOW_RB;
    if (test_low_gap(dp_desc))
        return ENUM_PP_LOW_GAP;
    if (test_low_md(dp_desc))
        return ENUM_PP_LOW_MD;

    return ENUM_PP_SIZE;
}

void find_current_low(CBackData *db, int end_date, vector<point_t> *low_points)
{
    int day_range = 3;
    vector<int/*stock sn*/> allset;
    //get last date
    if (!end_date)
        end_date = db->get_latest_date();
    int begin_date = db->get_latest_range(end_date, day_range);

    map<int/*sn*/, vector<day_price_t> > dp_desc;
    db->get_dp_desc(begin_date, end_date, dp_desc);

    foreach_itt(itt, &dp_desc){
        int sn = itt->first;
        vector<day_price_t> *dplist = &itt->second;
        if (dplist->size() < day_range){
            //INFO("%d has no %d trading days before %d\n", sn, day_range, end_date);
            continue;
        }

        point_policy_t ret = _find_current_low(dplist);
        if (ret != ENUM_PP_SIZE){
            day_price_t *dp = &*dplist->begin();
            point_t p(sn, end_date, dp->open, 0, ret);
            low_points->push_back(p);
        }
    }
}

static int verbose_flag;
static int lg_end_date = 0;
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
            {"end",  required_argument, 0, 'e'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "d:e:",
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
            case 'e':
                printf ("end date: %s\n", optarg);
                lg_end_date = atoi(optarg);
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
        fprintf(stderr, "Usage: %s -d db_path [-e end_date]\n", argv[0]);
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

    vector<point_t> low_points;
    find_current_low(db, lg_end_date, &low_points);

    sort(low_points.begin(), low_points.end());
    foreach_itt(itt, &low_points){
        (*itt).print("IN");
    }
}
