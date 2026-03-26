import os
import subprocess
import yaml
import re
from datetime import datetime

proc_filename='cpuinfo_max_freq'

cache_level_filename='level'
cache_size_filename='size'

mem_info_filename='/proc/meminfo'

all_cpus_dir = '/sys/devices/system/cpu/'


def get_cpu_clock(proc_dir):
    try:
        proc_file = os.path.join(proc_dir, proc_filename)
        with open(proc_file,'r') as f:
            string = f.read().strip()
            freq_khz = float(string)
            freq_mhz = freq_khz / 1000
        return freq_mhz
    except Exception:
        return 0.0

def get_cpu_cache(cache_dir):
    try:
        fnl = os.path.join(cache_dir, cache_level_filename)
        fns = os.path.join(cache_dir, cache_size_filename)
        with open(fnl,'r') as f:
            l = f.read()
        with open(fns) as f:
            s = f.read()
        lvl=f'CPU_L{l}Cache'
        reg_exp=re.search('[0-9]+',s)
        sz=int(reg_exp.group())
        return lvl,sz
    except:
        return None, 0

def get_memory_info():
    ram_size = "ERROR"
    ram_freq = "ERROR"
    # Quantidade total
    try:
        with open(mem_info_filename, 'r') as f:
            line = f.readline()  # MemTotal é a primeira linha
            ram_size = line.split(":")[1].strip()
    except Exception:
        pass

    # Clock da memória (dmidecode)
    try:
        # Executa como sudo para pegar o clock
        cmd = "sudo dmidecode -t memory | grep 'Speed' | head -n 1"
        output = subprocess.check_output(cmd, shell=True, text=True)
        if output:
            ram_freq = output.split(":")[1].strip()
    except Exception:
        pass
        
    return ram_size, ram_freq

def get_core_info(cpu_prefix, cache_prefix):
    core_info ={}
    core_info["RAM_Size"],core_info["RAM_Frequency"] = get_memory_info()
    
    cache_dirs = [cd for cd in os.listdir(cache_prefix) if cd.startswith("index") and cd[5:].isdigit()]
    for dir in cache_dirs:
        d = os.path.join(cache_prefix,dir)
        cache_name, cache_value = get_cpu_cache(d)
        core_info[cache_name] = cache_value
    cpu_freq_filename = os.path.join(cpu_prefix)
    frequency = get_cpu_clock(cpu_freq_filename)
    core_info['CPU_ClockMaximumFrequency'] = frequency
    return core_info
    

def main():
    report = []
    cpus=[id for id in os.listdir(all_cpus_dir) if id.startswith('cpu') and id[3:].isdigit()]
    for cpuid in cpus:
        cpu_dir = os.path.join(all_cpus_dir, cpuid)
        cache_dir = os.path.join(cpu_dir,'cache')
        report.append(get_core_info(cpu_dir,cache_dir))


    output_file = "hardware_info.yaml"
    with open(output_file, 'w') as f:
        yaml.dump(report, f, sort_keys=False, default_flow_style=False)
    
    print(f"Relatório YAML gerado com sucesso: {output_file}")

if __name__ == "__main__":
    main()