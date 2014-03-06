#ifndef _CPOS_SLAVE_H
#define _CPOS_SLAVE_H

#include "CPos.h"

class CStock;
class CPos;
class CPosMaster;
class CPosSlave;
class CPosSlaveG2;

class CPosSlave : public CPos{
protected:
    CPosMaster *m_holder;
    CPosSlave *m_buddy;
public:
    CPosSlave(CStock *owner, CPosMaster *holder, int begin_date, int count, int cost, int clear_price);
    CPosSlave(CPosMaster *mp, day_price_t& dp);
    bool is_mpos() { return false; }
    bool is_tpos() { return true; }
    bool is_t2pos() { return false; }
    CPosMaster *get_holder() { return m_holder; }
    virtual CPosSlave *create_tpos(day_price_t& dp);
    virtual int get_buy_price() { return (int)((float)m_init_cost * m_t0_bpr); }
    void print_profit(int date) { return _print_profit(date); }
    int clear_pos(int date, int price) { return _clear_pos(date, price); }
};

class CPosSlaveG2 : public CPosSlave{
protected:
public:
    CPosSlaveG2(CPosSlave *sp, day_price_t& dp);
    bool is_mpos() { return false; }
    bool is_tpos() { return true; }
    bool is_t2pos() { return true; }
    virtual CPosSlave *create_tpos(day_price_t& dp) { return NULL; };
    virtual int get_buy_price() { assert(0); }
};

#endif
