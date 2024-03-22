//

#include "Controller.h"
namespace NiDAQ {
    NACS_EXPORT() DeviceInfo::DeviceInfo(std::string dev_name, std::string trig_chn, std::string clock_chn, uint64_t sample_rate) :
        dev_name(dev_name),
        trig_chn(trig_chn),
        clock_chn(clock_chn),
        sample_rate(sample_rate)
    {
    }
    NACS_EXPORT() Controller::Controller(const YAML::Node &config) {
        for (auto it = config.begin(); it != config.end(); ++it) {
            std::string dev_name = (it->first).as<std::string>();
            auto info_node = it->second;
            uint64_t sample_rate;
            std::string trig_chn;
            std::string clock_chn;
            if (auto sample_node = info_node["sample_rate"]){
                sample_rate = sample_node.as<uint64_t>();
            }
            else {
                throw std::runtime_error("Sample rate not specified. Specify as sample_rate.");
            }
            if (auto trig_node = info_node["trig_chn"]) {
                trig_chn = trig_node.as<std::string>();
            }
            else {
                throw std::runtime_error("Trigger channel name not specified. Specify as trig_chn.");
            }
            if (auto clock_node = info_node["clock_chn"]){
                clock_chn = clock_node.as<std::string>();
            }
            else {
                throw std::runtime_error("Clock channel name not specified. Specify as clock_chn.");
            }
            m_dev_infos.emplace_back(dev_name, trig_chn, clock_chn, sample_rate);
        }
    }

    NACS_EXPORT() bool Controller::load_seq(SeqCache::Sequence &seq, const double *data, uint32_t sz) {
        TaskHandle &t_hdl = seq.t_hdl;
        uint32_t num_chn = seq.n_chns;
        int32_t written;
        auto status = DAQmxCfgDigEdgeStartTrig(t_hdl,m_dev_infos[0].trig_chn.data(),DAQmx_Val_Rising);
        if (DAQmxFailed(status))
            goto error;
        status = DAQmxCfgSampClkTiming(t_hdl,m_dev_infos[0].clock_chn.data(),m_dev_infos[0].sample_rate,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,sz / num_chn);
        if (DAQmxFailed(status))
            goto error;
        status = DAQmxWriteAnalogF64(t_hdl,sz / num_chn,0,10.0,DAQmx_Val_GroupByChannel,data,&written,NULL);
        if (DAQmxFailed(status))
            goto error;
        return true;
        error:
            char errBuff[2048]={'\0'};
            DAQmxGetExtendedErrorInfo(errBuff,2048);
            printf("DAQmx Error: %s\n",errBuff);
            fflush(stdout);
            return false;
    }

    NACS_EXPORT() bool Controller::start_seq(SeqCache::Sequence &seq) {
        TaskHandle &t_hdl = seq.t_hdl;
        auto status = DAQmxStartTask(t_hdl);
        if (DAQmxFailed(status))
            goto error;
        return true;
        error:                  
            char errBuff[2048]={'\0'};
            DAQmxGetExtendedErrorInfo(errBuff,2048);
            printf("DAQmx Error: %s\n",errBuff);
            fflush(stdout);
            return false;
    }

    NACS_EXPORT() bool Controller::wait_for_seq(SeqCache::Sequence &seq) {
        TaskHandle &t_hdl = seq.t_hdl;
        auto status = DAQmxWaitUntilTaskDone(t_hdl,120.0); // Hard coded timeout of 120 seconds
        if (DAQmxFailed(status))
            goto error;
        return true;
        error:                  
            char errBuff[2048]={'\0'};
            DAQmxGetExtendedErrorInfo(errBuff,2048);
            printf("DAQmx Error: %s\n",errBuff);
            fflush(stdout);
            return false;
    }

    NACS_EXPORT() bool Controller::stop_seq(SeqCache::Sequence &seq) {
        TaskHandle &t_hdl = seq.t_hdl;
        auto status = DAQmxStopTask(t_hdl); // Hard coded timeout of 120 seconds
        if (DAQmxFailed(status))
            goto error;
        return true;
        error:                  
            char errBuff[2048]={'\0'};
            DAQmxGetExtendedErrorInfo(errBuff,2048);
            printf("DAQmx Error: %s\n",errBuff);
            fflush(stdout);
            return false;
    }


};