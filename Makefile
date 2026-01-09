all: src examples

src:
	$(MAKE) -C src all

examples:
	$(MAKE) -C examples all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C examples clean

.PHONY:	src examples
