//

#include "Server.h"

#include <nacs-utils/zmq_utils.h>
#include <nacs-utils/fd_utils.h>
#include <nacs-utils/processor.h>
#include <nacs-utils/timer.h>

#include <yaml-cpp/yaml.h>

#include <system_error>
#include <thread>
#include <chrono>

using namespace NaCs;

namespace NiDAQ {
    struct WaitSeq{
    std::vector<zmq::message_t> addr;
    uint64_t id;
    };

    NACS_EXPORT() Server::QueueItem::QueueItem(SeqCache::Entry *in_entry, uint64_t in_id, const char *in_data, uint32_t data_sz) :
        entry(in_entry),
        id(in_id)
    {
        double *ptr = (double *) in_data;
        if (in_data)
            data = std::vector<double>( ptr, ptr + data_sz/8);
    }
    
    NACS_EXPORT() Server::Server(Config conf)
    : m_conf(std::move(conf)), 
    m_zmqctx(),
    m_zmqsock(m_zmqctx, ZMQ_ROUTER),
    m_ctrl(m_conf.devs),
    m_cache(1024) // pretty arbitrary
    {
        m_zmqsock.bind(m_conf.listen);
        m_serv_id = getTime(); //uint64_t in nanoseconds
    }

    NACS_EXPORT() bool Server::stop()
    {
        if (!m_running)
            return false;
        {
            std::lock_guard<std::mutex> locker(m_seqlock);
            m_running = false;
        }
        m_seqcv.notify_all();
        {
            std::lock_guard<std::mutex> locker(seq_finish_lock);
            seq_finished.store(true, std::memory_order_release);
        }
        return true;
    }

    NACS_EXPORT() bool Server::runSeq(std::string chn_config, const char *data, uint32_t data_sz, uint64_t seqcnt)
    {
        SeqCache::Entry* entry;
        if (!m_cache.get(chn_config, entry)) {
            return false;
        }
        {
            std::lock_guard<std::mutex> locker(m_seqlock);
            m_seque.push_back(QueueItem(entry, seqcnt, data, data_sz));
        }
        // This is a little racy, in principle we need to wait
        // until some data has been pushed to the controller.
        // However, we can't possibly remove the dependency
        // on the CPU code to be fast enough and this particular
        // issue is quite minor so I'm not going to worry about
        // it for now.
        m_seqcv.notify_all(); // Notifies Runner that sequence has arrived.
        return true;
    }

    NACS_EXPORT() bool Server::seqDone(uint64_t id) const
    {
        return m_seqfin.load(std::memory_order_acquire) >= id;
    }

    NACS_INTERNAL auto Server::popSeq() -> QueueItem
    {
        std::unique_lock<std::mutex> locker(m_seqlock);
        QueueItem res = QueueItem(nullptr, 0, nullptr, 0);
        m_seqcv.wait(locker, [&] {
            if (!m_running)
                return true;
            if (m_seque.empty())
                return false;
            res = m_seque[0];
            m_seque.erase(m_seque.begin());
            return true;
        });
        return res;
    }

    NACS_INTERNAL void Server::seqRunner()
    {
        // this call to popSeq hangs until a sequence (QueueItem) can be popped off
        while (auto entry = popSeq()) {
            m_ctrl.load_seq(entry.entry->m_seq, entry.data.data(), entry.data.size());
            m_ctrl.start_seq(entry.entry->m_seq);
            m_ctrl.wait_for_seq(entry.entry->m_seq);
            m_ctrl.stop_seq(entry.entry->m_seq);
            printf("Sequence finished!\n");
            fflush(stdout);
            m_seqfin.store(entry.id, std::memory_order_release);
            // Notify server thread
            {
                std::lock_guard<std::mutex> locker(seq_finish_lock);
                seq_finished.store(true, std::memory_order_release);
            }
        }
    }

    inline bool Server::recvMore(zmq::message_t &msg) {
        return ZMQ::recv_more(m_zmqsock, msg);
    }

