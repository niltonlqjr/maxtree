import skimage as sk
import numpy as np

img = np.array([[  1,  1,100,100,200],
                [  1,100,100,100,200],
                [  1,  1,  1,100,200],
                [  1,200,200,200,200]],
               dtype=np.uint8)

sk.io.imsave('teste_maxtree.png',img)