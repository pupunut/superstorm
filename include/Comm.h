#ifndef _COMM_H_
#define _COMM_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sqlite3.h>

#include <map>
#include <string>
#include <algorithm>

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

#define RET_END 100

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
    fprintf(stdout, "INFO: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stdout, __VA_ARGS__);\
} while (0)
#else
#define INFO(...) \
do {\
    fprintf(stdout, __VA_ARGS__);\
} while (0)
#endif

#define WARN(...) \
do {\
    fprintf(stderr, "WARN: %s: ", __PRETTY_FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
} while (0)

#define ERROR(...) \
do {\
    fprintf(stderr, "ERROR: %s:%d:", __PRETTY_FUNCTION__, __LINE__);\
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

class CStock;
class CPos;
class CPosMaster;
class CPosSlave;
class CPosSlaveG2;
class CBackData;

//no parameters and no return values
typedef void (CPosSlave::*ps_func)();
typedef void (CPosSlaveG2::*ps2_func)();
typedef void (CPosMaster::*pm_unc)();
typedef void (simulator_func)(CBackData *, int, int, int, int);

typedef enum{
    ENUM_NONE = 0,
    ENUM_MPOS,
    ENUM_TPOS,
    ENUM_T2POS,
    ENUM_POS_TYPE_SIZE
}pos_type_t;

typedef enum{
    ENUM_PP_NONE = 0,
    ENUM_PP_LOW_NRB,
    ENUM_PP_LOW_RB,
    ENUM_PP_LOW_GAP,
    ENUM_PP_LOW_MD,
    ENUM_PP_HIGH_NRB,
    ENUM_PP_HIGH_RB,
    ENUM_PP_HIGH_GAP,
    ENUM_PP_HIGH_MD,
    ENUM_PP_SIZE
}point_policy_t;

typedef enum{
    ENUM_PPC_NONE = 0,
    ENUM_PPC_3DROP,
    ENUM_PPC_SIZE
}point_policy_coarse_t;

typedef enum{
    ENUM_PT_NONE = 0,
    ENUM_PT_IN,
    ENUM_PT_OUT,
    ENUM_PT_SIZE
}point_type_t;

typedef enum{
    ENUM_SP_NONE = 1,
    ENUM_SP_3D,
    ENUM_SP_5D,
    ENUM_SP_10D,
    ENUM_SP_20D,
    ENUM_SP_3S,
    ENUM_SP_SIZE
}short_policy_t;

static const char *point_policy[] = {
    "NONE"
    "NRB",
    "RB",
    "GAP",
    "MD",
    "RB",
    "RB",
    "GAP",
    "MD"
};

enum{
    ENUM_BACKDATA_DB = 1
};

struct macd_t{
    int id;
    int sn;
    int date;
    int diff;
    int dea;
    int macd;
    macd_t(int sn, int date, int diff, int dea, int macd)
    {
        this->sn = sn;
        this->date = date;
        this->diff = diff;
        this->dea = dea;
        this->macd = macd;
    }

    macd_t(int id, int sn, int date, int diff, int dea, int macd)
    {
        this->id = id;
        this->sn = sn;
        this->date = date;
        this->diff = diff;
        this->dea = dea;
        this->macd = macd;
    }

};

struct ma_t{
    int id;
    int sn;
    int date;
    int pma;
    int avg;
    ma_t(int id, int sn, int date, int pma, int avg)
    {
        this->id = id;
        this->sn = sn;
        this->date = date;
        this->pma = pma;
        this->avg = avg;
    }

    ma_t(int sn, int date, int pma, int avg)
    {
        this->sn = sn;
        this->date = date;
        this->pma = pma;
        this->avg = avg;
    }

};

struct ema_t : public ma_t{
    ema_t(int sn, int date, int pma, int avg)
        :ma_t(sn, date, pma, avg)
    {
    }


};

struct mabp_t{
    int id;
    int sn;
    int date;
    int small_pma;
    int large_pma;
    mabp_t(int sn, int date, int small_pma, int large_pma)
    {
        this->sn = sn;
        this->date = date;
        this->small_pma = small_pma;
        this->large_pma = large_pma;
    }

    mabp_t(int id, int sn, int date, int small_pma, int large_pma)
    {
        this->id = id;
        this->sn = sn;
        this->date = date;
        this->small_pma = small_pma;
        this->large_pma = large_pma;
    }

};

struct day_price_t{
    int id;
    int date;
    int open;
    int high;
    int low;
    int close;
    int count;
    int total;
    //for macd
    int macd;

    day_price_t()
    {
    }

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

    day_price_t(int id, int date, int open, int high, int low, int close, int count, int total)
    {
        this->id = id;
        this->date = date;
        this->open = open;
        this->high = high;
        this->low = low;
        this->close = close;
        this->count = count;
        this->total = total;
    }

    void print(const char *msg)
    {
        fprintf(stderr, "%s: day price:%d, %d\n", msg, date, open);
    }

    int get_body() { return abs(close - open); }
    int get_range() { return high - low; }
    int get_top_tail() { return open > close ? high - open : high - close; }
    int get_bottom_tail() { return open > close ? close - low : open - low; }
    bool is_red() { return open < close; }
    bool is_cross() { return open == close; }
    bool is_blue() { return open > close; }
};

struct point_t{
    int id;
    int sn;
    int date;
    int price;
    int count;
    int curr_count;
    point_type_t type;
    point_policy_t fine;
    point_policy_coarse_t coarse;
    point_t () {}
    point_t (int sn, int date, int price, point_type_t type, point_policy_coarse_t coarse)
    {
        this->sn = sn;
        this->date = date;
        this->price = price;
        this->type = type;
        this->fine = ENUM_PP_NONE;
        this->coarse = coarse;
        count = curr_count = 0;
    }

    point_t (int sn, int date, int price, point_type_t type, point_policy_coarse_t coarse, point_policy_t fine)
    {
        this->sn = sn;
        this->date = date;
        this->price = price;
        this->type = type;
        this->coarse = coarse;
        this->fine = fine;
        count = curr_count = 0;
    }

    point_t (int id, int sn, int date, int price, int count, point_type_t type, point_policy_coarse_t coarse, point_policy_t fine)
    {
        this->id = id;
        this->sn = sn;
        this->date = date;
        this->price = price;
        this->type = type;
        this->coarse = coarse;
        this->fine = fine;
        this->count = count;
        this->curr_count = count;
    }


    bool operator < (const point_t& p) const {
        if (coarse == p.coarse){
            if (fine == p.fine)
                return sn < p.sn;
            else
                return fine < p.fine;
        }else {
            return coarse < p.coarse;
        }
    }

    void print(const char *msg)
    {
        fprintf(stderr, "%s: point, sn:%06d, p:%s, date:%d, open:%d, type:%d\n",
                msg, sn, point_policy[fine], date, price, type);
    }

};

struct short_point_t{
    int id; //for db
    point_t lp;
    day_price_t dp;
    int count;
    int price;
    int profit;
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

char *to_date(int date, char *buf);

int main_comm(int argc, char *argv[], simulator_func *func);

#endif //_COMM_H
