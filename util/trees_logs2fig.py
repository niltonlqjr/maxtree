import re
import argparse
import networkx as nx
import matplotlib.pyplot as plt

 
def get_field(line, name, tp, sep=','):
    tfield = line.split(name)[1]
    field = tfield.split(sep)[0]
    return tp(field)

parser = argparse.ArgumentParser()
parser.add_argument('file', help = 'filename')
parser.add_argument('--output-dir', '-d', dest='out_dir', default='.', help='output directory')
parser.add_argument('--output-prefix', '-o', dest='output', default='fig', help='output file prefix')
parser.add_argument('--output-ext', '-e', dest='ext', default='.pdf', help='output file extension')
parser.add_argument('--show-values', '-s', dest='show_values', action='store_true', help='')
parser.add_argument('--filter-unimportant-nodes', '-f', dest='filter', action='store_true', help='remove nodes with attribute 0 or 1 that arent fathers on the tree')
args = parser.parse_args()

filter_nodes=args.filter

fn = args.file
nsize = 30
fsize = nsize//10
ext= args.ext
out_dir = args.out_dir
out_prefix = args.output
with open(fn) as f:
    texto = f.read()

lines = texto.split('\n')

all_trees = []
tree_data = []
type_line = ''
iter=0
for l in lines:
    
    insere = False
    info = re.search(r'(BASE BOUNDARY TREE:.*\d+.*\d+.*|TO_MERGE BOUNDARY TREE:.*\d+.*\d+.*|MERGED BOUNDARY TREE:.*\d+.*\d+.*|Final Boundary Tree:.*)',l)
    if info != None:
        data_name = l
        type_line = 'header'
        #print('---->',data_name,'<----')
    else:
        a=re.search(r'\(.*idx:.*-?\d+.*,.*border_lr:.*-?\d+.*,.*gval:.*-?\d+.*,.*attribute:.*-?\d+.*\)',l)
        if a!=None:
            spl=l.split(')')
            if(len(spl) <= 2):
               type_line='data'
        else:
            if type_line!='':
                insere = True
                type_line = ''

    if type_line == 'data':
        p = l.find('(')
        if p != -1:
            tree_data.append(l[p:])
        
    if insere:
        print('tree:')
        print(data_name)
        for node in tree_data:
            print(node)
        all_trees.append({'nome':str(iter)+'_'+data_name,'data':tree_data})
        tree_data = []
        iter+=1

for tdata in all_trees:
    #print(tdata['nome'])
    out_name = out_dir+'/'+out_prefix+'_'+tdata['nome']+'.txt'
    f=open(out_name,'w')
    for d in tdata['data']:
        f.write(d+'\n')
        print(d)
    f.close()
    
