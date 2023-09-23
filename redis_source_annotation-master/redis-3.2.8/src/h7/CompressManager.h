#pragma once

#include "c_common.h"
#include "common.h"
#include <map>
#include <memory>
#include <mutex>

#include "compress_ctx.h"

namespace h7 {

struct Compressor{
    ~Compressor(){};
    virtual uint64 compress(const void* data, uint64 len, String* out) = 0;
    virtual uint64 deCompress(const void* data, uint64 len, String* out) = 0;
};

class CompressManager
{
public:
    static CompressManager* get();
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
    CompressManager(){};
    std::mutex m_mtx;
    std::map<int, std::shared_ptr<Compressor>> m_map;
};

}
