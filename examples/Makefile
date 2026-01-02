
all: helloworld mandelbrot

helloworld: helloworld.c
	cl65 -t none --config ./64xpi.cfg  -O -o helloworld.bin helloworld.c

mandelbrot: mandelbrot.c
	cl65 -t none --config ./64xpi.cfg  -O -o mandelbrot.bin mandelbrot.c

clean:
	rm -f *.o *.bin
