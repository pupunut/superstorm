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
    void _get_trade_days(char *query, map<int, vector<struct day_price_t> >& trade_days);
public:
    CBackData(int type, char *path);
    ~CBackData();
    int get_open_price(int stock_sn, int date);
    int get_close_price(int stock_sn, int date);
    void get_trade_days(int stock_sn, int begin_date, int end_date, vector<struct day_price_t>& trade_days);
    void get_trade_days(int stock_sn, int begin_date, vector<struct day_price_t>& trade_days);
    int get_latest_date();
    int get_latest_range(int date, int range);
    int get_all_sn(vector<int/*sn*/> &snlist);
    int get_dp_desc(int sn, int begin_date, int end_date, vector<day_price_t> &dp_desc);
    int get_dp_desc(int begin, int end, map<int, vector<day_price_t> > &dp_desc);
};

#endif


