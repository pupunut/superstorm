#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <getopt.h>

#include <map>
#include <stack>
#include <string>

#include "t0.h"

using namespace std;

op_t::op_t(struct pos_t *owner, int date, int di, int count, int price)
{
    this->owner = owner;
    this->date = date;
    this->di = di;
    this->count = count;
    this->price = price;
    memcpy(p_uuid, owner->uuid, sizeof(uuid));

    uuid_generate(uuid);
    fee1 = price * count * FEE1_RATIO;
    if (di == DI_SELL)
        fee2 = price * count * FEE2_RATIO;
    else
        fee2 = 0;

    fee4 = fee3 = 0;
    fee = get_fee();
}

int op_t::get_fee()
{
    return (fee1 + fee2 + fee3 + fee4);
}

pos_t:: pos_t(stock_t *owner, int begin_date, int count, int cost, \
            float cpr, float t0_cpr, float t0_bpr, \
            float t0_bcr) //for main pos
{
    this->owner = owner;
    this->begin_date = begin_date;
    this->end_date = 0;
    init_count = curr_count = count;
    init_cost = curr_cost = cost;
    status = ST_OPEN;
    mpos_flag = 1;
    this->cpr = cpr;
    this->bpr = 0; //main pos MUST NOT buy... only sell and clear
    this->t0_cpr = t0_cpr;
    this->t0_bpr = t0_bpr;
    this->t0_bcr = t0_bcr;

    op_t *op = new op_t(this, begin_date, DI_BUY, count,  price, cost);
    ops[begin_date] = op;
    profit.fee = op->get_fee();
}

//t0 pos can only create by this interface
tpos_t *pos_t::create_tpos(day_price_t& dp)
{
    pos_t *t0 = new pos_t;
    t0->owner = owner;
    t0->begin_date = dp.date;
    t0->end_date = 0;
    t0->init_count = curr_count = (int)((float)count * t0_bcr);
    t0->init_cost = curr_cost = dp->price;
    t0->status = ST_OPEN;
    t0->mpos_flag = this.mpos_flag--;
    assert(t0->mpos_flag > -1); //a t0 pos can create a child t0 pos at most
    if (mpos_flag = 1){ //main pos
        t0->cpr = t0_cpr;
        t0->bpr = t0_bpr;
        t0->init_count = curr_count = (int)((float)count * t0_bcr);
    }else if (mpos_flag = 0){ //t0 pos
        t0->cpr = cpr;
        t0->bpr = bpr;
        t0->init_count = curr_count = this->count;
    }else
        assert(0);

    op_t *op = new op_t(begin_date, DI_BUY, cost);

    t0->ops[begin_date] = op;
    t0->profit.fee = op->get_fee();

    return t0;
}

void pos_t::process_tpos(day_price_t& price, bool can_create_new_tpos_flag)
{
    if (price.high > get_clear_price())
        clear_tpos();

    if (can_create_new_tpos_flag){
        if (price.low < get_buy_price())
            create_tpos(price);
    }
}

void pos_t::clear_tpos(int date, int price)
{
    assert(status == ST_OPEN);

    status = ST_CLOSE;
    end_date = date;
    curr_cost = price;

    op_t *op = new op_t(this, date, DI_SELL, curr_count, price);
    profit.fee += op->get_fee();
    ops[begin_date] = op;
    assert(init_count == curr_count); //we only support buy and sell whole pos once a time
    profit.float_profit = 0;
    profit.cash_profit = curr_count * (curr_cost - init_count);
    profit.total_profit = profit.cash_profit - profit.fee;
}

int pos_t::get_profit(int date)
{
    if (date >= begin_date && date <= end_date){
        int price = get_close_price(owner->sn, date);
        profit.float_profit = curr_count * (price - curr_cost) - profit.fee;
        return profit.float_profit;
    }else if (date > end_date) {
        profit.float_profit = 0;
        return profit.total_profit;
    }else
        assert(0); //do know what to do, panic
}

void pos_t::_summary(int date)
{
    get_profit(date);
    printf("\t,%d,%d,%d,%d,%d,%d,%d\n", \
            owner->sn, mpos_flag, status, begin_date, end_date, \
            profit.float_profit, profit.profit_total);
}

void pos_t::summay(int date)
{
    //print base info and profit info
    if (mpos_flag){// main pos
        printf("main pos, sn, flag, status, begin_date, end_date, float_profit, total_profit\n");
    }else {
        printf("tpos, sn, flag, status, begin_date, end_date, float_profit, total_profit\n");
    }

    get_profit(date);
    _summary(date);
}

int pos_t::get_clear_price()
{
    return (int)(((float)init_cost) * (1 + cpr));
}

int pos_t::get_buy_price()
{
    return (int)(((float)init_cost) * (1 - bpr));
}

stock_t::stock_t(int sn, string& name, int date, int count, int cost, \
            float cpr, float t0_cpr,float t0_bpr) //create main pos
{
    this->sn = sn;
    this->name = name;
    this->cpr = cpr;
    this->t0_cpr = t0_cpr;
    this->t0_bpr = t0_bpr;
    main_pos = new pos_t(date, count, cost, cpr);
    mpos[date] = main_pos;
}

void stock_t::process_old_tpos(day_price_t& price)
{
    foreach_itt(itt, &tpos){
        if (itt->second->begin_date < price.date)
            itt->second->process_tpos(price, false);
    }
}

