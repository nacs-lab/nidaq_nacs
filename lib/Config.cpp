//

#include "Config.h"

using namespace NaCs;

namespace NiDAQ {

NACS_EXPORT() Config::Config()
: listen("tcp://*:8888")
{
}

NACS_EXPORT() Config Config::loadYAML(const char *fname)
{
    Config conf;
    auto file = YAML::LoadFile(fname);
    if (auto server_node = file["server"])
    {
        if (auto listen_node = server_node["listen"])
            conf.listen = listen_node.as<std::string>();
    }
    if (auto dev_node = file["devices"])
        conf.devs = dev_node;
    return conf;
}

}