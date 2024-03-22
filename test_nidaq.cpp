#include <stdio.h>
#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int main(void){
    	int         error=0;
	TaskHandle  taskHandle=0;
	float64     data[4000];
        float64     data2[8000];
	char        errBuff[2048]={'\0'};
	int			i=0;
	int32   	written;

	for(;i<4000;i++) {
            data[i] = 5.0*(double)i/4000.0;
            data2[3999 - i] = 5.0*(double)i/4000.0;
        }
        i = 0;
        for(;i<4000;i++) {
            data2[4000 + i] = 5.0 * (double)i/4000.0;
        }
	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
	//DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle,"Dev1/ao1","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle,"Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(taskHandle,"PFI1",DAQmx_Val_Rising));
	DAQmxErrChk (DAQmxCfgOutputBuffer(taskHandle, 4000));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"PFI0",1000.0,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,3999));

	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle,3999,0,10.0,DAQmx_Val_GroupByChannel,data2,&written,NULL));
        printf("Written: %li\n", written);
	//DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle,4000,0,10.0,DAQmx_Val_GroupByChannel,data2,&written,NULL));
        //printf("Written: %li\n", written);

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	/*********************************************/
	// DAQmx Wait Code
	/*********************************************/
	DAQmxErrChk (DAQmxWaitUntilTaskDone(taskHandle,120.0));

        	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
        DAQmxErrChk(DAQmxStopTask(taskHandle));
	
	
	//DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle,8000,0,10.0,DAQmx_Val_GroupByChannel,data2,&written,NULL));
    //    printf("Written: %li\n", written);
    //    DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"PFI0",1000.0,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,4000));
	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	//DAQmxErrChk (DAQmxStartTask(taskHandle));

	/*********************************************/
	// DAQmx Wait Code
	/*********************************************/
	//DAQmxErrChk (DAQmxWaitUntilTaskDone(taskHandle,20.0));

    //    DAQmxErrChk(DAQmxStopTask(taskHandle));

    //    DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle,4000,0,10.0,DAQmx_Val_GroupByChannel,data,&written,NULL));
        	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	//DAQmxErrChk (DAQmxStartTask(taskHandle));

	/*********************************************/
	// DAQmx Wait Code
	/*********************************************/
	//DAQmxErrChk (DAQmxWaitUntilTaskDone(taskHandle,20.0));

    //    DAQmxErrChk(DAQmxStopTask(taskHandle)); */

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
	}
	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);
	fflush(stdout);
	printf("End of program, press Enter key to quit\n");
	fflush(stdout);
	getchar();
	return 0;
}
