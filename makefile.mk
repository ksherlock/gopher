CFLAGS += $(DEFINES) -v -w 
OBJS = main.o gopher.o url.o connection.o readline2.o scheme.o ftype.o setftype.o \
       s16debug.o common.o http.0.9.o http.1.0.o dictionary.o

gopher: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

utest: utest.o url.o scheme.o
	$(CC) $(LDFLAGS) utest.o url.o scheme.o -o $@

dtest: dtest.o dictionary.o 
	$(CC) $(LDFLAGS) dtest.o dictionary.o -o $@

main.o: main.c url.h
url.o: url.c url.h
connection.o: connection.c connection.h
readline2.o: readline2.c readline2.h
common.o: common.c

gopher.o: gopher.c url.h connection.h
http.0.9.o: http.0.9.c
http.1.0.o: http.1.0.c

data.o: data.c data.h
dictionary.o: dictionary.c dictionary.h

setftype.o: setftype.c
scheme.o: scheme.c url.h
ftype.o: ftype.c

s16debug.o: s16debug.c s16debug.h

# tests
utest.o: utest.c
dtest.o: dtest.c


clean:
	$(RM) *.o *.root

clobber: clean
	$(RM) gopher

