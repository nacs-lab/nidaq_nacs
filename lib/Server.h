// Server for NIDAQ

#ifndef _NACS_NIDAQ_SERVER_H
#define _NACS_NIDAQ_SERVER_H

#include <zmq.hpp>
#include <stdio.h>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <mutex>
#include <utility>
#include <functional>
#include "SeqCache.h"
#include "Controller.h"
#include "Config.h"

namespace NiDAQ {

    class Server {
        public:
            Server(Config conf);

            bool runSeq(std::string chn_config, const char *data, uint32_t data_sz, uint64_t seqcnt);
            bool seqDone(uint64_t) const;

            bool stop();
            void run();

            private:
            struct QueueItem {
                SeqCache::Entry *entry;
                uint64_t id;
                std::vector<double> data;
                QueueItem(SeqCache::Entry *entry, uint64_t id, const char *data, uint32_t data_sz);
                operator bool() const
                {
                    return entry != nullptr;
                }
            };
            QueueItem popSeq();
            void seqRunner();
            bool recvMore(zmq::message_t&);

            Config m_conf;

            zmq::context_t m_zmqctx;
            zmq::socket_t m_zmqsock;
            std::atomic<bool> seq_finished{false};
            mutable std::mutex seq_finish_lock;
            Controller m_ctrl;
            SeqCache m_cache;
            std::atomic<uint64_t> m_seqfin{0};
            mutable std::mutex m_seqlock;
            std::condition_variable m_seqcv;
            std::vector<QueueItem> m_seque;
            bool m_running{false}; // Determines if server is running
            uint64_t m_serv_id;
        };

};


#endif