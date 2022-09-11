#pragma once
#include <string>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <jsoncpp/json/json.h>
#include "httplib.h"
#include "database.hpp"
#include "session.hpp"
#include "room_player.hpp"

using namespace std;
using namespace httplib;

class WebGobang
{
    public:
        WebGobang()
        {
            db_svr_ = NULL;
            all_sess_ = NULL;
            pm_ = NULL;
            rm_ = NULL;
        }

        ~WebGobang()
        {
            if(db_svr_)
            {
                delete db_svr_;
                db_svr_ = NULL;
            }
            if(all_sess_)
            {
                delete all_sess_;
                all_sess_ = NULL;
            }
            if(pm_)
            {
                delete pm_;
                pm_ = NULL;
            }
            if(rm_)
            {
                delete rm_;
                rm_ = NULL;
            }
        }

        int Init()
        {
            db_svr_ = new DataBaseSvr();

            if(db_svr_ == NULL)
                return -1;

            if(db_svr_->Connect2Mysql() == false)
            {   
                cout<<"mysql_connect"<<endl;
                return -2;
            }

            all_sess_ = new AllSessionInfo();
            if(all_sess_  == NULL)
            {
                return -3;
            }
            
            pm_ = new PlayerManager();
            if(pm_ == NULL)
            {
                return -4;
            }
            
            rm_ = new RoomManager();
            if(rm_ == NULL)
            {
                return -5;
            }

            return 0;
        }
        
        string Serialize(Json::Value v)
        {
            Json::FastWriter w;
            return w.write(v);
        }

        void StartWebGobang()
        {
          http_svr_.Post("/register", [this](const Request &res, Response &resp){
                   //校验账号和密码是否已存在
                  Json::Reader r;
                  Json::Value v;
                  r.parse(res.body, v);
                  
                  cout << "register" << endl;

                  Json::Value resp_json;
                  resp_json["status"] = this->db_svr_->Add_User(v);
                  
                  resp.body = Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");

              });
          
          http_svr_.Post("/login", [this](const Request &res, Response &resp){
                  // 校验登录信息
                  Json::Reader r;
                  Json::Value v;
                  r.parse(res.body, v);

                  Json::Value resp_json;
                  int user_id = this->db_svr_->QueryUserExist(v);
                  string tmp = "";
                  //根据用户信息->添加会话信息
                  if(user_id > 0)
                  {
                      Session sess(v, user_id); 
                      string session_id =  sess.GetSesseionId();
                      tmp = "JSESSIONID=" + session_id;
                      this->all_sess_->SetSessionInfo(session_id, sess);
                      this->pm_->InsertPlayer2Map(user_id);
                  }
                  resp_json["status"] = user_id > 0 ? true:false;
                  resp.body = Serialize(resp_json);
                  resp.set_header("Set-Cookie", tmp.c_str());
                  resp.set_header("Content-Type", "application/json");
                
              });

          // 会话校验
          http_svr_.Get("/GetUserId", [this](const Request &res, Response &resp)
          {
                  Json::Value resp_json;                                   
                  resp_json["user_id"] = this->all_sess_->CheckSessionInfo(res);
                  resp.body = Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
                  });

          // 匹配,将用户放到匹配池中
          http_svr_.Get("/SetMatch", [this](const Request &res, Response &resp)
          {       
                  cout << "set_match" <<endl;
                  Json::Value resp_json;                                   
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  if(user_id > 0)
                  {
                      resp_json["status"] = this->PushPlayer2MatchPool(user_id);
                  }
                  else
                      resp_json["status"] = -1;
                  
                  resp.body = Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
                  });
          
          http_svr_.Get("/Match", [this](const Request &res, Response &resp)
          {       
                  Json::Value resp_json;                                   
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  cout << user_id << ":match" << endl;
                  if(user_id < 0)
                  {
                      resp_json["status"] = -1;
                  }
                  else
                  {
                      Player r = this->pm_->GetPlayerInfo(user_id);
                      cout << r.room_id_ << r.player_status_ << r.chess_name_ << endl;

                      if(r.room_id_ >= 10000 && r.player_status_ == PLAYING)
                      {
                          // 匹配成功
                          resp_json["room_id"] = r.room_id_;
                          resp_json["chess_name"] = r.chess_name_;
                          resp_json["status"] = 1;
                      }
                      else
                      {
                          resp_json["status"] = 0;
                      }
                  }
                  resp.body = Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
          });

          http_svr_.Post("/IsMyTurn", [this](const Request &res, Response &resp)
          {
                  Json::Value resp_json;                                   
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  if(user_id < 0)
                  {
                      resp_json["status"] = -1;
                  }
                  else
                  {
                      Json::Reader r;
                      Json::Value v;
                      r.parse(res.body, v);
                      
                      int room_id = v["room_id"].asInt();
                      int user_id = v["user_id"].asInt();
                      resp_json["status"] = this->rm_->isMyTurn(user_id, room_id);
                  }
                  resp.body = Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
                  
                  });
          http_svr_.Post("/Step", [this](const Request &res, Response &resp)
          {
                  Json::Value resp_json;                                   
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  if(user_id < 0)
                  {
                      resp_json["status"] = -1;
                  }
                  else
                  {
                      resp_json["status"] = this->rm_->Step(res.body);
                  }

                  resp.body = Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
                  });
          http_svr_.Post("/GetPeerStep", [this](const Request &res, Response &resp)
          {
                  Json::Value resp_json;                                   
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  if(user_id < 0)
                  {
                      resp_json["status"] = -1;
                  }
                  else
                  {
                      string peer_step;
                      if(this->rm_->GetRoomStep(res.body, peer_step) < 0)
                      {
                          resp_json["status"] = 0;
                      }
                      else
                      {
                          resp_json["status"] = 1;
                          resp_json["peer_step"] = peer_step;
                      }
                      resp.body = this->Serialize(resp_json);
                      resp.set_header("Content-Type", "application/json");
                  }
                  });

          http_svr_.Post("/Winner", [this](const Request &res, Response &resp)
          {
              
                  Json::Value resp_json;          
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  cout << "winner: " << user_id << endl;
                  if(user_id <= 0)
                  {
                      resp_json["status"] = -1;
                  }
                  else
                  {
              
                      Json::Reader r;
                      Json::Value v;
                      r.parse(res.body, v);

                      int user_id = v["user_id"].asInt();
                      int room_id = v["room_id"].asInt();
                      this->rm_->SetRoomWinner(user_id, room_id);
                      // 保存对应数据到数据库
                      resp_json["status"] = this->db_svr_->InsertRoomInfo(rm_->GetRoomInfo(room_id));
                      cout <<"insert into database"<< resp_json["status"] << endl;
                      // resp_json["status"] = 1;
                  }
                  resp.body = this->Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
                  });

          http_svr_.Post("/Restart", [this](const Request &res, Response &resp)
          {
              
                  Json::Value resp_json;          
                  int user_id = this->all_sess_->CheckSessionInfo(res);
                  cout << "Restart" << endl;
                  if(user_id <= 0)
                  {
                      resp_json["status"] = -1;
                  }
                  else
                  {
              
                      Json::Reader r;
                      Json::Value v;
                      r.parse(res.body, v);

                      int user_id = v["user_id"].asInt();
                      int room_id = v["room_id"].asInt();
                      resp_json["status"] = this->rm_->RemoveRoom(room_id);
                    }
                  resp.body = this->Serialize(resp_json);
                  resp.set_header("Content-Type", "application/json");
          });
          pthread_t tid;
          int ret = pthread_create(&tid, NULL, MatchServer, (void*)this);
          if(ret < 0)
          {
              cout << "create match server failed" <<endl;
              exit(1);
          }
          cout << "pthread created" << endl;
          

          // 设置根目录+监听
          http_svr_.set_mount_point("/", "./www");  
          http_svr_.listen("0.0.0.0",37878);
        }

