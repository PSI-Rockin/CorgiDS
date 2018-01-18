/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "config.hpp"

namespace Config
{
    std::string arm7_bios_path;
    std::string arm9_bios_path;
    std::string firmware_path;
    std::string savelist_path;
    std::string gba_bios_path;
    bool direct_boot_enabled;
    bool gba_direct_boot;
    bool pause_when_unfocused;

    bool bg_enable[4];

    int frameskip;
    bool enable_framelimiter;

    bool hle_bios;
    bool test;
};
