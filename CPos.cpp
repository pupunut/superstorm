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
#include "COperation.h"
#include "CStock.h"
#include "CBackData.h"

using namespace std;

CPos:: CPos(CStock *owner, int begin_date, int count, int cost, int clear_price)
{
    m_db = owner->get_db();
    m_owner = owner;
    m_begin_date = begin_date;
    m_end_date = 0;
    m_init_count = m_curr_count = count;
    m_init_cost = m_curr_cost = cost;
    m_status = ST_OPEN;
    m_clear_price = clear_price;

    m_t0_cpr = 1 + 0.05; //the base value is dynamic(day line), so there is only a rate
    m_t0_bpr = 1 - 0.05;
    m_t0_bcr = 0.5;

    COperation *op = new COperation(this, begin_date, DI_BUY, count, cost);
    m_ops[begin_date] = op;
    m_fix_profit.fee = op->get_fee();
}

int CPos::_clear_pos(int date, int price)
{
    assert(m_status == ST_OPEN);
    assert(date >= m_begin_date && m_end_date == 0);

    m_status = ST_CLOSE;
    m_end_date = date;
    m_curr_cost = price;
    if (price < m_clear_price)
        INFO("INFO: pos_id:%lld Not reach target to clear. target price:%d, exec price:%d\n",
                m_id, m_clear_price, price);

    COperation *op = new COperation(this, date, DI_SELL, m_curr_count, price);

    m_ops[date] = op;
    assert(m_init_count == m_curr_count); //we only support buy and sell whole pos once a time

    //we get fix_profit now
    profit_t *p = &m_fix_profit;
    p->fee += op->get_fee();
    p->float_profit = 0;
    p->cash_profit = m_curr_count * (m_curr_cost - m_init_count);
    p->total_profit = p->cash_profit - p->fee;

    return p->total_profit;
}

int CPos::_account_pos(int date)
{
    assert(m_status == ST_OPEN);
    assert(date >= m_begin_date && (date <= m_end_date || m_end_date == 0));

    int price = m_db->get_close_price(m_owner->get_sn(), date);
    return m_curr_count * (price - m_curr_cost) - m_fix_profit.fee;
}

int CPos::_get_profit(int date)
{
    if (date < m_begin_date)
        return 0; //we don't distinguish this case and the real float/fix profit is zero

    if (m_end_date != 0 && date >= m_end_date){
        assert(m_status == ST_CLOSE);
        return m_fix_profit.total_profit;
    }

    if (m_status == ST_CLOSE)
        return m_fix_profit.total_profit;
    else
        return _account_pos(date);
}

void CPos::_print_profit(int date)
{
    if (date < m_begin_date)
        return;

    printf("id, type, status, begin_date, end_date, profit\n");
    printf("%lld,%d,%d,%d,%d,%d\n", \
            m_id, m_type, m_status, m_begin_date, m_end_date, _get_profit(date));
}


