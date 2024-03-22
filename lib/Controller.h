// Class for managing NIDAQ Devices

#ifndef _NACS_NIDAQ_CTRL_H
#define _NACS_NIDAQ_CTRL_H

#include "SeqCache.h"

#include <stdio.h>
#include <NIDAQmx.h>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <nacs-utils/utils.h>

namespace NiDAQ {
    struct DeviceInfo {
        std::string dev_name;
        std::string trig_chn;
        std::string clock_chn;
        uint64_t sample_rate;
        DeviceInfo(std::string dev_name, std::string trig_chn, std::string clock_chn, uint64_t sample_rate);
    };

    class Controller{
        public:
            Controller(const YAML::Node &config);

            bool load_seq(SeqCache::Sequence &seq, const double *data, uint32_t sz);
            bool start_seq(SeqCache::Sequence &seq);
            bool wait_for_seq(SeqCache::Sequence &seq);
            bool stop_seq(SeqCache::Sequence &seq);

            std::vector<DeviceInfo> m_dev_infos;
    };

};

#endif