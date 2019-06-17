# Always re-run the benchmark.
.PHONY: threshold.png
benchmark.png: threshold threshold.gnuplot
	./threshold > values.dat
	gnuplot threshold.gnuplot

calibration: threshold.c cacheutils.h
	gcc -std=gnu11 -O0 -o $@ $@.c
