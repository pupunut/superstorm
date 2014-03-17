#ifndef _CSTOCK_H
#define _CSTOCK_H

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
class CBackData;

class CStock
{
private:
    CBackData *m_db;
    int m_sn;
    string m_name;
    int m_cp; //clear price rate: increasing rate of open price
    float m_t0_cpr; //t0 clear price rate: increasing rate of open price
    float m_t0_bpr; //t0 buy price rate: increasing rate of open price
    float m_t0_bcr; //t0 buy price rate: increasing rate of open price
    int m_psc; //percentage of short count, default 1/3
    CPosMaster *m_mpos;
    map<int/*begin date*/, CPosMaster*> m_mpos_map; //all master positions ever

public:
    CStock(CBackData *db, int sn, string& name, int date, int count, int cost, \
            int cp, float t0_cpr,float t0_bpr, float t0_bcr);
    CStock(CBackData *db, int sn, string& name, int date, int count, int cost, \
            int psc);
    int get_sn() { return m_sn; }
    CBackData *get_db() { return m_db; }
    long long get_next_mposid(int date);
    void process_old_tpos(day_price_t& price);
    void run_t0(day_price_t& price, int policy);
    int run_drop(day_price_t& price);
};

#endif
