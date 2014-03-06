#ifndef _COMM_H_
#define _COMM_H_

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

class CStock;
class CPos;
class CPosMaster;
class CPosSlave;
class CPosSlaveG2;

//no parameters and no return values
typedef void (CPosSlave::*ps_func)();
typedef void (CPosSlaveG2::*ps2_func)();
typedef void (CPosMaster::*pm_unc)();

typedef enum{
    ENUM_MPOS = 1,
    ENUM_TPOS,
    ENUM_T2POS,
    ENUM_POS_TYPE_SIZE
}pos_type_t;

enum{
    ENUM_BACKDATA_DB = 1
};

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


#endif //_COMM_H
