# -*- coding: utf-8 -*-
"""
Created on Sat May 30 14:56:08 2015

@author: Asus
"""

# -*- coding: utf-8 -*-
"""
Created on Wed May 27 10:35:38 2015

@author: Asus
"""
import geojson
from geojson import Polygon, dumps
from geojson import Feature
from geojson import FeatureCollection
import sys

if (len(sys.argv)!=4):
    print("Usage:synth_data.py num_rectangles xsize ysize outfile_name")

xsize = int(sys.argv[1])
ysize = int(sys.argv[2])
outfile = str(sys.argv[3])



def make_square(coords): # coords is a list of the four corners of your square
    
    thesquare = Polygon([coords])    
    
    newsquare = geojson.utils.map_coords(lambda x: x+0.00000001, thesquare)
    return newsquare
    
def one_square(x_offset, y_offset, square_size, numpoints):
    xstart = 8*x_offset
    ystart = 8*y_offset
    
    
    
    bottom_left = ((8 - square_size)/2 + xstart, (8 - square_size)/2 + ystart)
    top_left = (bottom_left[0], bottom_left[1] + square_size)
    top_right = (bottom_left[0] + square_size, bottom_left[1] + square_size)
    bottom_right = (bottom_left[0] + square_size, bottom_left[1])
    
    step = 0
    square = [bottom_left]
    if numpoints!=0:
        step = square_size/(numpoints+1)
    
    
    
    for i in xrange(numpoints):
        point = (bottom_left[0], bottom_left[1] + step*(i+1))
        square.append(point)
    
    square.append(top_left)
    
    for i in xrange(numpoints):
        point = (top_left[0] + step*(i+1), top_left[1])
        square.append(point)  
    
    square.append(top_right)
    
    for i in xrange(numpoints):
        point = (top_right[0], top_right[1] - step*(i+1))
        square.append(point)
    
    square.append(bottom_right)
    
    for i in xrange(numpoints):
        point = (bottom_right[0] - step*(i+1), bottom_right[1])
        square.append(point)
    
    square.append(bottom_left)
    
    return square
    
    
def getCoords(imsizex, imsizey, square_size, numpoints1):
    coordlist = [];    
    
    for i in xrange(imsizex):
        for j in xrange(imsizey):
            square = one_square(i, j, square_size, numpoints1)
            coordlist.append(square)
    return coordlist

    
def make_feature_collection(imsizex, imsizey, coordlist):
    features = [];
    flag = 1
    for i in xrange(imsizex):
        flag = -1*flag
        if flag == -1:
            therange = xrange(imsizey - 1, -1, -1);
        else:
            therange = xrange(imsizey);
        for j in therange:
        
            square = make_square(coordlist[i*imsizey + j])
            feature = Feature(geometry=square, properties={"STATE":str((imsizey*i + j)%2 + 1)})
            features.append(feature)
    
    
    return FeatureCollection(features)


def make_geojson(imsizex, imsizey, outfile, numpoints, square_size):
    coordlist = getCoords(imsizex, imsizey, square_size, numpoints)
    collection = make_feature_collection(imsizex, imsizey, coordlist)
    out = dumps(collection)
    f = open(outfile, 'w')
    f.write(out)
    return collection
    
make_geojson(xsize, ysize, outfile, 1, 2)