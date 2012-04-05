CFLAGS += $(DEFINES) -v -w 
OBJS = gopher.o url.o connection.o

gopher: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

gopher.o: gopher.c url.h connection.h
url.o: url.c url.h
connection.o: connection.c connection.h
data.o: data.c data.h
dictionary.o: dictionary.c dictionary.h

clean:
	$(RM) *.o *.root

clobber: clean
	$(RM) gopher