    static void* MatchServer(void *arg)
    {
        pthread_detach(pthread_self());
        WebGobang* wg = (WebGobang*)arg;

        cout << "Match Server Start" << endl;
        
        while(1)
        {
            pthread_mutex_lock(&wg->vec_lock_);
            while(wg->match_pool_num_ < 2)
            {
                pthread_cond_wait(&wg->vec_cond_, &wg->vec_lock_);
            }

            // 匹配池中匹配人数需 >= 2
            // 奇数 / 偶数需分情况处理
            // 偶数：两两匹配， 奇数：轮空一人
            
            int last_id = -1;
            auto& vec = wg->match_pool_;

            size_t vec_size = vec.size();
            if(vec_size % 2)
            {
                // 奇数, 最后一人停止匹配
                last_id = vec[vec_size - 1];
                vec_size -= 1;
            }

            for(int i = vec_size - 1; i >= 0 ; i -= 2)
            {
                int player_one = vec[i];
                int player_two = vec[i - 1];

                // 创建房间
                int room_id = wg->rm_->CreatRoom(player_one, player_two);
                cout << "room_id : " << room_id << endl;

                // 设置用户信息
                wg->pm_->SetUserStatus(player_one, PLAYING);
                wg->pm_->SetUserStatus(player_two, PLAYING);
                
                wg->pm_->SetRoomId(player_one, room_id);
                wg->pm_->SetRoomId(player_two, room_id);

                wg->pm_->SetUserChessName(player_one, "黑棋");
                wg->pm_->SetUserChessName(player_two, "白棋");
                 
                cout << "player_one: " << player_one << " player_two: "<< player_two<< endl;
            }
            wg->MatchPoolClear();
            pthread_mutex_unlock(&wg->vec_lock_);
            if(last_id > 0)
                wg->PushPlayer2MatchPool(last_id);
        }
        return NULL;
    }

    void MatchPoolClear()
    {
        match_pool_.clear();
        match_pool_num_ = 0;
    }

    int PushPlayer2MatchPool(int user_id)
    {
        // 设置用户为匹配状态
        pm_->SetUserStatus(user_id, MATCHING);

        // 将用户插入匹配池
        pthread_mutex_lock(&vec_lock_);
        match_pool_.push_back(user_id);
        match_pool_num_++;
        pthread_mutex_unlock(&vec_lock_);

        cout<<"match pool num: "<< match_pool_num_ <<endl;

        // 通知匹配线程工作
        pthread_cond_broadcast(&vec_cond_);
        return 0;
    }
    private:
        Server http_svr_;
        DataBaseSvr* db_svr_;
        AllSessionInfo* all_sess_;
        PlayerManager* pm_; 
        RoomManager* rm_;

        // 匹配池，登录的用户放入该匹配池中
        vector<int> match_pool_;
        int match_pool_num_;
        pthread_mutex_t vec_lock_;
        pthread_cond_t vec_cond_;
};
