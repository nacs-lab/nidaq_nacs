//

#include <zmq.hpp>
#include <vector>
#include <nacs-utils/zmq_utils.h>

template<typename T>
void write(std::vector<uint8_t> &vec, uint32_t loc, T obj)
{
    memcpy(&vec[loc], &obj, sizeof(obj));
}

template<typename T>
uint32_t write(std::vector<uint8_t> &vec, T obj)
{
    static_assert(std::is_trivial_v<T>);
    auto len = (uint32_t)vec.size();
    vec.resize(len + sizeof(obj));
    memcpy(&vec[len], &obj, sizeof(obj));
    return len;
}

template<typename T>
size_t vector_sizeof(const typename std::vector<T>& vec)
{
    return sizeof(T) * vec.size();
}

uint32_t write(std::vector<uint8_t> &vec, const uint8_t *src, size_t sz)
{
    auto len = (uint32_t)vec.size();
    vec.resize(len + sz);
    memcpy(&vec[len], src, sz);
    return len;
}

uint32_t write(std::vector<uint8_t> &vec, char *src, size_t sz)
{
    auto len = (uint32_t)vec.size();
    vec.resize(len + sz);
    memcpy(&vec[len], src, sz);
    return len;
}


int main(int argc, char **argv)
{
    std::unique_ptr<NaCs::ZMQ::MultiClient::SockRef> m_sock;
    auto &client = NaCs::ZMQ::MultiClient::global();
    m_sock.reset(new NaCs::ZMQ::MultiClient::SockRef(client.get_socket("tcp://127.0.0.1:8889")));

    //zmq::context_t m_zmqctx;
    //zmq::socket_t m_zmqsock(m_zmqctx, ZMQ_REQ)
    std::vector<uint8_t> msg;
    uint32_t version = 0;
    std::string chn_config = "Dev1/ao0,Dev1/ao1";

    // Data
    std::vector<double> act_data;
    for(int i = 0;i<4000;i++) {
        act_data.push_back(5.0*(double)i/4000.0);
    }
    for(int i = 0;i<4000;i++) {
        act_data.push_back(5.0*(double)(4000 -i)/4000.0);
    }

    write(msg, chn_config.data(), chn_config.size() + 1);
    write(msg, (uint32_t) act_data.size());
    write(msg, (uint8_t *) act_data.data(), vector_sizeof(act_data));

    auto reply = m_sock->send_msg([&] (auto &sock) {
        NaCs::ZMQ::send_more(sock, NaCs::ZMQ::str_msg("run_seq"));
        NaCs::ZMQ::send_more(sock, NaCs::ZMQ::bits_msg(version));
        NaCs::ZMQ::send(sock, zmq::message_t(msg.data(), msg.size()));
    }).get();

    uint64_t m_cur_wait_id = 0;

    if (reply.empty())
        throw std::runtime_error("did not get reply from server");
    std::string reply_str((char*) reply[0].data(), reply[0].size());
    if (reply_str.compare("ok") == 0)
    {
        if (reply.size() < 2)
            throw std::runtime_error("expecting id after ok");
        auto rep_data = (const uint8_t*)reply[1].data();
        memcpy(&m_cur_wait_id, rep_data, sizeof(m_cur_wait_id));
    }
    else {
        throw std::runtime_error("unknown server side error");
    }

    m_sock->send_msg([&] (auto &sock) {
        NaCs::ZMQ::send_more(sock, NaCs::ZMQ::str_msg("wait_seq"));
        NaCs::ZMQ::send(sock, NaCs::ZMQ::bits_msg(m_cur_wait_id));
    }).wait();
    
    NaCs::ZMQ::shutdown();

}