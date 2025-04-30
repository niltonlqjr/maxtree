import skimage as sk
import numpy as np
import argparse



def apply_threshold(img, th):
    new = img >= th
    int_vec = np.vectorize(int)
    return np.array(int_vec(new), dtype=np.uint8)


parser = argparse.ArgumentParser()
parser.add_argument('file', help = 'filename')
parser.add_argument('--output', '-o', dest='output', default='fig', help='output file prefix')
parser.add_argument('--ext', '-e', dest='ext', default='png', help='output format')
args = parser.parse_args()


out_prefix = args.output
out_ext = args.ext

img = sk.io.imread(args.file)

print(img)

dec={}
old = apply_threshold(img,0)
dec[0] = old


for i in range(1,256):
    new = apply_threshold(img, i)
    d = new != old
    if len(np.extract(d, d)) > 0:
        
        dec[i] = new
        old = new


print('writing files')

for k in dec:
    fname = f'{out_prefix}_{k}.{out_ext}'
    print(fname)
    print(dec[k])
    im_save = dec[k]*255
    sk.io.imsave(fname,im_save,check_contrast=False)
