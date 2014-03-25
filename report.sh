sqlite3 day.db << EOF
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
