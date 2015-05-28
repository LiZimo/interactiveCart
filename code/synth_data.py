# -*- coding: utf-8 -*-
"""
Created on Wed May 27 10:35:38 2015

@author: Asus
"""
from geojson import Polygon, dumps
from geojson import Feature
from geojson import FeatureCollection
from random import uniform
import sys
from decimal import *

getcontext().prec = 20

if (len(sys.argv)!=5):
    print("Usage:synth_data.py num_rectangles xsize ysize outfile_name")

numsquares = int(sys.argv[1])
xsize = int(sys.argv[2])
ysize = int(sys.argv[3])
outfile = str(sys.argv[4])



def make_square(coords): # coords is a list of the four corners of your square
    
    
    return Polygon([coords])
    

def getCoords(numsquares, imsizex, imsizey, numpoints):
    coordlist = [];    
    
    xmarks = [-imsizex/2, imsizex/2];
    for i in range(numsquares - 1):
        xmarks.append(uniform(-imsizex/2, imsizex/2))
    xmarks.sort()
    
    for i in range(numsquares):
        xstart = xmarks[i]
        xend = xmarks[i+1]
        
        bottom_l = (xstart, -imsizey/2)
        top_l = (xstart, imsizey/2)
        top_r = (xend, imsizey/2)
        bot_r = (xend, -imsizey/2)
        
        square = [bottom_l]
        for i in range(numpoints):
            step = imsizey/numpoints
            point = (xstart, step*i - imsizey/2)
            square.append(point)
        square.append(top_l)
        
        for i in range(numpoints):
            step = (xend-xstart)/numpoints
            point = (step*i - xstart, imsizey)
            square.append(point)
        square.append(top_r)
        
        for i in range(numpoints):
            step = imsizey/numpoints
            point = (xend, imsizey/2 - step*i)
            square.append(point)
        square.append(bot_r)
        
        for i in range(numpoints):
            step = (xend-xstart)/numpoints
            point = (xend - step*i, -imsizey/2)
        square.append(bottom_l)
    
        
        coordlist.append(square)
    
    return coordlist
    
def make_feature_collection(coordlist):
    features = [];
    for i in range(len(coordlist)):
        
        square = make_square(coordlist[i])
        feature = Feature(geometry=square, properties={"STATE":str(i+1)})
        features.append(feature)
    
    
    return FeatureCollection(features)


def make_geojson(numsquares, imsizex, imsizey, outfile, numpoints):
    coordlist = getCoords(numsquares, float(imsizex)+0.0000001, float(imsizey)+0.0000001, numpoints)
    collection = make_feature_collection(coordlist)
    out = dumps(collection)
    f = open(outfile, 'w')
    f.write(out)
    return collection
    
make_geojson(numsquares, xsize, ysize, outfile, 100)