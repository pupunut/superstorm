#ifndef _COPERATION_H
#define _COPERATION_H

class COperation //operations
{
private:
    CPos *m_owner;
    int m_id;
    int m_date;
    int m_di;
    int m_count;
    int m_price;
    int m_fee1; //交易佣金
    int m_fee2; //印花税，单向
    int m_fee3; //利息
    int m_fee4; //其他
    int m_fee; //the cost of this ops

public:
    COperation(CPos *owner, int date, int di, int count, int price)
    {
        m_owner = owner;
        m_date = date;
        m_di = di;
        m_count = count;
        m_price = price;

        m_id = owner->get_next_opid();
        m_fee1 = price * count * FEE1_RATIO;
        if (di == DI_SELL)
            m_fee2 = price * count * FEE2_RATIO;
        else
            m_fee2 = 0;

        m_fee4 = m_fee3 = 0;
        m_fee = get_fee();
    }

    int get_fee()
    {
        return (m_fee1 + m_fee2 + m_fee3 + m_fee4);
    }
};

#endif
