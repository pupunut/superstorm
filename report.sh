#! /bin/bash

db=/mnt/hgfs/dayline/20140325/dayline.db
./find_low -d $db -t'20140325' -a'20140225' && ./eval_bp -d $db

sqlite3 $db << EOF
.header on
.mode column
select * from view_sp where pp<0 and fine>0 order by pp ASC limit 50;
select * from view_sp where pp>100 and fine>0 order by pp DESC limit 50;
select * from point where date = max(date) and fine>0;
EOF
exit 0

program=$(basename $0)
if [ $# != 1 ]; then
 	echo "Usage: $program day_data_dir"
	exit 1
fi

dbdir=$1
db=$dbdir/dayline.db

sqlite3 $db << EOF
drop table if exists report;
create table report(total integer, up integer, succ integer, coarse integer, fine integer, policy integer, avgp integer, maxp integer, minp integer, avgp1 integer);
insert into report select count(*), 0, 0, coarse, fine, policy, cast(avg(pp) as integer), max(pp), min(pp), 0 from view_sp where fine>0 group by coarse, fine, policy;
update report set up=(select count(*) from view_sp where coarse=report.coarse and fine=report.fine and policy=report.policy and pp > 0 group by
coarse,fine,policy);
update report set avgp1=(select cast(avg(pp) as integer) from view_sp where coarse=report.coarse and fine=report.fine and policy=report.policy and pp > 0 group by coarse,fine,policy);
update report set succ=up*100/total;
.header on
.mode column
select * from report;
EOF
