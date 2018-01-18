/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <string>

namespace Config
{
    extern std::string arm7_bios_path;
    extern std::string arm9_bios_path;
    extern std::string firmware_path;
    extern std::string savelist_path;
    extern std::string gba_bios_path;
    extern bool direct_boot_enabled;
    extern bool gba_direct_boot;
    extern bool pause_when_unfocused;

    extern bool bg_enable[4];

    extern int frameskip;
    extern bool enable_framelimiter;

    extern bool hle_bios;
    extern bool test;
};

#endif // CONFIG_HPP
