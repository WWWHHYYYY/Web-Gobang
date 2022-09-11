#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>
#include <string.h>
#include <iostream>
#include "room_player.hpp"

#define JUDGE_VALUE(P) (P!=NULL ? P:"")
using namespace std;

class DataBaseSvr
{   
    public:
        DataBaseSvr()
        {};
        ~DataBaseSvr()
        {
            mysql_close(&mysql_);
        };
        
        bool Connect2Mysql()
        {
            if(!mysql_real_connect(&mysql_, "0.0.0.0", "root", "85861076", "web-gobang", 3306, NULL,CLIENT_FOUND_ROWS))
                return false;
            return true;
        }
        
        bool Add_User(Json::Value v)
        {
            string name = v["name"].asString();
            string password = v["passwd"].asString();
            string email = v["email"].asString();
            string phone_num = v["phonenum"].asString();
#define ADD_USER "insert into sys_user (name, email, phone_num, passwd) values(\'%s\', \'%s\', \'%s\', \'%s\');"
            char sql[1024] = {0};
            snprintf(sql, sizeof(sql) - 1, ADD_USER, name.c_str(), email.c_str(), phone_num.c_str(), password.c_str());
            cout << sql << endl;

            return ExecuteSql(sql);
        }

        int QueryUserExist(Json::Value v)
        {
            // 1. 组织sql语句
            string email = v["email"].asString();
            string password = v["password"].asString();

#define GET_USER_INFO "select * from sys_user where email=\'%s\';"
            char sql[1024] = {0};
            snprintf(sql, sizeof(sql) - 1, GET_USER_INFO, email.c_str());

            // 2. 执行查询语句
            MYSQL_RES *res;
            if(ExecuteSql(sql, &res) == false)
                return -2;
            
            // 3.针对查询结果进行处理
            if(mysql_num_rows(res) != 1)
                return -3;
            
            // 获取结果集第一行数据
            MYSQL_ROW row = mysql_fetch_row(res);
            string passwd_db = JUDGE_VALUE(row[4]);
            
            if(password != passwd_db)
                return -4;

            mysql_free_result(res);
            return atoi(row[0]);
        }
        
        int InsertRoomInfo(const Room& r)
        {
              /*
               *插入用户信息到数据库，用到事务
               * */
              if(ExecuteSql("start transaction;") == false)
              {
                  return -1;
              }

              if(ExecuteSql("savepoint start_insert;") == false)
              {
                  return -2;
              }
#define INSERT_INTO_INFO "insert into game_info values(%d, %d, %d, %d, %d, '\%s\');"
              for(int i = 0 ; i < r.step_num_; ++i)
              {
                  char sql[1024] = {0};
                  snprintf(sql, sizeof(sql) - 1, INSERT_INTO_INFO, r.room_id_, r.p1_, r.p2_,
                      r.winner_, r.p1_, r.step_vec_[i].c_str());
                  if(ExecuteSql(sql) == false)
                  {
                      ExecuteSql("rollback to start_insert;");
                      ExecuteSql("commit;");
                      return -3;
                  }

              }
              ExecuteSql("commit;");
              return 0;

        }

        bool ExecuteSql(const string &sql, MYSQL_RES **res)
        {
            // 1.设置客户端字符集
            mysql_query(&mysql_,"set names utf8");

            if(mysql_query(&mysql_, sql.c_str()) != 0)
                return false;
            
            // mysql执行结果放在结果集中
            *res = mysql_store_result(&mysql_);
            if(res == NULL)
                return false;
            return true;
        }

        bool ExecuteSql(const string &sql)
        {
            // 1.设置客户端字符集
            mysql_query(&mysql_,"set names utf8");
            if(mysql_query(&mysql_, sql.c_str()) != 0)
                return false;
            return true;
        }



    private:
        MYSQL mysql_;
};
