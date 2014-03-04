#if !defined(_T0_H_)                                           |            m_layout.m_disk_layout.dmeta_size, \
#define _T0_H_

#define ZERO_PRICE_RATE 0.001
#define FEE1_RATIO 0.0008
#define FEE2_RATIO 0.001
#define FEE3_RATIO 0.1

#define COUNT_INIT_MAIN 10000
#define COUNT_INIT_T0 2 //T0 pos, how many stocks against main pos to operate
#define COUNT_T0_S1 2 //first step, how many stocks against T0_pos to operate
#define COUNT_T0_S2 2 //second step, hwo many stocks against T0_pos to operate

#define CONCATNATE(a, b) a##b
#define iterator_of(item) CONCATNATE(item, _itt)


#define foreach_itt(itt, container)\
    for (typeof((container)->begin()) itt = (container)->begin();\
            itt != (container)->end(); itt++)

#define foreach(item, container)\
    for (typeof((container)->begin()) iterator_of(item) = (container)->begin();\
            iterator_of(item) != (container)->end() && ({item = *iterator_of(item); true;});\
            iterator_of(item)++)

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

struct op_t
{
    char uuid[16];
    char p_uuid[16]; //parent pos
    struct pos_t *owner;
    int date;
    int di;
    int count;
    int price;
    int fee1; //交易佣金
    int fee2; //印花税，单向
    int fee3; //利息
    int fee4; //其他
    int fee; //the cost of this ops

    op_t(struct pos_t *owner, int date, int di, int count, int price);
    int get_fee();
};

struct profit_t{
    int fee;
    int float_profit;
    int cash_profit;
    int total_profit;

    profit_t()
    {
        fee = 0;
        float_profit = 0;
        cash_profit = 0;
        total_profit = 0;
    }
};

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
struct pos_t
{
    stoct_t *owner;
    char uuid[16];
    char p_uuid[16]; //main pos
    int mpos_flag;
    int begin_date;
    int end_date;
    int status;
    int init_count;
    int init_cost;
    int curr_count;
    int curr_cost;
    float cpr; //clear price rate: increasing rate of open price
    float t0_cpr; //t0 clear price rate: increasing rate of open price
    float t0_bpr; //t0 buy price rate: increasing rate of open price
    float t0_bcr; //t0 buy count rate;
    struct profilt_t profit; //accumulative profit
    map<int/*date*/, op_t* /*operation*/> ops;

    pos_t() { }
    pos_t(stock_t *owner, int begin_date, int count, int cost, \
            float cpr, float t0_cpr, float t0_bpr, \
            float t0_bcr); //for main pos
    //t0 pos can only create by this interface
    tpos_t *create_tpos(day_price_t& dp);
    void process_tpos(day_price_t& price, bool can_create_new_tpos_flag);
    void clear_tpos(int date, int price);
    int get_profit(int date);
    void _summary(int date);
    void summay(int date);
    int get_clear_price();
    int get_buy_price();
};

struct stock_t
{
    int sn;
    string name;
    float cpr; //clear price rate: increasing rate of open price
    float t0_cpr; //t0 clear price rate: increasing rate of open price
    float t0_bpr; //t0 buy price rate: increasing rate of open price
    pos_t *mpos;
    map<int/*begin date*/, pos_t* /*main pos ever*/> mpos;
    map<string /*mpos uuid*/, pos_t* /*t0 pos*/> tpos;

    stock_t(int sn, string& name, int date, int count, int cost, \
            float cpr, float t0_cpr,float t0_bpr); //create main pos
    void process_old_tpos(day_price_t& price);
    void run_t0(day_price_t& price, int policy);
};


#endif
