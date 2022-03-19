#!/usr/bin/env python3
import os, glob
# import argparse

# parser = argparse.ArgumentParser("Filter the construction comments from the original text of clox")
# parser.add_argument('-i', type=str, dest='file', required=True)
# args = parser.parse_args()

def strip_one(infile) :

    print(infile)
    outfile = infile + '.tmp'

    with open(infile, 'r') as inf:
        with open(outfile, 'w') as outf:
            lines = inf.readlines()
            for line in lines:
                if not (line.strip().startswith('//<') or line.strip().startswith('//>')):
                    outf.write(line)

    os.rename(infile, infile+'.old')
    os.rename(outfile, infile)

for name in glob.glob('../src/*.c'):
    strip_one(name)

for name in glob.glob('../src/*.h'):
    strip_one(name)

for name in glob.glob('../src/*.old'):
    os.rename(name, name.replace('src', 'obj'))