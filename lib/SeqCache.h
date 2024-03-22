// SeqCache for storing sequence configurations, so that they can be restored. These are known as "tasks" as far as the NiDAQMx is concerned

#ifndef _NACS_NIDAQ_SEQCACHE_H
#define _NACS_NIDAQ_SEQCACHE_H

#include <stdio.h>
#include <NIDAQmx.h>
#include <nacs-utils/utils.h>
#include <atomic>

namespace NiDAQ {

    class SeqCache {
        public:

        // A sequence which is constructed. The TaskHandle which represents the sequence is stored.
        struct Sequence {
            Sequence(SeqCache& cache, std::string chn_config);
            Sequence(Sequence&&) = default;

            SeqCache& m_cache;
            TaskHandle t_hdl;
            uint32_t n_chns;
            bool is_valid;
        };

        // An entry in the cache.
        struct Entry {
        Sequence m_seq;
        Entry(Sequence seq)
            : m_seq(std::move(seq)),
              age(1)
        {
        }
        Entry(const Entry&) = delete;
        Entry(Entry&&) = default;
        private:
            // >0 means refcount
            // <0 means age
            mutable std::atomic<ssize_t> age;
            friend class SeqCache;
        };

        SeqCache(size_t szlim);
        bool get(std::string chn_config, Entry* &entry);
        void unref(const Entry &entry) const;
        bool hasSeq(std::string chn_config);

    private:
        bool ejectOldest();
        bool _get(std::string chn_config, Entry* &entry);
        const size_t m_szlim;
        size_t m_totalsz{0};
        std::map<std::string, Entry> m_cache{};
        mutable std::atomic<ssize_t> m_age __attribute__((aligned(64))){0};
        };

};

#endif