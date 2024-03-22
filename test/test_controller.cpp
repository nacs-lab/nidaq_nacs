//

#include "../lib/Controller.h"
#include "../lib/SeqCache.h"
#include <yaml-cpp/yaml.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else


int main() {
    int error=0;
    //NiDAQ::SeqCache cache = NiDAQ::SeqCache(1024);

    auto file = YAML::LoadFile("C:\\msys64\\home\\Tweezer5\\projects\\nidaq_nacs\\test\\config.yml");
    NiDAQ::Controller ctrl = NiDAQ::Controller(file["devices"]);
    for (int i = 0; i < ctrl.m_dev_infos.size(); i++) {
        auto info = ctrl.m_dev_infos[i];
        printf("name: %s, trig_chn: %s, clock_chn: %s, sample_rate: %llu\n", info.dev_name.data(), info.trig_chn.data(), info.clock_chn.data(), info.sample_rate);
    }

    // Load in a sequence. Note that right now, only device 1 is used.
    printf("Hello\n");
    // Data
    double act_data[4000];
    for(int i = 0;i<4000;i++) {
        act_data[i] = 5.0*(double)i/4000.0;
    }
    /*TaskHandle t_hdl;
    int32_t written;
    DAQmxErrChk(DAQmxCreateTask("",&t_hdl));
    DAQmxErrChk(DAQmxCreateAOVoltageChan(t_hdl,"Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,NULL));
    DAQmxErrChk(DAQmxCfgSampClkTiming(t_hdl,"PFI0",1000,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,4000));
    DAQmxErrChk(DAQmxWriteAnalogF64(t_hdl,4000,0,10.0,DAQmx_Val_GroupByChannel,act_data,&written,NULL));
    DAQmxErrChk(DAQmxStartTask(t_hdl));
    DAQmxErrChk(DAQmxWaitUntilTaskDone(t_hdl,120.0));
    DAQmxErrChk(DAQmxStopTask(t_hdl));*/

    
    NiDAQ::SeqCache cache = NiDAQ::SeqCache(1024);
    NiDAQ::SeqCache::Entry* entry;
    std::string chn_config = "Dev1/ao0";
    uint32_t sz = chn_config.size();
    const uint8_t* data = (const uint8_t*) chn_config.data();
    cache.get(chn_config, entry);

    ctrl.load_seq(entry->m_seq, act_data, 4000);
    printf("Starting seq\n");
    ctrl.start_seq(entry->m_seq);
    ctrl.wait_for_seq(entry->m_seq);
    printf("Seq done\n");
    ctrl.stop_seq(entry->m_seq);
    /*
    Error:
    char errBuff[2048]={'\0'};
    if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( t_hdl!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		/*DAQmxStopTask(t_hdl);
		DAQmxClearTask(t_hdl);
	}
	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);
	printf("End of program, press Enter key to quit\n");
	getchar();
	return 0; */
}

