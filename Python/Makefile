pdapilot.so: pdapilot.o
	gcc -g -L../libsock -shared pdapilot.o -lpisock -o pdapilot.so

pdapilot.o: pdapilot.c
	gcc -g -I/usr/local/include/Py -I../include -c pdapilot.c

clean:
	rm -f *.so *.o *~

