pdapilot.so: pdapilot.o
	gcc -L../ -shared pdapilot.o -lpisock -o pdapilot.so

pdapilot.o: pdapilot.c
	gcc -I/usr/local/include/Py -I../include -c pdapilot.c

clean:
	rm -f *.so *.o *~

