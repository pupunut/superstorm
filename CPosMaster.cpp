#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>

#include "CPosMaster.h"
#include "Comm.h"
#include "CStock.h"
#include "CPosSlave.h"

using namespace std;

CPosMaster::CPosMaster(CStock *owner, int begin_date, int count, int cost)
:CPos(owner, begin_date, count, cost)
{
    m_id = owner->get_next_mposid(begin_date);
    m_type = ENUM_MPOS;
}

void CPosMaster::print_profit(int date)
{
    printf("\n================\n");
    printf("date: %d\n", date);
    printf("================\n");

    _print_profit(date);

    foreach_itt(itt, &m_tpos)
        itt->second->print_profit(date);

    foreach_itt(itt, &m_tpos_g2)
        itt->second->print_profit(date);
}

int CPosMaster::clear_pos(int date, int price)
{
    foreach_itt(itt, &m_tpos){
        if (itt->second->get_status() != ST_CLOSE)
            ASSERT("There are still alive tpos for this mpos, id:%lld\n", m_id);
    }

    return CPos::clear_pos(date, price);
}

CPosSlave *CPosMaster::create_tpos(day_price_t& dp)
{
    assert(0); //TBD
    /*
    CPosSlave *t0 = new CPosSlave(m_owner, this, dp.date, \
            get_t0_bc(), dp.open, get_t0_cp(dp.open));

    return t0;
            */
}

void CPosMaster::clear_tpos(day_price_t& dp, map<int/*date*/, CPosSlave * /*tpos*/> *tpos_map)
{
    assert(0); //TBD
    /*
    foreach_itt(itt, tpos_map){
        CPosSlave *t0 = itt->second;
        int date = itt->first;

        //skip invalid tpos
        if (t0->get_status() != ST_OPEN)
            continue;
        if (dp.high < t0->get_clear_price())
            continue;

        pos_type_t type = t0->get_type();
        switch (type){
            case ENUM_TPOS:
                if (dp.date >= date)
                    t0->clear_pos(dp.date, t0->get_clear_price());
                break;
            case ENUM_T2POS:
                if (dp.date > date)
                    t0->clear_pos(dp.date, t0->get_clear_price());
                break;
            default:
                assert(0);
        }
    }
    */
}

void CPosMaster::process_tpos(day_price_t& dp)
{
    assert(0); //TBD
    //create a t0 first
    /*
    CPosSlave *t0 = create_tpos(dp);
    if (t0)
        m_tpos[dp.date] = t0;

    //create another t0(tpos_g2) for the upper t0 if possible
    if (dp.low <= t0->get_buy_price()){
        CPosSlaveG2 *t2 = (CPosSlaveG2 *)t0->create_tpos(dp);
        if (t2)
            m_tpos_g2[dp.date] = t2;
    }

    //clear old and new tpos if possible
    clear_tpos(dp, &m_tpos);
    clear_tpos(dp, &m_tpos_g2);
    */
}

long long CPosMaster::get_next_tposid(int date)
{
    char pos_id[1024];
    char *id = pos_id;

    memset(id, 0, sizeof(id));
    sprintf(id, "%lld", m_id);
    id += strlen(id);
    sprintf(id, "%d", date - 20000000);
    id += strlen(id);

    int sn = 1;
    {
        typeof(m_tpos.end()) it = m_tpos.find(date);
        if (it != m_tpos.end())
            sn++;
    }
    {
        typeof(m_tpos_g2.end()) it = m_tpos_g2.find(date);
        if (it != m_tpos_g2.end())
            sn++;
    }

    sprintf(id, "%02d", sn);
    return atoll(pos_id);
}




