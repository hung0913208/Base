import os

if os.path.exists('./libhello.so'):
    os.rename('./libhello.so', './Hello.so')

import Hello
Hello.Print()
