#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <memory>
#include <iostream>
#include <fstream>

namespace config
{   
    /**
     * @brief 解析类
     */
    class Config
    {   
    public:
        Config()=default;

        Config(const std::string& path)
        {
            load(path);
        }

        std::string get(const std::string& k)
        {
            auto it=kvs.find(k);
            if(it==kvs.end()) return nullptr;
            return it->second;
        }

        bool load(const std::string& path)
        {
            std::ifstream in(path);
            if(!in.is_open()){
                std::cerr<<"file open fail"<<std::endl;
                return false;
            }

            std::string line;
            kvs.clear();
            while(getline(in,line)){
                if(line.empty()||line[0]=='#'||line[0]=='[') continue;
                auto vst=line.find("=");
                auto ven=line.find("\n");
                kvs[line.substr(0,vst)]=line.substr(vst+1,ven-vst-1);
            }

            in.close();
            return true;
        }
    private:
        std::map<std::string,std::string> kvs;
    };
}

#endif