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
    CPosMaster *m_mpos;
    map<int/*begin date*/, CPosMaster*> m_mpos_map; //all master positions ever

public:
    CStock(CBackData *db, int sn, string& name);
    //create a stock with a master pos
    CStock(CBackData *db, int sn, string& name, int date, int count, int cost);
    int get_sn() { return m_sn; }
    CBackData *get_db() { return m_db; }
    CPosMaster *create_mpos(int date, int count, int cost);
    long long get_next_mposid(int date);

    //for simulator
    int short_mpos(day_price_t& price, int percent);
    int clear_mpos(day_price_t& price);
    int long_mpos(day_price_t& price, int percent) { assert(0);} //TBD

    //for recording real operations
    int short_mpos(int date, int count, int price) { assert(0); } //TBD
    int clear_mpos(int date, int price) { assert(0); } //TBD
    int long_mpos(int date, int count, int price) { assert(0); } //TBD
};

#endif
