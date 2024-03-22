#ifndef _NACS_NIDAQ_CONFIG_H
#define _NACS_NIDAQ_CONFIG_H

#include <string>
#include <map>
#include <yaml-cpp/yaml.h>

#include <nacs-utils/utils.h>
#include <stdint.h>

using namespace NaCs;
namespace NiDAQ {
    struct Config {
        Config();
        std::string listen;
        YAML::Node devs;
        static Config loadYAML(const char *fname);
    };
}

#endif