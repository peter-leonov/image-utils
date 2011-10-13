all: pngm jfifwh

pngm: pngm.c
	gcc pngm.c -Wall -Werror -O2 -o pngm

jfifwh: jfifwh.c
	gcc jfifwh.c -Wall -Werror -O2 -o jfifwh
