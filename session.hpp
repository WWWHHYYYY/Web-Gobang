#pragma once
#include "jsoncpp/json/json.h"
#include <string>
#include <vector>
#include <iostream>
#include <openssl/md5.h>
#include <string.h>
#include "httplib.h"
#include "tools.hpp"
#include <unordered_map>

using namespace std;
using namespace httplib;

class Session
{
    public:
        Session()
        {}
        Session(Json::Value v, int user_id)
        {
            real_str_.clear();
            
            // 字符串由用户邮箱和密码生成
            real_str_ += v["email"].asString();
            real_str_ += v["password"].asString();
            user_id_ = user_id;
        }
        ~Session()
        {}

        bool SumMd5()
        {
            // md5初始化
            MD5_CTX ctx;
            MD5_Init(&ctx);

            if(MD5_Update(&ctx, real_str_.c_str(), real_str_.size()) != 1)
                return false;
            
            unsigned char md5[16] = {0};
            if(MD5_Final(md5, &ctx) != 1)
                return false;

            char tmp[3] = {0};
            char buf[32] = {0};

            for(int i = 0; i < 16; ++i)
            {
                snprintf(tmp, sizeof(tmp) - 1, "%02x", md5[i]);
                // 追加拷贝
                strncat(buf, tmp, 2);
            }
            session_id_ = buf;
            return true;
        }

        string& GetSesseionId()
        {
            SumMd5();
            return session_id_;
        }

    public:
        string session_id_;

        // 生成会话id的原生字符串
        string real_str_;

        // 与会话id对应的user_id
        int user_id_;
};

//管理用户与会话信息 键值对对应
class AllSessionInfo
{
    public:
        AllSessionInfo()
        {
            session_map_.clear();
            pthread_mutex_init(&map_lock_, NULL);
        }

        ~AllSessionInfo()
        {
            pthread_mutex_destroy(&map_lock_);
        }

        void SetSessionInfo(string& session_id, Session& sess)
        {
            pthread_mutex_lock(&map_lock_);
            session_map_.insert(make_pair(session_id, sess));
            pthread_mutex_unlock(&map_lock_);
        }
        
        int GetSessionInfo(string& session_id, Session* session_info)
        {
            // session_info 出参，存储session信息
            pthread_mutex_lock(&map_lock_);
            auto iter = session_map_.find(session_id);
            if(iter == session_map_.end())
            {
                pthread_mutex_unlock(&map_lock_);
                return -1;
            }

            if(session_info != nullptr)
            {
                *session_info = iter->second;
            }

            pthread_mutex_unlock(&map_lock_);
            return 0;
        }

        int CheckSessionInfo(const Request& res)
        {
            string session_id =GetSessionId(res);
            if(session_id == "")
                return -1;
            
            Session sess;
            if(GetSessionInfo(session_id, &sess) < 0)
                return -2;

            return sess.user_id_;
        }

        string GetSessionId(const Request& res)
        {
            string session_kv = res.get_header_value("Cookie");
            
            // 字符串分割
            vector<string> v;
            StringTools::Split(session_kv, "=", &v);
            return v[v.size()-1]; 
        }

    private:
        unordered_map<string, Session> session_map_;
        pthread_mutex_t map_lock_;
};
