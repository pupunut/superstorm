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

int main (int argc, char **argv)
{
    return main_comm(argc, argv, &run_t0_simulator);
}

