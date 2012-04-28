#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#ifndef __TCPIP__
#include <tcpip.h>
#endif

enum {
  kConnectionStateDNR = 1,
  kConnectionStateConnecting,
  kConnectionStateConnected,
  kConnectionStateDisconnecting,
  kConnectionStateDisconnected,
  kConnectionStateError
};

typedef struct Connection {
  Word memID;
  Word ipid;
  Word terr;
  Word state;
  dnrBuffer dnr;
  Word port;
} Connection;



void ConnectionInit(Connection *, Word memID);

Word ConnectionOpen(Connection *, const char *host, Word port);
Word ConnectionOpenC(Connection *, const char *host, Word port);
Word ConnectionOpenGS(Connection *, const GSString255 *host, Word port);

Word ConnectionClose(Connection *);
Word ConnectionPoll(Connection *);


#endif
