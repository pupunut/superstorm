#if !defined(_T0_H_)
#define _T0_H_

#include <assert.h>
#include <sqlite3.h>

#include <map>
#include <string>

using namespace std;

#define DI_BUY 1
#define DI_SELL 0

#define ST_OPEN 1
#define ST_CLOSE 0

#define ZERO_PRICE_RATE 0.001
#define FEE1_RATIO 0.0008
#define FEE2_RATIO 0.001
#define FEE3_RATIO 0.1

#define COUNT_INIT_MAIN 10000
#define COUNT_INIT_T0 2 //T0 pos, how many stocks against main pos to operate
#define COUNT_T0_S1 2 //first step, how many stocks against T0_pos to operate
#define COUNT_T0_S2 2 //second step, hwo many stocks against T0_pos to operate
#define DEFAULT_T0_BCR 2//denominator
#define DEFAULT_T0_BPR 20//denominator
#define DEFAULT_T0_CPR 20//denominator

#define CONCATNATE(a, b) a##b
#define iterator_of(item) CONCATNATE(item, _itt)

#define foreach_itt(itt, container)\
    for (typeof((container)->begin()) itt = (container)->begin();\
            itt != (container)->end(); itt++)

#define foreach(item, container)\
    for (typeof((container)->begin()) iterator_of(item) = (container)->begin();\
            iterator_of(item) != (container)->end() && ({item = *iterator_of(item); true;});\
            iterator_of(item)++)

