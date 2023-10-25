#pragma once

#include "CompressManager.h"

namespace h7 {

class DNACompressManager
{
public:

    static DNACompressManager* get();
    static void regAll();

    bool reg(int type, std::shared_ptr<Compressor> cmp){
        auto it = m_map.find(type);
        if(it != m_map.end()){
           return false;
        }
        m_map[type] = cmp;
        return true;
    }
    void unreg(int type){
        auto it = m_map.find(type);
        if(it != m_map.end()){
            m_map.erase(it);
        }
    }
    std::shared_ptr<Compressor> getCompressor(int type){
        auto it = m_map.find(type);
        if(it != m_map.end()){
            return it->second;
        }
        return nullptr;
    }

private:
    DNACompressManager(){};
    std::map<int, std::shared_ptr<Compressor>> m_map;
};

}
