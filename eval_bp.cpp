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
static int lg_head_date = 0;
static int lg_tail_date = 0;
static CBackData *lg_db = NULL;

void eval_bp_single_3s(point_t *p, map<int/*date*/, day_price_t> *dp)
{
    return ;
}

void eval_bp_single_3d(point_t *p, map<int/*date*/, day_price_t> *dp)
{
    typeof (dp->begin()) it = dp->find(p->date);
    if (it == dp->end()){
        ERROR("Should found this point in dayline:\n");
        p->print("");
        assert(0);
    }

    //it = it + 2; //the third day after point
    it++;it++; //the third day after point
    lg_db->clear_point(p, &it->second, ENUM_SP_3D);
}

void eval_bp_single(int sn, vector<point_t> &plist)
{
    map<int/*date*/, day_price_t/*dp*/> dayline;
    lg_db->get_day_line(sn, dayline);

    foreach_itt(itt, &plist){
        point_t *p = &*itt;
        eval_bp_single_3d(p, &dayline);
        eval_bp_single_3s(p, &dayline);
    }
}

void eval_bp()
{
    //get point
    map<int/*sn*/, vector<point_t> > sn_list;
    lg_db->get_point_sn(sn_list);
    if (sn_list.size() == 0)
        return;

    //evaluate single group of points
    foreach_itt(itt, &sn_list){
        eval_bp_single(itt->first, itt->second);
    }
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

    THROW_ASSERT(!lg_db->reset_sp());
    eval_bp();
}