#ifdef _DEBUG
#define DEBUG(...) \
do {\
    fprintf(stderr, "DEBUG: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
} while (0)
#else
#define DEBUG(...)
#endif

#ifdef _DEBUG
#define INFO(...) \
do {\
    fprintf(stderr, "INFO: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
} while (0)
#else
#define INFO(...) \
do {\
    fprintf(stderr, __VA_ARGS__);\
} while (0)
#endif

#define WARN(...) \
do {\
    fprintf(stderr, "WARN: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
} while (0)

#define ERROR(...) \
do {\
    fprintf(stderr, "ERROR: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
} while (0)

#define RETERR(...) \
do {\
    fprintf(stderr, "ERROR: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
    return -1; \
} while (0)

#define THROW(...) \
do {\
    fprintf(stderr, "FATAL: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
    assert(0);\
} while (0)

#define THROW_ASSERT(a, ...)	\
do {\
	if (!(a)) {\
		THROW("Assertion `" #a "' failed.\nDESC: "__VA_ARGS__);\
	}\
} while (0)

#define ASSERT(...) THROW_ASSERT(0, __VA_ARGS__)

typedef enum{
    ENUM_MPOS = 1,
    ENUM_TPOS,
    ENUM_T2POS,
    ENUM_POS_TYPE_SIZE
}pos_type_t;


class CStock;
class CPos;
class CPosSlave;
class CPosSlaveG2;

struct day_price_t{
    int date;
    int open;
    int high;
    int low;
    int close;
    int count;
    int total;

    day_price_t(int date, int open, int high, int low, int close, int count, int total)
    {
        this->date = date;
        this->open = open;
        this->high = high;
        this->low = low;
        this->close = close;
        this->count = count;
        this->total = total;
    }
};

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
    COperation(struct CPos *owner, int date, int di, int count, int price);
    int get_fee();
};

typedef struct profit_s{
    int fee;
    int float_profit;
    int cash_profit;
    int total_profit;

    profit_s()
    {
        fee = 0;
        float_profit = 0;
        cash_profit = 0;
        total_profit = 0;
    }
}profit_t;

//the struct support t0_pos operate multiple time，but now we only support
//limit times of operations:
//first: buy, create a new t0_pos
//second: sell or create 1 another t0_pos（a special buy)
//Base upon assumption, the profit of tpos or mpos is simplified
//for buy: there is a fixed fee.
//for sell: there is cash_profit and total_profit(total_profit=cash_profit-fee)
//for dynamic day view for a open pos, the total_profit =
//(float_profit - fee) based on day_price
//for dynamic day view for a close pos, the total_profit =
//(cash_profit - fee)
//NOTE: every pos can have one or more t0 pos, but we only let some special 
//classes of pos have capabilicty/member functions to do that
class CPos {
protected:
    CStock *m_owner;
    long long m_id;
    pos_type_t m_type;
    int m_begin_date;
    int m_end_date;
    int m_status;
    int m_init_count;
    int m_init_cost;
    int m_curr_count;
    int m_curr_cost;
    int m_clear_price; //target price to clear this pos, is a fixed value when this pos is created
    float m_t0_cpr; //t0 clear price rate: increasing rate of open price
    float m_t0_bpr; //t0 buy price rate: increasing rate of open price
    float m_t0_bcr; //t0 buy count rate;
    profit_t m_fix_profit; //the result of clear_pos(), NOTE: float profit should be gotten by account_pos()
    map<int/*date*/, COperation * /*operation*/> m_ops;

    int _clear_pos(int date, int price);
    int _account_pos(int date);
    int _get_profit(int date);
    void _print_profit(int date);

public:
    CPos() { }
    CPos(CStock *owner, int begin_date, int count, int cost, int clear_price);
    virtual bool is_mpos() { assert(0); }
    virtual bool is_tpos() { assert(0); }
    virtual bool is_tv2pos() { assert(0); }
    virtual CPosSlave *create_tpos(day_price_t& dp) { assert(0); };
    CStock *get_owner() { return m_owner; }
    int get_status() { return m_status; }
    int get_clear_price() { return m_clear_price; }
    virtual int get_buy_price() { assert(0); }
    pos_type_t get_type() { return m_type; }
    int get_next_opid() { return m_ops.size() + 1; }
    int get_t0_bc() { return (int)((float)m_curr_count * m_t0_bcr); }
    int get_t0_cp(int price) { return (int)((float)price * m_t0_cpr); }
    int get_t0_bp(int price) { return (int)((float)price * m_t0_bpr); }

};

class CPosMaster : public CPos{
protected:
    map<int/*date*/, CPosSlave* /*T0  pos*/> m_tpos;
    map<int/*date*/, CPosSlave* /*T0  pos*/> m_tpos_g2;
public:
    CPosMaster(CStock *owner, int begin_date, int count, int cost, int  clear_price);
    bool is_mpos() { return true; }
    bool is_tpos() { return false; }
    bool is_tv2pos() { return false; }
    long long get_next_tposid(int date);
    void process_tpos(day_price_t& dp);
    void clear_tpos(day_price_t& dp, map<int/*date*/, CPosSlave * /*tpos*/> *tpos_map);
    int clear_pos(int date, int price); //clear itself
    virtual CPosSlave *create_tpos(day_price_t& dp);
    void print_profit(int date);
};

class CPosSlave : public CPos{
protected:
    CPosMaster *m_holder;
public:
    CPosSlave(CStock *owner, CPosMaster *holder, int begin_date, int count, int cost, int clear_price);
    CPosSlave(CPosMaster *mp, day_price_t& dp);
    bool is_mpos() { return false; }
    bool is_tpos() { return true; }
    bool is_tv2pos() { return false; }
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
    bool is_tv2pos() { return true; }
    virtual CPosSlave *create_tpos(day_price_t& dp) { return NULL; };
    virtual int get_buy_price() { assert(0); }
};

class CStock
{
private:
    int m_sn;
    string m_name;
    int m_cp; //clear price rate: increasing rate of open price
    float m_t0_cpr; //t0 clear price rate: increasing rate of open price
    float m_t0_bpr; //t0 buy price rate: increasing rate of open price
    float m_t0_bcr; //t0 buy price rate: increasing rate of open price
    CPosMaster *m_mpos;
    map<int/*begin date*/, CPosMaster*> m_mpos_map; //all master positions ever

public:
    CStock(int sn, string& name, int date, int count, int cost, \
            int cp, float t0_cpr,float t0_bpr, float t0_bcr);
    int get_sn() { return m_sn; }
    long long get_next_mposid(int date);
    void process_old_tpos(day_price_t& price);
    void run_t0(day_price_t& price, int policy);
};

int open_db(const char *db_path, sqlite3 **db);
int get_open_price(int stock_sn, int date);
int get_close_price(int stock_sn, int date);
void get_trade_days(int stock_sn, int begin_date, int end_date, vector<struct day_price_t>& trade_days);
void run_t0_simulator(int stock_sn, string& begin_date, string& end_date, int policy);

#endif
