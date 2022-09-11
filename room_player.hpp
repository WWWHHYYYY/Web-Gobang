/***
 *玩家管理+房间管理
 ***/
#pragma once
#include <unordered_map>
#include <string>
#include <iostream>
#include <jsoncpp/json/json.h>

using namespace std;

typedef enum PlayerStatus
{
    ONLINE=0,
    MATCHING,
    PLAYING,
}status_t;


class Player
{
    public:
        Player()
        {}
        Player(int user_id)
        {
            user_id_ = user_id;
            room_id_ = -1;
            player_status_ = ONLINE;
            chess_name_.clear();
        }
    ~Player()
    {}
    
    

    public:
        int user_id_;
        int room_id_;

        status_t player_status_;
        string chess_name_ = "";
};

class PlayerManager
{   
    public:
        PlayerManager()
        {
            player_map_.clear();
            pthread_mutex_init(&map_lock_, NULL);
        }
        ~PlayerManager()
        {
            pthread_mutex_destroy(&map_lock_);
        }
        
        void InsertPlayer2Map(int user_id)
        {
            pthread_mutex_lock(&map_lock_);
            Player p(user_id);
            player_map_.insert({user_id, p});
            pthread_mutex_unlock(&map_lock_);
        }

        void SetUserStatus(int user_id ,const status_t sta)
        {
           player_map_[user_id].player_status_ = sta;
        }
        
        void SetRoomId(int user_id, int room_id)
        {
            player_map_[user_id].room_id_ = room_id;
        }

        Player& GetPlayerInfo(int user_id)
        {
            return player_map_[user_id];
        }

        void SetUserChessName(int user_id, const string chess_name)
        {
            player_map_[user_id].chess_name_ = chess_name;
        }

        void ResetUserNameInfo(int user_id)
        {
            player_map_[user_id].chess_name_ = "";
            player_map_[user_id].room_id_ = -1;
            player_map_[user_id].player_status_ = MATCHING;
        }
    private:
        unordered_map<int, Player> player_map_;
        pthread_mutex_t map_lock_;
};

class Room
{
    public:
        Room(){}
        Room(int p1, int p2, int room_id)
        {
            p1_ = p1;
            p2_ = p2;

            who_turn_ = p1;
            step_num_ = 0;
            room_id_ = room_id;
            step_vec_.clear();

            winner_ = -1;
        }

        ~Room()
        {}
        
        bool isMyTurn(int user_id)
        {
            return user_id == who_turn_ ? 1:0;
        }
          
        int Step(int user_id, const string &body)
        
        {   
            if(user_id != who_turn_)
              return 0;

            step_vec_.push_back(body);
            who_turn_ = (user_id == p1_ ? p2_:p1_);

            step_num_++;
            return 1;
        }

        void SetWinner(int user_id)
        {
            winner_ = user_id;
        }

        int GetPeerStep(int user_id, string& s)
        {
            if(step_num_ <= 0)
                return -1;
            if(user_id != who_turn_)
            {
                return -2;
            }
            s = step_vec_[step_num_ - 1];
            return 0;
        }

        vector<string>& GetRoomStepInfo()
        {  
          return step_vec_;
        }
    public:
        int p1_;
        int p2_;

        // 记录当前是谁的回合
        int who_turn_;
        int step_num_;
        int room_id_;

        // 记录用户走的每一步
        vector<string> step_vec_;
        int winner_;
        
};

class RoomManager
{
    public:
        RoomManager()
        {
            room_map_.clear();
            pthread_mutex_init(&map_lock_, NULL);

            prepare_id_ = 10000;
        }
        


        ~RoomManager()
        {
            pthread_mutex_destroy(&map_lock_);
        }

        int CreatRoom(int p1, int p2)
        {
            pthread_mutex_lock(&map_lock_);
            int room_id = prepare_id_++;
            Room r(p1, p2, room_id);
            room_map_.insert({room_id, r});
            pthread_mutex_unlock(&map_lock_);

            return room_id;
        }
        
        int isMyTurn(const int user_id, const int room_id)
        {
            return room_map_[room_id].isMyTurn(user_id);
        }
        
        int Step(const string&body)
        {
            Json::Reader r;
            Json::Value v;
            r.parse(body, v);

            int room_id = v["room_id"].asInt();
            int user_id = v["user_id"].asInt();

            return room_map_[room_id].Step(user_id, body);
        }

        int GetRoomStep(const string &body, string &s)
        {
            Json::Reader r;
            Json::Value v;
            r.parse(body ,v);

            int room_id = v["room_id"].asInt();
            int user_id = v["user_id"].asInt();

            return room_map_[room_id].GetPeerStep(user_id, s);
        }

        void SetRoomWinner(const int user_id, const int room_id)
        {
            room_map_[room_id].SetWinner(user_id);
        }

        Room& GetRoomInfo(int room_id)
        {
            return room_map_[room_id];
        }

        int RemoveRoom(int room_id)
        {
            pthread_mutex_lock(&map_lock_);
            auto iter = room_map_.find(room_id);
            if(iter == room_map_.end())
            {
                pthread_mutex_unlock(&map_lock_);
                return -1;
            }
            room_map_.erase(room_id);
            pthread_mutex_unlock(&map_lock_);
            return 0;
        }

    private:
        unordered_map<int, Room> room_map_;
        pthread_mutex_t map_lock_;
        
        // 预分配的房间 id
        int prepare_id_;
};