    NACS_EXPORT() void Server::run() {
        printf("Running Server\n");
        fflush(stdout);
        uint64_t seqcnt = 0;
        m_running = true;
        std::thread worker(&Server::seqRunner, this);
        zmq::message_t empty(0);

        std::vector<WaitSeq> waits;

        auto send_reply = [&] (auto &addr, auto &&msg) {
            ZMQ::send_addr(m_zmqsock, addr, empty);
            ZMQ::send(m_zmqsock, msg);
        };

        auto handle_msg = [&] {
            auto addr = ZMQ::recv_addr(m_zmqsock);

            zmq::message_t msg;

            if (!recvMore(msg)) {
                printf("No further message received\n");
                fflush(stdout);
                send_reply(addr, ZMQ::bits_msg(false));
                goto out;
            }
            else if (ZMQ::match(msg, "run_seq")) {
                printf("run_seq received\n");
                fflush(stdout);
                // [4B: Version]
                if (!recvMore(msg) || msg.size() != 4) {
                    // No version
                    printf("No version received\n");
                    fflush(stdout);
                    send_reply(addr, ZMQ::bits_msg(uint64_t(0)));
                    goto out;
                }
                uint32_t ver;
                memcpy(&ver, msg.data(), 4);
                if (ver != 0) {
                    // Wrong version
                    printf("Version %lu not recognized\n", ver);
                    fflush(stdout);
                    send_reply(addr, ZMQ::bits_msg(uint64_t(0)));
                    goto out;
                }
                if (!recvMore(msg)) {
                    // No code
                    printf("Data not received\n");
                    fflush(stdout);
                    send_reply(addr, ZMQ::bits_msg(uint64_t(0)));
                    goto out;
                }
                //[chn-config: NUL terminated string][4B: data_sz][data] data should be doubles
                auto msg_data = (const uint8_t*) msg.data();
                uint32_t msg_sz = msg.size();
                std::string chn_config((char *) msg_data);
                uint32_t str_size = chn_config.size() + 1;
                msg_data += str_size;
                msg_sz -= str_size;
                auto id = ++seqcnt;
                printf("chn_config: %s\n", chn_config.data());
                fflush(stdout);

                uint32_t data_sz;
                memcpy(&data_sz, msg_data, 4);

                msg_data += 4;
                msg_sz -= 4;

                if (!runSeq(chn_config, (const char*) msg_data, msg_sz, id)) {
                    printf("Error in running sequence\n");
                    fflush(stdout);
                    send_reply(addr, ZMQ::str_msg("error"));
                    goto out;
                }
                // Do as much as possible before waiting for the wait request to be processed
                ZMQ::send_addr(m_zmqsock, addr, empty);
                auto rpy = ZMQ::bits_msg(id);
                ZMQ::send_more(m_zmqsock, ZMQ::str_msg("ok"));
                ZMQ::send(m_zmqsock, rpy);
            }
            else if (ZMQ::match(msg, "wait_seq")) {
                printf("wait_seq received\n");
                fflush(stdout);
                if (!recvMore(msg) || msg.size() != 8) {
                    send_reply(addr, ZMQ::bits_msg(false));
                    goto out;
                }
                uint64_t id;
                memcpy(&id, msg.data(), 8);
                if (id > seqcnt || id == 0) {
                    send_reply(addr, ZMQ::bits_msg(false));
                    goto out;
                }
                if (seqDone(id)) {
                    send_reply(addr, ZMQ::bits_msg(true));
                    goto out;
                }
                waits.push_back(WaitSeq{std::move(addr), id});
            }
        out:
            printf("something else received\n");
            fflush(stdout);
            ZMQ::readall(m_zmqsock);
        };

        // The majority of the above were just definitions of two key function used by run. Now, we get hte actual code of Server::run
        // three events to poll: Requests from control computer, events from the other thread about the completion of a sequence (thorugh m_evfd), and triggers
        std::vector<zmq::pollitem_t> polls {
            {(void*) m_zmqsock, 0, ZMQ_POLLIN, 0}
        };
        while (m_running) {
            zmq::poll(polls);
            //auto t = getTime();
            //uint64_t t = 0;
            if (polls[0].revents & ZMQ_POLLIN) {
                // network event
                printf("Network event received!\n");
                fflush(stdout);
                handle_msg();
            }
            else {
                std::lock_guard<std::mutex> locker(seq_finish_lock);
                if (seq_finished.load(std::memory_order_acquire))
                {
                    seq_finished.store(false, std::memory_order_release);
                    // sequence finish event
                    for (size_t i = 0; i < waits.size(); i++) {
                        auto &wait = waits[i];
                        if (!seqDone(wait.id))
                            continue;
                        send_reply(wait.addr, ZMQ::bits_msg(true));
                        waits.erase(waits.begin() + i);
                        i--;
                    }
                }
            }
        }
        for (auto &wait: waits)
            send_reply(wait.addr, ZMQ::bits_msg(false));

        stop();

        worker.join();
    }
};