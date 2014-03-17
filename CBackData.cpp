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


CBackData::CBackData(int type, string& path)
{
    assert(m_type == ENUM_BACKDATA_DB);

    if (open_db(m_path.c_str(), &m_db)){
        fprintf(stderr, "Cannot open db:%s\n", m_path.c_str());
        assert(0);
    }

    m_path = path;
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
    assert(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    return sqlite3_column_int(stmt, 0);
}

int CBackData::get_close_price(int stock_sn, int date)
{
    char buf[4096];
    sqlite3_stmt* stmt;

    snprintf(buf, sizeof(buf), "SELECT close FROM dayline where sn=%d and date='%d'", \
            stock_sn, date);
    assert(sqlite3_prepare_v2(m_db, buf, strlen(buf), &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW); //should be only one row

    return sqlite3_column_int(stmt, 0);
}

//vector<struct day_price_t> trade_days;
void CBackData::_get_trade_days(char *query, vector<struct day_price_t>& trade_days)
{
    sqlite3_stmt* stmt;
    assert(sqlite3_prepare_v2(m_db, query, strlen(query), &stmt, NULL) == SQLITE_OK);

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