void stock_t::run_t0(day_price_t& price, int policy)
{
    //process existed t0_pos
    mpos->process_old_tpos(price);

    //get current main pos and create new t0 pos
    pos_t *t0 = mpos->create_tpos(price);
    tpos[string(t0->uuid)] = t0;

    //process today's tpos, may be create new tpos
    t0->process_tpos(price, true);

    //day summary
    mpos->summary(price.date);
    foreach_itt(itt, &tpos){
        itt->second->summary(price.date);
    }
}

int close_db(sqlite3 *db)
{
    sqlite3_close(db);
    return 0;
}

int open_db(char *db_path, sqlite3 **db)
{
    if (sqlite3_open(db_path, db) != SQLITE_OK){
        fprintf(stderr, "Could not open database: %s\n", sqlite3_errmsg(*db));
        return -1;
    }

    //set some global parameters for db
    char *errstr = NULL;
    sqlite3_exec(db, "PRAGMA synchronous=OFF", NULL, NULL, &errstr);
    //sqlite3_exec(db, "PRAGMA synchronous=ON", NULL, NULL, &errstr);
    sqlite3_free(errstr);
    sqlite3_exec(db, "PRAGMA count_changes=OFF", NULL, NULL, &errstr);
    sqlite3_free(errstr);
    sqlite3_exec(db, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errstr);
    sqlite3_free(errstr);
    sqlite3_exec(db, "PRAGMA temp_store=MEMORY", NULL, NULL, &errstr);
    sqlite3_free(errstr);

    return 0;
}

int get_open_price(int stock_sn, int date)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT open FROM dayline where sn=%d and date='%d'", \
            stock_sn, date);
    assert(sqlite3_prepare_v2(cfg_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    return sqlite3_column_int(stmt, 0);
}

//vector<int/*date*/, struct day_price_t> trade_days;
void get_trade_days(stock_sn, begin_date, end_date, trade_days)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT * FROM dayline where sn=%d and date bwtween '%d' and '%d'", \
            stock_sn, begin_date, end_date);
    assert(sqlite3_prepare_v2(cfg_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);

    int num = 0;
    while(1){
        int i = sqlite3_step(stmt);
        if (i == SQLITE_ROW){
            struct day_price_t d(atoi(sqlite3_column_int(stmt, 1)), \
                    sqlite3_column_int(stmt, 2), \
                    sqlite3_column_int(stmt, 3), \
                    sqlite3_column_int(stmt, 4), \
                    sqlite3_column_int(stmt, 5), \
                    sqlite3_column_int(stmt, 6), \
                    sqlite3_column_int(stmt, 7));

            trade_days.push_back(d);
        }else if(i == SQLITE_DONE){
            break;
        }else {
            fprintf(stderr, "SQL Failed.\n");
            assert(0);
        }
    }

    sqlite3_finalize(stmt);
}

void run_t0_simulator(int stock_sn, string& begin_date, string& end_date, int policy)
{
    //create_main_pos
    int cost = get_open_price(stock_sn, begin_date);
    if (cost <= 0){
        fprintf(stderr, "FATAL: raw stock cost should no <= 0, but we get:%d\n", cost);
        assert(0);
    }

    stock_t stock(stock_sn, string("unknown"), begin_date, COUNT_INIT_MAIN, cost);


    //from begin_date to end_date run_t0_operation day by day
    vector<int/*date*/, struct day_price_t> trade_days;
    get_trade_days(stock_sn, begin_date, end_date, trade_days);

    if (trade_days.count() > 0)
        trade_days.erase (trade_days.begin()); //T0 from the second day of creating main pos
    printf(stderr, "There are %d trade days between %d and %d\n", trade_days.count(), begin_date, end_date);

    if (!trade_days.count()){
        printf(stderr, "No trade_days, done.\n");
        return;
    }

    foreach(itt, trade_days){
        stock.run_t0(itt->second, policy);
    }
}

/* Flag set by ‘--verbose’. */
int verbose_flag;
sqlite3 *g_db = NULL;

int main (int argc, char **argv)
{
    string begin_date, end_date, db_path;
    int stock_sn, policy;

    int c;

    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"stock",  required_argument, 0, 's'},
            {"begin",  required_argument, 0, 'b'},
            {"end",  required_argument, 0, 'e'},
            {"policy",    required_argument, 0, 'p'},
            {"db",    required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "b:e:p:",
                long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 'b':
                printf ("begin date: %s\n", optarg);
                begin_date.assign(optarg);
                break;

            case 'd':
                printf ("database: %s\n", optarg);
                db_path.assign(optarg);
                break;

            case 'e':
                printf ("end date: %s\n", optarg);
                end_date.assign(optarg);
                break;

            case 'p':
                printf ("policy is: %s\n", optarg);
                break;

            case 's':
                printf("stock is: %s\n", optarg);
                stock_sn = atoi(optarg);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }
    }

    /* Instead of reporting ‘--verbose’
       and ‘--brief’ as they are encountered,
       we report the final status resulting from them. */
    if (verbose_flag)
        puts ("verbose flag is set");

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    //OK , let's begin

    if (open_db(db_path, &g_db)){
        fprintf(stderr, "Cannot open db:%s\n", db_path);
        exit(EXIT_FAILURE);
    }

    int ret = run_t0_simulator(stock_sn, begin_date, end_date, policy);

    return ret;
}

