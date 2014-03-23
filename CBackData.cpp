#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <uuid/uuid.h>
#include <sqlite3.h>

#include <map>
#include <vector>
#include <string>

#include "CBackData.h"
#include "Comm.h"

using namespace std;

#define SQL_ASSERT(a) \
do { \
    if (!(a)) {\
        THROW("sql errmsg: %s\n", sqlite3_errmsg(m_db));\
    }\
} while (0)


int open_db(const char *m_path, sqlite3 **db)
{
    if (sqlite3_open(m_path, db) != SQLITE_OK){
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

int close_db(sqlite3 *db)
{
    sqlite3_close(db);
    return 0;
}


CBackData::CBackData(int type, char *path)
{
    THROW_ASSERT(type == ENUM_BACKDATA_DB);

    if (open_db(path, &m_db))
        ASSERT("Cannot open db:%s\n", m_path.c_str());

    m_type = type;
    m_path.assign(path);
}

CBackData::~CBackData()
{
    if (m_type == ENUM_BACKDATA_DB)
        close_db(m_db);
}

int CBackData::get_open_price(int stock_sn, int date)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT open FROM dayline where sn=%d and date='%d'", \
            stock_sn, date);
    SQL_ASSERT(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    SQL_ASSERT(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    int price =  sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return price;
}

int CBackData::get_close_price(int stock_sn, int date)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT close FROM dayline where sn=%d and date='%d'", \
            stock_sn, date);
    SQL_ASSERT(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    SQL_ASSERT(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    int price =  sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return price;
}

//vector<struct day_price_t> trade_days;
void CBackData::_get_trade_days(char *query, vector<struct day_price_t>& trade_days)
{
    sqlite3_stmt* stmt;
    SQL_ASSERT(sqlite3_prepare_v2(m_db, query, strlen(query), &stmt, NULL) == SQLITE_OK);

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
            ASSERT("SQL Failed.\n");
        }
    }

    sqlite3_finalize(stmt);
}

//map<int, vector<struct day_price_t>> trade_days;
void CBackData::_get_trade_days(char *query, map<int, vector<struct day_price_t> > &trade_days)
{
    sqlite3_stmt* stmt;
    SQL_ASSERT(sqlite3_prepare_v2(m_db, query, strlen(query), &stmt, NULL) == SQLITE_OK);

    int num = 0;
    while(1){
        int i = sqlite3_step(stmt);
        if (i == SQLITE_ROW){
            int sn = sqlite3_column_int(stmt, 0);
            struct day_price_t d(atoi((const char *)sqlite3_column_text(stmt, 1)), \
                    sqlite3_column_int(stmt, 2), \
                    sqlite3_column_int(stmt, 3), \
                    sqlite3_column_int(stmt, 4), \
                    sqlite3_column_int(stmt, 5), \
                    sqlite3_column_int(stmt, 6), \
                    sqlite3_column_int(stmt, 7));

            trade_days[sn].push_back(d);
        }else if(i == SQLITE_DONE){
            break;
        }else {
            ASSERT("SQL Failed.\n");
        }
    }

    sqlite3_finalize(stmt);
}



//vector<struct day_price_t> trade_days;
void CBackData::get_trade_days(int stock_sn, int begin_date, int end_date, vector<struct day_price_t>& trade_days)
{
    char buf[4096];
    snprintf(buf, sizeof(buf), "SELECT * FROM dayline where sn=%d and date between '%d' and '%d'", \
            stock_sn, begin_date, end_date);
    _get_trade_days(buf, trade_days);
}

void CBackData::get_trade_days(int stock_sn, int begin_date, vector<struct day_price_t>& trade_days)
{
    char buf[4096];

    snprintf(buf, sizeof(buf), "SELECT * FROM dayline where sn=%d and date >= '%d'", \
            stock_sn, begin_date);

    _get_trade_days(buf, trade_days);
}

int CBackData::get_latest_date()
{
    char buf[4096];
    sqlite3_stmt* stmt;
    snprintf(buf, sizeof(buf), "SELECT max(date) FROM dayline");
    SQL_ASSERT(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    SQL_ASSERT(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    int date = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return date;
}

int CBackData::get_latest_range(int date, int range)
{
    char buf[4096];
    sqlite3_stmt* stmt;
    snprintf(buf, sizeof(buf), "SELECT min(date) FROM (SELECT distinct(date) FROM dayline WHERE date < '%d' ORDER BY date DESC LIMIT %d)", \
            date, range);
    SQL_ASSERT(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    SQL_ASSERT(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    int begin_date = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return begin_date;
}


int CBackData::get_all_sn(vector<int/*sn*/> &snlist)
{
    char buf[4096];
    sqlite3_stmt* stmt;
    snprintf(buf, sizeof(buf), "SELECT DISTINCT sn FROM dayline");
    SQL_ASSERT(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);

    int num = 0;
    while(1){
        int i = sqlite3_step(stmt);
        if (i == SQLITE_ROW){
            snlist.push_back(sqlite3_column_int(stmt, 0));
        }else if(i == SQLITE_DONE){
            break;
        }else {
            ASSERT("SQL Failed.\n");
        }
    }

    sqlite3_finalize(stmt);
    return 0;
}

int CBackData::get_dp_desc(int sn, int begin, int end, vector<day_price_t> &dp_desc)
{
    char buf[4096];
    snprintf(buf, sizeof(buf), "SELECT * FROM dayline where sn=%d and date between '%d' and '%d' ORDER BY date DESC", \
            sn, begin, end);
    _get_trade_days(buf, dp_desc);

    return 0;
}

int CBackData::get_dp_desc(int begin, int end, map<int, vector<day_price_t> > &dp_desc)
{
    char buf[4096];
    snprintf(buf, sizeof(buf), "SELECT * FROM dayline where date between '%d' and '%d' ORDER BY date DESC", \
            begin, end);
    _get_trade_days(buf, dp_desc);

    return 0;
}

