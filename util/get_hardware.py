import os
import subprocess
import yaml
import re
from datetime import datetime
import argparse

parser = argparse.ArgumentParser('collect and generate a yaml file with some hardware information',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-c', '--cpu-dir', dest='all_cpus_dir', type=str, default='/sys/devices/system/cpu/',
                    help = 'directory used to search processor (and cache) informarions')
parser.add_argument('-f', '--frequency-filename', dest='freq_filename', type=str, default='cpufreq/scaling_max_freq',
                    help = 'filename that stores the frequency of CPU')
parser.add_argument('-s', '--cache-size-filename', dest='cache_size_filename', type=str, default='size',
                    help = 'filename that stores the size of cache memory of CPU')
parser.add_argument('-l', '--cache-level-filename', dest='cache_level_filename', type=str, default='level',
                    help = 'filename that stores memory cache level')
parser.add_argument('-t', '--cache-type-filename', dest='cache_type_filename', type=str, default='type',
                    help = 'filename that stores memory cache type')
parser.add_argument('--cpu-dir-prefix-name', dest = 'cpu_dir_prefix', type=str, default='cpu[0-9]+',
                    help = 'prefix of folder name inside cpu-dir, so, the search of cpu files will be on cpu-dir/cpu-dir-prefix')
parser.add_argument('--cache-dir-prefix-name', dest = 'cache_dir_prefix', type=str, default='cache/index[0-9]+',
                    help = 'prefix of folder name inside core directory (core directory is cpu-dir/cpu_index/), '
                         + 'so, the search for cache files will be on cpu-dir/cpu-dir-prefix[0-9]+/cache-dir-prefix')
parser.add_argument('-o', '--output', dest = 'output_file', type=str, default='hardware.yaml',
                    help = 'output filenam')

args=parser.parse_args()

all_cpus_dir = args.all_cpus_dir

freq_filename=args.freq_filename

cache_level_filename=args.cache_level_filename
cache_size_filename=args.cache_size_filename
cache_type_filename=args.cache_type_filename

mem_info_filename='/proc/meminfo'

cache_dir_prefix = args.cache_dir_prefix
cpu_dir_prefix = args.cpu_dir_prefix

output_file = args.output_file

def get_cpu_clock(proc_dir):
    try:
        proc_file = os.path.join(proc_dir, freq_filename)
        print(proc_file)
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
        fnct = os.path.join(cache_dir, cache_type_filename)
        with open(fnl) as f:
            l = int(f.read())
        with open(fns) as f:
            s = f.read().strip()
        with open(fnct) as f:
            t = f.read().strip()
        lvl=f'CPU_L{l}Cache{t}'
        sz=int(re.search('[0-9]+',s).group())
        
        return lvl,sz
    except:
        return None, 0

def get_memory_info():
    ram_size = 0
    ram_freq = 0
    # Quantidade total
    try:
        with open(mem_info_filename, 'r') as f:
            line = f.readline()  # MemTotal é a primeira linha
            s_ram_size = line.split(":")[1].strip()
            s_ram_size = re.search('[0-9]+',s_ram_size).group()
            ram_size = int(s_ram_size)
    except Exception:
        pass

    # Clock da memória (dmidecode)
    try:
        # Executa como sudo para pegar o clock
        cmd = "sudo dmidecode -t memory | grep 'Speed' | head -n 1"
        output = subprocess.check_output(cmd, shell=True, text=True)
        if output:
            s_ram_freq = output.split(":")[1].strip()
            s_ram_freq = re.search('[0-9]+',s_ram_freq).group()
            ram_freq = int(s_ram_freq)
    except Exception:
        pass
    
    
    

    return ram_size, ram_freq

def get_core_info(cpu_prefix, cache_regex):
    core_info ={}
    core_info["RAM_Size"],core_info["RAM_Frequency"] = get_memory_info()
    cache_prefix, last_prefix_val = os.path.split(cache_regex)
    cache_fulldir = os.path.join(cpu_prefix,cache_prefix)
    print(f'last_prefix_val:{last_prefix_val}')
    print(f'cache_prefix:{cache_prefix}')
    cache_dirs = [cd for cd in os.scandir(cache_fulldir) if re.search(last_prefix_val, cd.name)]
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
    print('getting cpu ids:')
    last_prefix_val = os.path.split(cpu_dir_prefix)[1]
    print (last_prefix_val)
    cpus=[id.name for id in os.scandir(all_cpus_dir) if re.search(last_prefix_val, id.name)]
    for cpuid in cpus:
        cpu_dir = os.path.join(all_cpus_dir, cpuid)
        print(f'reading data for core: {cpuid} at directory {cpu_dir}')
        
        cache_dir = os.path.join(cpu_dir, cache_dir_prefix)
        print(f'reading data for core: {cpuid} Cache Memory at directory {cache_dir}')

        report.append(get_core_info(cpu_dir,cache_dir))



    with open(output_file, 'w') as f:
        yaml.dump(report, f)
    
    print(f"Relatório YAML gerado com sucesso: {output_file}")

if __name__ == "__main__":
    main()