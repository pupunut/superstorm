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

using namespace std;

//1. dplist is ordering in des
//2. the test do not include the first day in dplist
bool test_drop_3d(vector<day_price_t> &dp_desc)
{
    bool ret = true;

    typeof(dp_desc.begin()) it_begin = dp_desc.begin() + 1;
    typeof(dp_desc.begin()) it_end = it_begin + 3; //3 days defore the current day
    typeof(dp_desc.begin()) it;

    //3 days' dropping
    for (it = it_begin; it <= it_end; it++){
        day_price_t *curr_dp = &*it;
        day_price_t *priv_dp = &*(it+1);
        if (curr_dp->open < curr_dp->close) //go up
            return false;
        if (priv_dp->open < priv_dp->close) //go up
            return false;
        if (curr_dp->open > (priv_dp->close + (priv_dp->open - priv_dp->close)/3)) //not go down sequential
            return false;
    }

    return ret;
}

bool test_low_nrb(vector<day_price_t> &dp_desc)
{
    if (false == test_drop_3d(dp_desc))
        return false;

    //lastest day is nrb
    day_price_t *c = &(*dp_desc.begin());
    day_price_t *p = &(*(dp_desc.begin() + 1));

    //the body of curr_dp is shadowed by priv_dp
    //and the range of curr_dp is less than 1/2 of that of priv_dp
    if (c->get_range() < p->get_range()/2
        && c->low <= p->low)
        return true;

    return true;
}

bool test_low_rb(vector<day_price_t> &dp_desc)
{
    if (false == test_drop_3d(dp_desc))
        return false;

    //lastest day is rb
    day_price_t *c = &(*dp_desc.begin());

    //curr_dp is going up
    if (c->open >= c->close)
        return false;

    //curr_dp has very short top tail, NOTE: must be very very short
    if (c->get_top_tail() > c->get_range()/10)
        return false;

    //curr_dp has a long bottom tail
    if (c->get_bottom_tail() < c->get_range()/2)
        return false;


    return true;
}

bool test_low_gap(vector<day_price_t> &dp_desc)
{
    return false; //TBD
}

bool test_low_md(vector<day_price_t> &dp_desc)
{
    return false; //TBD
}

point_policy_t _find_current_low(vector<day_price_t> &dp_desc)
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

void find_current_low(CBackData *db, vector<point_t> &low_points)
{
    vector<int/*stock sn*/> allset;
    //get last date
    int last_date = db->get_lastest_date();
    assert(!db->get_all_sn(allset));

    foreach_itt(itt, &allset){
        int sn = *itt;
        vector<day_price_t> dp_desc;
        db->get_dp_desc(sn, last_date, dp_desc);
        if (!dp_desc.size())
            continue;

        point_policy_t ret = _find_current_low(dp_desc);
        if (ret != ENUM_PP_SIZE){
            day_price_t *dp = &*dp_desc.begin();
            point_t p(sn, last_date, dp->open, 0, ret);
            low_points.push_back(p);
        }
    }
}

static int verbose_flag;
CBackData *setup_db(int argc, char *argv[])
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
                db_path.assign(optarg);
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
    return db;
}
int main (int argc, char **argv)
{
    CBackData *db = setup_db(argc, argv);
    assert(db);

    vector<point_t> low_points;
    find_current_low(db, low_points);

    foreach_itt(itt, &low_points){
        (*itt).print();
    }
}
