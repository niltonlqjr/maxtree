#include "custom.hpp"

std::vector<std::unordered_map<std::string, TWorkerAttr>> *parse_hw_config(std::string yaml_filename){
    std::string yml_buf, l;    
    auto ret = new std::vector<std::unordered_map<std::string, TWorkerAttr>>();
    
    std::ifstream f(yaml_filename.c_str());
    while(!f.eof()){
        std::getline(f, l);
        
        if(!is_blank(l)){
            yml_buf += l + "\n";
        }
    }
    f.close();

    ryml::Tree tree = ryml::parse_in_place(yml_buf.data());
    ryml::ConstNodeRef root = tree.rootref();
    for(auto core: root){
        
        std::unordered_map<std::string, TWorkerAttr> c;
        TWorkerAttr freq;
        TWorkerAttr l1d ;
        TWorkerAttr l1i ;
        TWorkerAttr l2 ;
        TWorkerAttr l3 ;
        TWorkerAttr ram_size;
        TWorkerAttr ram_freq;

        try{
            c4::atod(core["CPU_ClockMaximumFrequency"].val(), &freq);
        }catch(...){
            freq = TWorkerAttr_NULL;
        }

        try{
            c4::atod(core["CPU_L1CacheData"].val(), &l1d);
        }catch(...){
             l1d = TWorkerAttr_NULL;
        }
            
        try{
            c4::atod(core["CPU_L1CacheInstruction"].val(), &l1i);
        }catch(...){
            l1i = TWorkerAttr_NULL;
        }
        
        try{
            c4::atod(core["CPU_L2CacheUnified"].val(), &l2);
        }catch(...){
            l2 = TWorkerAttr_NULL;
        }
 
        try{
            c4::atod(core["CPU_L3CacheUnified"].val(), &l3);
        }catch(...){
            l3 = TWorkerAttr_NULL;
        }
            
        try{
            c4::atod(core["RAM_Size"].val(), &ram_size);
        }catch(...){
             ram_size = TWorkerAttr_NULL;
        }
            
        try{
            c4::atod(core["RAM_Frequency"].val(), &ram_freq);
        }catch(...){
            ram_freq = TWorkerAttr_NULL;
        }
            

        c["CPUMHZ"] = freq / 1000; // in GHz
        c["L1"] = l1d+l1i ;
        c["L2"] = l2;
        c["L3"] = l3;
        c["CACHE"] = (l1d+l1i+l2+l3) / 1024; // cache in MB 
        c["RAMSIZE"] = ram_size / (1024*1024); // ram memory in GB
        c["RAMFREQ"] = ram_freq / 1000; // in GHz
        ret->push_back(c);
    }
    return ret;
}


Tprocess_power calculate_process_power(std::unordered_map<std::string, TWorkerAttr> *hardware_attributes){
    TWorkerAttr proccessor_power;
    TWorkerAttr ram_power;
    TWorkerAttr cache_power;
    TWorkerAttr ram_freq = hardware_attributes->at("RAMFREQ");
    proccessor_power = hardware_attributes->at("MHZ");
    cache_power = hardware_attributes->at("CACHE");
    ram_power = hardware_attributes->at("RAMSIZE") * ram_freq != 0? ram_freq : 1;

    return proccessor_power + ram_power + cache_power;
 }