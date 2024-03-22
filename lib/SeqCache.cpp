//

#include "SeqCache.h"

#include <sstream>
#include <iostream>
#include <vector>

namespace NiDAQ {
    template <typename T>
    static inline size_t entrySize(const T &entry)
    {
        return 1;
    }

    NACS_EXPORT() SeqCache::SeqCache(size_t szlim) :
    m_szlim(szlim)
    {    
        
    }

    NACS_EXPORT() bool SeqCache::get(std::string chn_config, Entry* &entry){
    // This version is called if seq is sent!
    if (_get(chn_config, entry)){
        // In cache and age has been mutated. Entry is also now populated. 
        return true;
    }
    Sequence seq(*this, chn_config);
    if (seq.is_valid) {
        auto it = m_cache.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(chn_config)), std::forward_as_tuple(std::move(seq))).first;
        m_totalsz += entrySize(it);
        ssize_t age = it->second.age.load(std::memory_order_relaxed);
        ssize_t new_age;
        do {
            if (age > 0) {
                new_age = age + 1;
            }
            else {
                new_age = 1;
            }} while (!it->second.age.compare_exchange_weak(age, new_age, std::memory_order_relaxed));
        entry = &(it->second);
        while (m_totalsz > m_szlim) {
            if (!ejectOldest()) {
                break;
            }
        }
        return true;
    }
    else {
        return false;
    }
    }

    bool SeqCache::_get(std::string chn_config, Entry* &entry)
    {
        // This is the only function that mutate the cache. Other thread can only call unref
        // and can only mutate the age.
        // Since none of the mutation invalidates map iterator we don't need a lock.
        // For this to be safe, we also need `unref` to not do any cache lookup.
        auto it = m_cache.find(chn_config);
        if (it != m_cache.end()){
    // In fact, no other thread should mutate anything other than `age` so we don't
            // need any synchronization when accessing `age`. We only need `relaxed` ordering
            // to ensure there's no data race.
            ssize_t age = it->second.age.load(std::memory_order_relaxed);
            ssize_t new_age;
            do {
                if (age > 0) {
                    new_age = age + 1;
                }
                else {
                    new_age = 1;
                }
            } while (!it->second.age.compare_exchange_weak(age, new_age, std::memory_order_relaxed));
            entry = &(it->second);
            return true;
        }
        return false;
    }

    bool SeqCache::ejectOldest()
    {
        if (m_cache.empty()) {
            m_totalsz = 0;
            return false;
        }
        auto end = m_cache.end();
        auto entry = end;
        for (auto it = m_cache.begin(); it != end; ++it) {
            if (it ->second.age > 0)
                continue;
            if (entry == end || entry->second.age < it->second.age) {
                entry = it;
                continue;
            }
        }
        if (entry == end)
            return false;
        m_totalsz -= entrySize(entry);
        m_cache.erase(entry);
        return true;
    }

    NACS_EXPORT() void SeqCache::unref(const Entry &entry) const
    {
        ssize_t global_age = -1;
        auto get_global_age = [&] {
            if (global_age == -1)
                global_age = m_age.fetch_add(1, std::memory_order_relaxed) + 1;
            return global_age;
        };

        ssize_t age = entry.age.load(std::memory_order_relaxed);
        ssize_t new_age;
        do {
            if (age > 1) {
                new_age = age - 1;
            }
            else {
                new_age = -get_global_age();
            }
        } while(!entry.age.compare_exchange_weak(age, new_age, std::memory_order_relaxed));
    }

    NACS_EXPORT() bool SeqCache::hasSeq(std::string chn_config) {
        return m_cache.count(chn_config) >= 1;
    }

    NACS_EXPORT() SeqCache::Sequence::Sequence(SeqCache& cache, std::string chn_config) :
    m_cache(cache)
    {
        uint32_t num_chn = 0;
        std::istringstream f(chn_config);
        std::string s;  
        auto status = DAQmxCreateTask("",&t_hdl);
        if (DAQmxFailed(status))
            goto error;
        
        // Iterate through channels and create them in the correct order.
        // chn_config is a comma delimited string
        // Example: "Dev1/ao2,Dev2/ao3"  
        while (getline(f, s, ',')) {
            status = DAQmxCreateAOVoltageChan(t_hdl,s.data(),"",-10.0,10.0,DAQmx_Val_Volts,NULL);
            if (DAQmxFailed(status))
                goto error;
            num_chn++;
        }
        n_chns = num_chn;
        is_valid = true;

        return;
        error:
            char errBuff[2048]={'\0'};
            DAQmxGetExtendedErrorInfo(errBuff,2048);
            if( t_hdl!=0 ) {
                /*********************************************/
                // DAQmx Stop Code
                /*********************************************/
                DAQmxStopTask(t_hdl);
                DAQmxClearTask(t_hdl);
            }
            printf("DAQmx Error: %s\n",errBuff);
            fflush(stdout);
            is_valid = false;
    }
};