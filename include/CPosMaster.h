#ifndef _CPOS_MASTER_
#define _CPOS_MASTER_

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>

#include "CPos.h"
#include "Comm.h"

using namespace std;

class CStock;
class CPos;
class CPosMaster;
class CPosSlave;
class CPosSlaveG2;

class CPosMaster : public CPos{
protected:
    map<int/*date*/, CPosSlave* /*T0  pos*/> m_tpos; //1 tpos one day at most
    map<int/*date*/, CPosSlave* /*T0  pos*/> m_tpos_g2; //1 t2pos one day at most
public:
    CPosMaster(CStock *owner, int begin_date, int count, int cost);
    bool is_mpos() { return true; }
    bool is_tpos() { return false; }
    bool is_t2pos() { return false; }
    long long get_next_tposid(int date);
    void process_tpos(day_price_t& dp);
    void clear_tpos(day_price_t& dp, map<int/*date*/, CPosSlave * /*tpos*/> *tpos_map);
    int clear_pos(int date, int price); //clear itself
    int clear_pos(day_price_t &dp) { return CPos::clear_pos(dp); }
    virtual CPosSlave *create_tpos(day_price_t& dp);
    void print_profit(int date);
};

#endif

