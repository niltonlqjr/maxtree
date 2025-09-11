import argparse
import networkx as nx
import matplotlib.pyplot as plt

 
def get_field(line, name, tp, sep=','):
    tfield = line.split(name)[1]
    field = tfield.split(sep)[0]
    return tp(field)

parser = argparse.ArgumentParser()
parser.add_argument('file', help = 'filename')
parser.add_argument('--output', '-o', dest='output', default='fig.pdf', help='output file')
parser.add_argument('--show-values', '-s', dest='show_values', action='store_true', help='')
args = parser.parse_args()

fn = args.file
with open(fn) as f:
    texto = f.read()

lines = texto.split('\n')

lines_data = []

for l in lines:
    p = l.find('(')
    if l!= '':
        lines_data.append(l[p:])

g = nx.DiGraph()

for ld in lines_data:
    idx = get_field(ld, "idx:", int)
    gval = get_field(ld, "gval:", int)
    attribute = get_field(ld, "attribute:", int, sep = ')')
    parent = get_field(ld, "bound_parent:", int)
    g.add_node(idx, gval=gval, attr=attribute, par = parent)

for u in g.nodes(data = True):
    idx = u[0]
    parent = u[1]["par"]
    if(parent != -1):
        g.add_edge(parent,idx)


pdot=nx.drawing.nx_pydot.pydot_layout(g, prog='dot')

nx.draw(g, with_labels=True, pos=pdot, node_size=250, font_size=8, node_color="#8a8a8a")
out_name = args.output
plt.savefig(out_name)