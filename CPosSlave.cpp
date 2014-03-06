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
#include "CPosSlave.h"
#include "Comm.h"

using namespace std;

CPosSlave::CPosSlave(CPosMaster *mp, day_price_t& dp) //for create t0
:CPos(mp->get_owner(), dp.date, mp->get_t0_bc(), dp.open, mp->get_t0_cp(dp.open))
{
    m_id = mp->get_next_tposid(dp.date);
    m_type = ENUM_TPOS;
    m_holder = mp;
    m_buddy = NULL;
}

CPosSlave::CPosSlave(CStock *owner, CPosMaster *holder, int begin_date, int count, int cost, int clear_price)
:CPos(owner, begin_date, count, cost, clear_price)
{
    m_id = holder->get_next_tposid(begin_date);
    m_type = ENUM_TPOS;
    m_holder = holder;
    m_buddy = NULL;
}

CPosSlave *CPosSlave::create_tpos(day_price_t& dp)
{
    CPosSlave *t0 = new CPosSlaveG2(this, dp);
    m_buddy = t0;

    return t0;
}

CPosSlaveG2::CPosSlaveG2(CPosSlave *sp, day_price_t& dp) //for create t0
    :CPosSlave(sp->get_owner(), sp->get_holder(), dp.date, sp->get_t0_bc(), sp->get_t0_bp(dp.open), dp.open)
{
    m_type = ENUM_T2POS;
    m_buddy = sp;
}


