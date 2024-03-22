//

#include "../lib/Server.h"

#include <iostream>

int main(int argc, char **argv)
{
    std::string fname = "C:\\msys64\\home\\Tweezer5\\projects\\nidaq_nacs\\test\\config.yml";
    NiDAQ::Config conf;
    conf = conf.loadYAML(fname.data());
    NiDAQ::Server serv{conf};

    serv.run();
}