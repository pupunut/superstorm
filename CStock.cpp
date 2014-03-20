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

CStock::CStock(CBackData *db, int sn, string& name)
{
    m_db = db;
    m_sn = sn;
    m_name = name;
}

CStock::CStock(CBackData *db, int sn, string& name, int date, int count, int cost)
{
    m_db = db;
    m_sn = sn;
    m_name = name;

    assert(m_mpos == NULL); //only support 1 alive master pos
    m_mpos = new CPosMaster(this, date, count, cost);
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

CPosMaster *CStock::create_mpos(int date, int count, int cost)
{
    assert(m_mpos == NULL); //only support 1 alive master pos
    m_mpos = new CPosMaster(this, date, count, cost);
    m_mpos_map[date] = m_mpos;
    return m_mpos;
}

int CStock::short_mpos(day_price_t& dp, int percent)
{
    dp.print("Short mpos today");
    int ret = m_mpos->short_pos(dp.date, dp.open, percent);

    //day summary
    m_mpos->print_profit(dp.date);

    if (m_mpos->get_count() == 0){ //mpos has been cleared already
        m_mpos = NULL;
        return RET_END;
    }
    return 0;
}

int CStock::clear_mpos(day_price_t& dp)
{
    dp.print("Clear mpos today");
    int ret = m_mpos->clear_pos(dp);
    m_mpos = NULL;

    //day summary
    m_mpos->print_profit(dp.date);

    return RET_END;
}




