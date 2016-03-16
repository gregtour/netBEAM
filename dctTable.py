#!/usr/bin/python
# Generate DCT tables.
from math import pi, cos
def outputTables():
	total = 64
	xwidth = 8
	i = 0
	x = 0
	y = 0
	print "// DCT { 1/4 x a(u) x a(v) x cos((2x+1)u pi / 16) x cos((2y+1)v pi / 16) } x 1000."
	print "int cofactor[" + str(total) + "][8][8] = {"
	while i < total:
		i = i + 1
		print "// " + str(y) + "," + str(x)
		print "  {"
		for yy in range(8):
			print "    {",
			for xx in range(8):
				value = cos((2*xx+1)*x*pi/16.0) * cos((2*yy+1)*y*pi/16.0)
				value = value / 4
				if x == 0:
					value = value * 0.7071
				if y == 0:
					value = value * 0.7071
				value = int(value * 1000)
				print str(value) + ",",
			print "},"
		print "  },"
		x = x + 1
		if x >= xwidth:
			x = 0
			y = y + 1
	print "};"
	
