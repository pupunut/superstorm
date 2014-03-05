#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>

#include "t0.h"

using namespace std;

static sqlite3 *g_db;

COperation::COperation(CPos *owner, int date, int di, int count, int price)
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

int COperation::get_fee()
{
    return (m_fee1 + m_fee2 + m_fee3 + m_fee4);
}

CPos:: CPos(CStock *owner, int begin_date, int count, int cost, int clear_price)
{
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

    int price = get_close_price(m_owner->get_sn(), date);
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

CPosMaster::CPosMaster(CStock *owner, int begin_date, int count, int cost, int  clear_price)
:CPos(owner, begin_date, count, cost, clear_price)
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

    return _clear_pos(date, price);
}

CPosSlave *CPosMaster::create_tpos(day_price_t& dp)
{
    CPosSlave *t0 = new CPosSlave(m_owner, this, dp.date, \
            get_t0_bc(), dp.open, get_t0_cp(dp.open));

    return t0;
}

void CPosMaster::clear_tpos(day_price_t& dp, map<int/*date*/, CPosSlave * /*tpos*/> *tpos_map)
{
    foreach_itt(itt, tpos_map){
        CPosSlave *t0 = itt->second;
        int date = itt->first;

        //skip invalid tpos
        if (t0->get_status() != ST_OPEN)
            continue;
        if (dp.high < t0->get_clear_price())
            continue;

        //yes, TDAY can clear tpos, because it is tpos...
        if (dp.date >= date)
            t0->clear_pos(dp.date, t0->get_clear_price());
    }
}

void CPosMaster::process_tpos(day_price_t& dp)
{
    //create a t0 first
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

    sprintf(id, "%02lu", (m_tpos.size() + m_tpos_g2.size() + 1U));

    return atoll(pos_id);
}


CPosSlave::CPosSlave(CPosMaster *mp, day_price_t& dp) //for create t0
:CPos(mp->get_owner(), dp.date, mp->get_t0_bc(), dp.open, mp->get_t0_cp(dp.open))
{
    m_id = mp->get_next_tposid(dp.date);
    m_type = ENUM_TPOS;
    m_holder = mp;
}

CPosSlave::CPosSlave(CStock *owner, CPosMaster *holder, int begin_date, int count, int cost, int clear_price)
:CPos(owner, begin_date, count, cost, clear_price)
{
    m_id = holder->get_next_tposid(begin_date);
    m_type = ENUM_TPOS;
    m_holder = holder;
}

CPosSlave *CPosSlave::create_tpos(day_price_t& dp)
{
    CPosSlave *t0 = new CPosSlaveG2(this, dp);

    return t0;
}

CPosSlaveG2::CPosSlaveG2(CPosSlave *sp, day_price_t& dp) //for create t0
    :CPosSlave(sp->get_owner(), sp->get_holder(), dp.date, sp->get_t0_bc(), sp->get_t0_bp(dp.open), dp.open)
{
    m_type = ENUM_T2POS;
}

CStock::CStock(int sn, string& name, int date, int count, int cost, \
            int cp, float t0_cpr,float t0_bpr, float t0_bcr)
{
    m_sn = sn;
    m_name = name;
    m_cp = cp;
    m_t0_cpr = t0_cpr;
    m_t0_bpr = t0_bpr;
    m_t0_bcr = t0_bcr;

    m_mpos = new CPosMaster(this, date, count, cost, cost*2);
    m_mpos_map[date] = m_mpos;
}

long long CStock::get_next_mposid(int date)
{
    char pos_id[1024];
    char *id = pos_id;

    memset(id, 0, sizeof(id));
    sprintf(id, "%d", date - 20000000);
    id += strlen(id);
    sprintf(id, "%02lu", m_mpos_map.size() + 1);
    return atoll(pos_id);
}

void CStock::run_t0(day_price_t& dp, int policy)
{
    //process tpos till dp.date, including
    //1. clear existed tpos if possible
    //2. clear existed tpos_g2 if possible
    //3. create new tpos_g2 in the dp.date if possible
    m_mpos->process_tpos(dp);

    //day summary
    m_mpos->print_profit(dp.date);
}

