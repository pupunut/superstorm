#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>

#include "CStock.h"
#include "Comm.h"
#include "CPosMaster.h"

using namespace std;

CStock::CStock(CBackData *db, int sn, string& name, int date, int count, int cost, \
            int cp, float t0_cpr,float t0_bpr, float t0_bcr)
{
    m_db = db;
    m_sn = sn;
    m_name = name;
    m_cp = cp;
    m_t0_cpr = t0_cpr;
    m_t0_bpr = t0_bpr;
    m_t0_bcr = t0_bcr;

    m_mpos = new CPosMaster(this, date, count, cost, cost*2);
    m_mpos_map[date] = m_mpos;
}

long long CStock::get_next_mposid(int date)
{
    char pos_id[1024];
    char *id = pos_id;

    memset(id, 0, sizeof(id));
    sprintf(id, "%d", date - 20000000);
    id += strlen(id);
    sprintf(id, "%02lu", m_mpos_map.size() + 1);
    return atoll(pos_id);
}

void CStock::run_t0(day_price_t& dp, int policy)
{
    //process tpos till dp.date, including
    //1. clear existed tpos if possible
    //2. clear existed tpos_g2 if possible
    //3. create new tpos_g2 in the dp.date if possible
    m_mpos->process_tpos(dp);

    //day summary
    m_mpos->print_profit(dp.date);
}


