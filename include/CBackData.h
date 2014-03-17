#ifndef _CBACK_DATA_H_
#define _CBACK_DATA_H_

#include <string>

using namespace std;

class CBackData {
private:
    sqlite3 *m_db;
    string m_path;
    int m_type; //file or db

    void _get_trade_days(char *query, vector<struct day_price_t>& trade_days);
public:
    CBackData(int type, string& path);
    ~CBackData();
    int get_open_price(int stock_sn, int date);
    int get_close_price(int stock_sn, int date);
    void get_trade_days(int stock_sn, int begin_date, int end_date, vector<struct day_price_t>& trade_days);
    void get_trade_days(int stock_sn, int begin_date, vector<struct day_price_t>& trade_days);
};

#endif


