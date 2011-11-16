#!/usr/bin/env python
from pprint import pprint as pp
import numpy as np
import time
import sys

CPHVB   = True
size    = 4

try:
    CPHVB   = int(sys.argv[1])
    size    = int(sys.argv[2])
except:
    pass

def main():

    x = np.array([range(1,size+1)]*size, dtype=np.float32, dist=CPHVB)
    y = np.array([range(1,size+1)]*size, dtype=np.float32, dist=CPHVB)
    z = np.empty((size,size), dtype=np.float32, dist=CPHVB)

    print np.add( x, y, z )

if __name__ == "__main__":
    main()
