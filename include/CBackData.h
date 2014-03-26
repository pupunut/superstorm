#ifndef _CBACK_DATA_H_
#define _CBACK_DATA_H_

#include <string>
#include "Comm.h"

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
    int get_latest_date(int date);
    int get_latest_range(int date, int range);
    int get_prev_date(int date);
    void get_all_sn(vector<int/*sn*/> &snlist);
    void get_point_sn(map<int/*sn*/, vector<point_t> > &sn_list);
    void get_day_line(int sn, map<int/*date*/, day_price_t/*dp*/> &dplist);
    void get_dp_desc(int sn, int begin_date, int end_date, vector<day_price_t> &dp_desc);
    void get_dp_desc(int begin, int end, map<int, vector<day_price_t> > &dp_desc);
    void clear_point(point_t *p, day_price_t *dp, int policy);
    void short_point(point_t *p, day_price_t *dp, int policy, int count);
    void dump_point(point_t *p);
    void dump_bpn(point_t *p);
    void reset_sp(point_t *p, int policy);
    void reset_point();
    void reset_bpn();
    void reset_sp();
    void create_view_sp();
    void merge_sp_aux();
    void reset_sp_aux(point_t *p, int policy);
    bool is_valid_date(int date);
};

#endif


