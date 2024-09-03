#ifndef CONNECTIONPOOL_HPP
#define CONNECTIONPOOL_HPP

#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <queue>
#include <functional>
#include <atomic>
#include <fstream>

#include "Common.hpp"

namespace pool
{   
    using time_type = decltype(std::chrono::steady_clock::now());

    /**
     * @brief 维护数据库(Mysql)的连接类
     */
    class Connection
    {
    public:
        Connection()
        {
            conn_=mysql_init(nullptr);
            if(nullptr==conn_){
                LOG("mysql init error");
                exit(-1);
            }
        }

        bool connect(const std::string& host,const std::string& user,const std::string& passwd,const std::string& db,std::uint32_t port=0)
        {
            if(nullptr==mysql_real_connect(conn_,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,nullptr,0)){
                LOG("mysql connect error");
                return false;
            }
            mysql_set_character_set(conn_,"utf8mb4");
            return true;
        }

        void update(const std::string& sql)
        {
            if(mysql_query(conn_,sql.c_str())){
                LOG("mysql query error");
                exit(-1);
            }
        }

        MYSQL_RES* query(const std::string& sql)
        {
            if(mysql_query(conn_,sql.c_str())){
                LOG("mysql query error");
                exit(-1);
            }
            return mysql_use_result(conn_);
        }

        ~Connection()
        {
            if(conn_!=nullptr) mysql_close(conn_);
        }

        void point()
        {
            start=std::chrono::steady_clock::now();
        }

        std::chrono::seconds getAliveTime()
        {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-start);
        }

    private:
        MYSQL* conn_=nullptr;
        time_type start;
    };

    /**
     * @brief 连接池
     */
    class ConnectionPool
    {
    public:
        static ConnectionPool* getInstance()
        {
            static ConnectionPool cp;
            return &cp;
        }

        std::shared_ptr<Connection> getConnection()
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            while(queue_connection_.empty()){
                std::cv_status cvs=queue_cond_.wait_for(lock,std::chrono::milliseconds(time_out_));
                if(cvs==std::cv_status::timeout&&queue_connection_.empty()){
                    LOG("get connection time out");
                    return nullptr;
                }
            }
            Connection* cn=queue_connection_.front();queue_connection_.pop();
            std::shared_ptr<Connection> sp=std::shared_ptr<Connection>(cn,std::bind(&ConnectionPool::cycleConnection,this,std::placeholders::_1));
            return sp;
        }

        std::size_t size()
        {
            return tot_connection_;
        }

    protected:
        /// @brief 回收连接
        /// @param cn 
        void cycleConnection(Connection* cn)
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            queue_connection_.emplace(cn);
            queue_connection_.front()->point();
            queue_cond_.notify_all();
        }

        /// @brief 创建空闲的连接
        void createMoreConnection()
        {
            for(;;)
            {
                {
                    std::unique_lock<std::mutex> lock(queue_mtx_);
                    while(!queue_connection_.empty()){
                        queue_cond_.wait(lock);
                    }
                    if(tot_connection_<max_connection_){
                        queue_connection_.emplace(new Connection());
                        if(queue_connection_.front()->connect(host_,user_,passwd_,db_)){
                            queue_connection_.front()->point();
                            tot_connection_++;
                            queue_cond_.notify_all();
                        }else{
                            delete queue_connection_.front();
                            queue_connection_.pop();
                        }
                    }
                }
            }
        }   

        /// @brief 回收空闲的连接
        void cycleFreeConnection()
        {
            for(;;)
            {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                {
                    std::unique_lock<std::mutex> lock(queue_mtx_);
                    while(tot_connection_>min_connection_&&std::chrono::seconds(alive_time_)<queue_connection_.front()->getAliveTime()){
                        delete queue_connection_.front();
                        tot_connection_--;
                        queue_connection_.pop();
                    }
                }
            }
        }

    private:
        //读取配置文件
        void init(const std::string& path)
        {
            std::ifstream in(path);
            if(!in.is_open()){
                LOG("open file fail");
                exit(-1);
            }
            std::string line;
            while(getline(in,line)){
                auto vst=line.find("=");
                auto ved=line.find("\n");
                auto k=line.substr(0,vst),v=line.substr(vst+1,ved-vst-1);
                if("host"==k){
                    host_=v;
                }else if("user"==k){
                    user_=v;
                }else if("passwd"==k){
                    passwd_=v;
                }else if("db"==k){
                    db_=v;
                }else if("min_connection"==k){
                    min_connection_=stoi(v);
                }else if("max_connection"==k){
                    max_connection_=stoi(v);
                }else if("time_out"==k){
                    time_out_=stoi(v);
                }else if("alive_time"==k){
                    alive_time_=stoi(v);
                }
            }
        }

        ConnectionPool()
        {
            /// 加载配置文件
            init("./confs/ConnectionPool.conf");

            /// 构造一定数量的连接
            for(int i=0;i<min_connection_;i++){
                queue_connection_.emplace(new Connection());
                if(queue_connection_.front()->connect(host_,user_,passwd_,db_)){
                    queue_connection_.front()->point();
                    tot_connection_++;
                }else{
                    delete queue_connection_.front();
                    queue_connection_.pop();
                }
            }

            /// 启动创建连接线程
            std::thread create(std::bind(&ConnectionPool::createMoreConnection,this));
            /// 启动多余回收线程
            std::thread cycle(std::bind(&ConnectionPool::cycleFreeConnection,this));

            /// 分离线程
            create.detach();
            cycle.detach();
        }

        ConnectionPool(const ConnectionPool& cp)=default;
        ConnectionPool& operator=(const ConnectionPool& cp)=default;
        ConnectionPool(ConnectionPool&& cp)=default;
        ConnectionPool& operator=(ConnectionPool&& cp)=default;

        ~ConnectionPool()
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            while(!queue_connection_.empty()){
                delete queue_connection_.front();
                queue_connection_.pop();
            }
        }

        std::string host_; ///< 数据库远端ip
        std::string user_; ///< 远端用户名
        std::string passwd_; ///< 远端用户密码
        std::string db_; ///< 操作的数据库名

        std::atomic<std::size_t> tot_connection_; ///< 已经创建连接数
        std::size_t min_connection_; ///< 最小连接数
        std::size_t max_connection_; ///< 最大连接数
        std::size_t time_out_; ///< 超时时间(毫秒)
        std::size_t alive_time_; ///< 保活时间(秒)

        std::queue<Connection*> queue_connection_; ///< 连接池队列
        std::mutex queue_mtx_; ///< 连接池互斥锁
        std::condition_variable queue_cond_; ///< 连接池条件变量
    };
}

#endif
