all: pngm geometry

pngm: pngm.c
	gcc pngm.c -Wall -Werror -O2 -o pngm

geometry: geometry.c
	gcc geometry.c -Wall -Werror -O2 -o geometry
