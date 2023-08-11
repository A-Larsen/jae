jae: jae.c term.c perror.h
	gcc -g $^ -o $@

clean:
	rm -rf jae

.PHONY: clean
