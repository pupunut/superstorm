#ifndef _CPOS_H
#define _CPOS_H
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

using namespace std;

class CStock;
class CPos;
class CPosMaster;
class CPosSlave;
class CPosSlaveG2;
class COperation;
class CBackData;

//the struct support t0_pos operate multiple time，but now we only support
//limit times of operations:
//first: buy, create a new t0_pos
//second: sell or create 1 another t0_pos（a special buy)
//Base upon assumption, the profit of tpos or mpos is simplified
//for buy: there is a fixed fee.
//for sell: there is cash_profit and total_profit(total_profit=cash_profit-fee)
//for dynamic day view for a open pos, the total_profit =
//(float_profit - fee) based on day_price
//for dynamic day view for a close pos, the total_profit =
//(cash_profit - fee)
//NOTE: every pos can have one or more t0 pos, but we only let some special 
//classes of pos have capabilicty/member functions to do that
class CPos {
protected:
    CBackData *m_db;
    CStock *m_owner;
    long long m_id;
    pos_type_t m_type;
    int m_begin_date;
    int m_end_date;
    int m_status;
    int m_init_count;
    int m_init_cost;
    int m_curr_count;
    int m_curr_cost;
    profit_t m_fix_profit; //the result of clear_pos(), NOTE: float profit should be gotten by account_pos()
    map<int/*date*/, COperation * /*operation*/> m_ops;

    int _account_pos(int date);
    int _get_profit(int date);
    void _print_profit(int date);

public:
    CPos() { }
    CPos(CStock *owner, int begin_date, int count, int cost);
    virtual bool is_mpos() { assert(0); }
    virtual bool is_tpos() { assert(0); }
    virtual bool is_t2pos() { assert(0); }
    int get_next_opid() { return m_ops.size() + 1; }
    virtual CPosSlave *create_tpos(day_price_t& dp) { assert(0); };
    CStock *get_owner() { return m_owner; }
    int get_status() { return m_status; }
    pos_type_t get_type() { return m_type; }
    int get_count() { return m_curr_count; }

    //for simulator
    virtual int short_pos(day_price_t &dp, int percent);
    virtual int clear_pos(day_price_t &dp);

    //for recording
    virtual int short_pos(int date, int count, int price) { assert(0); }
    virtual int clear_pos(int date, int price) { assert(0); }
};


#endif
