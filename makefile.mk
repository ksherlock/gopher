CFLAGS += $(DEFINES) -v -w 
OBJS = gopher.o url.o connection.o readline2.o scheme.o ftype.o setftype.o

gopher: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

utest: utest.o url.o scheme.o
	$(CC) $(LDFLAGS) utest.o url.o scheme.o -o $@

dtest: dtest.o dictionary.o 
	$(CC) $(LDFLAGS) dtest.o dictionary.o -o $@


gopher.o: gopher.c url.h connection.h
url.o: url.c url.h
connection.o: connection.c connection.h
readline2.o: readline2.c readline2.h

data.o: data.c data.h
dictionary.o: dictionary.c dictionary.h

setftype.o: setftype.c
scheme.o: scheme.c url.h
ftype.o: ftype.c

# tests
utest.o: utest.c
dtest.o: dtest.c


clean:
	$(RM) *.o *.root

clobber: clean
	$(RM) gopher

