CFLAGS += $(DEFINES) -v -w 
OBJS = main.o gopher.o url.o connection.o readline2.o scheme.o ftype.o \
       mime.o setftype.o s16debug.o common.o http.o http.utils.o \
       dictionary.o  options.o time.o smb.o

gopher: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

utest: utest.o url.o scheme.o
	$(CC) $(LDFLAGS) -o $@ $<

dtest: dtest.o dictionary.o 
	$(CC) $(LDFLAGS) -o $@ $<


htest: htest.o http.utils.o 
	$(CC) $(LDFLAGS) -o $@ $<

main.o: main.c url.h
url.o: url.c url.h
connection.o: connection.c connection.h
readline2.o: readline2.c readline2.h
common.o: common.c

options.o: options.c options.h


gopher.o: gopher.c url.h connection.h options.h
http.o: http.c url.h connection.h options.h
http.utils.o: http.utils.c
smb.o: smb.c smb.h url.h connection.h options.h

data.o: data.c data.h
dictionary.o: dictionary.c dictionary.h

setftype.o: setftype.c
scheme.o: scheme.c url.h
ftype.o: ftype.c
mime.o: mime.c 

time.o: time.c

s16debug.o: s16debug.c s16debug.h

# tests
utest.o: utest.c
dtest.o: dtest.c
htest.o: htest.c


clean:
	$(RM) *.o *.root

clobber: clean
	$(RM) gopher