int close_db(sqlite3 *db)
{
    sqlite3_close(db);
    return 0;
}

int open_db(const char *db_path, sqlite3 **db)
{
    if (sqlite3_open(db_path, db) != SQLITE_OK){
        fprintf(stderr, "Could not open database: %s\n", sqlite3_errmsg(*db));
        return -1;
    }

    //set some global parameters for db
    char *errstr = NULL;
    sqlite3_exec(*db, "PRAGMA synchronous=OFF", NULL, NULL, &errstr);
    //sqlite3_exec(*db, "PRAGMA synchronous=ON", NULL, NULL, &errstr);
    sqlite3_free(errstr);
    sqlite3_exec(*db, "PRAGMA count_changes=OFF", NULL, NULL, &errstr);
    sqlite3_free(errstr);
    sqlite3_exec(*db, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errstr);
    sqlite3_free(errstr);
    sqlite3_exec(*db, "PRAGMA temp_store=MEMORY", NULL, NULL, &errstr);
    sqlite3_free(errstr);

    return 0;
}

int get_open_price(int stock_sn, int date)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT open FROM dayline where sn=%d and date='%d'", \
            stock_sn, date);
    assert(sqlite3_prepare_v2(g_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    return sqlite3_column_int(stmt, 0);
}

int get_close_price(int stock_sn, int date)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT close FROM dayline where sn=%d and date='%d'", \
            stock_sn, date);
    assert(sqlite3_prepare_v2(g_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    return sqlite3_column_int(stmt, 0);
}


//vector<struct day_price_t> trade_days;
void get_trade_days(int stock_sn, int begin_date, int end_date, vector<struct day_price_t>& trade_days)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT * FROM dayline where sn=%d and date between '%d' and '%d'", \
            stock_sn, begin_date, end_date);
    assert(sqlite3_prepare_v2(g_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);

    int num = 0;
    while(1){
        int i = sqlite3_step(stmt);
        if (i == SQLITE_ROW){
            struct day_price_t d(atoi((const char *)sqlite3_column_text(stmt, 1)), \
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

void run_t0_simulator(int stock_sn, int begin_date, int end_date, int policy)
{
    //create_main_pos
    int cost = get_open_price(stock_sn, begin_date);
    if (cost <= 0){
        fprintf(stderr, "FATAL: raw stock cost should no <= 0, but we get:%d\n", cost);
        assert(0);
    }

    string name("unknown");
    CStock stock(stock_sn, name, begin_date, COUNT_INIT_MAIN, cost, \
            cost*2, 1.05f, 0.95f, 0.5f);


    //from begin_date to end_date run_t0_operation day by day
    vector<struct day_price_t> trade_days;
    get_trade_days(stock_sn, begin_date, end_date, trade_days);

    if (trade_days.size() > 0)
        trade_days.erase (trade_days.begin()); //T0 from the second day of creating main pos
    fprintf(stderr, "There are %lu trade days between %d and %d\n", trade_days.size(), begin_date, end_date);

    if (!trade_days.size()){
        fprintf(stderr, "No trade_days, done.\n");
        return;
    }

    foreach_itt(itt, &trade_days){
        stock.run_t0(*itt, policy);
    }
}

/* Flag set by ‘--verbose’. */
int verbose_flag;

int main (int argc, char **argv)
{
    string db_path;
    int stock_sn, begin_date, end_date, policy;

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

        c = getopt_long (argc, argv, "s:b:e:p:d:",
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
                begin_date = atoi(optarg);
                break;

            case 'd':
                printf ("database: %s\n", optarg);
                db_path.assign(optarg);
                break;

            case 'e':
                printf ("end date: %s\n", optarg);
                end_date = atoi(optarg);
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

    if (open_db(db_path.c_str(), &g_db)){
        fprintf(stderr, "Cannot open db:%s\n", db_path.c_str());
        exit(EXIT_FAILURE);
    }

    run_t0_simulator(stock_sn, begin_date, end_date, policy);

    return 0;
}

