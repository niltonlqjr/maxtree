#include <unordered_map>
#include <string>
#include <vector>
#include <ryml_std.hpp>
#include <ryml.hpp>

#include "const_enum_define.hpp"
#include "utils.hpp"

#ifndef __CUSTOM_HPP__
#define __CUSTOM_HPP__

std::vector<std::unordered_map<std::string, TWorkerAttr>> *parse_hw_config(std::string yaml_filename);
Tprocess_power calculate_process_power(std::unordered_map<std::string, TWorkerAttr> *hardware_attributes);

#endif