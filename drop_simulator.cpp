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

void run_drop_simulator(CBackData *db, int stock_sn, int begin_date, int end_date, int policy)
{
    //create_main_pos
    int cost = db->get_open_price(stock_sn, begin_date);
    if (cost <= 0){
        fprintf(stderr, "FATAL: raw stock cost should no <= 0, but we get:%d\n", cost);
        assert(0);
    }

    string name("unknown");
    CStock stock(db, stock_sn, name, begin_date, COUNT_INIT_MAIN, cost, 3);

    //from begin_date to end_date run_t0_operation day by day
    vector<struct day_price_t> trade_days;
    db->get_trade_days(stock_sn, begin_date, trade_days);
    assert(trade_days.size() > 1);

    struct day_price_t last_dp;
    foreach_itt(itt, &trade_days){
        if (itt == trade_days.begin()){
            last_dp = *itt;
            continue;
        }

        bool to_drop = true;
        if (last_dp.open < last_dp.close && (*itt).open >= last_dp.open)
            to_drop = false;
        else if (last_dp.open >= last_dp.close && (*itt).open >= last_dp.close)
            to_drop = false;

        if (to_drop)
            if (RET_END == stock.run_drop(*itt))
                return;

        last_dp = *itt;
    }
}

int main (int argc, char **argv)
{
    return main_comm(argc, argv, &run_drop_simulator);
}

