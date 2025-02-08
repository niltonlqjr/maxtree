import skimage as sk
import numpy as np
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file', help = 'filename')
parser.add_argument('--output', '-o', dest='output', default='fig.png', help='output file')
args = parser.parse_args()


arr=[]

with open(args.file) as f:
    lines = f.readlines()
    for line in lines:
        spl=line.split()
        arr.append([int(x) for x in spl])

img = np.array(arr,dtype=np.uint8)

sk.io.imsave(args.output,img)