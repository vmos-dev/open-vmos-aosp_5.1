/* this program is used to test UDP networking in Android.
 * used to debug the emulator's networking implementation
 */
#define  PROGNAME      "test_udp"
#define  DEFAULT_PORT  7000

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#define BUFLEN 512
#define NPACK  10

void diep(char *s)
{
    perror(s);
    exit(1);
}

static void
usage(int  code)
{
    printf("usage: %s [options]\n", PROGNAME);
    printf("options:\n");
    printf("    -p<port>  use specific port (default %d)\n", DEFAULT_PORT);
    printf("    -a<inet>  use specific IP address\n");
    printf("    -s        run server (default is client)\n");
    exit(code);
}

int main(int  argc, char**  argv)
{
    int   runServer = 0;
    int   udpPort   = DEFAULT_PORT;
    int   useLocal  = 0;
    int   address   = htonl(INADDR_ANY);

    struct sockaddr_in si_me, si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN];

    while (argc > 1 && argv[1][0] == '-') {
        const char*  optName = argv[1]+1;
        argc--;
        argv++;

        switch (optName[0]) {
            case 'p':
                udpPort = atoi(optName+1);
                if (udpPort < 1024 || udpPort > 65535) {
                    fprintf(stderr, "UDP port must be between 1024 and 65535\n");
                    exit(1);
                }
                break;

            case 's':
                runServer = 1;
                break;

            case 'a':
                if (inet_aton(optName+1, &si_other.sin_addr) == 0)
                    diep("inet_aton");
                address = si_other.sin_addr.s_addr;
                break;

            default:
                usage(1);
        }
    }

    if (runServer) {
        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");

        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family      = AF_INET;
        si_me.sin_port        = htons(udpPort);
        si_me.sin_addr.s_addr = address;
        if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me))==-1)
            diep("bind");

        printf("UDP server listening on %s:%d\n", inet_ntoa(si_me.sin_addr), udpPort);
        for (i=0; i<NPACK; i++) {
        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*)&si_other, (socklen_t*)&slen)==-1)
            diep("recvfrom()");
        printf("Received packet from %s:%d\nData: %s\n\n", 
                inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
        }

        printf("UDP server closing\n");
        close(s);
    }
    else  /* !runServer */
    {
        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
            diep("socket");

        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(udpPort);
        si_other.sin_addr.s_addr = address;

        printf("UDP client sending packets to %s:%d\n", inet_ntoa(si_other.sin_addr), udpPort);

        for (i=0; i<NPACK; i++) {
            printf("Sending packet %d\n", i);
            sprintf(buf, "This is packet %d\n", i);
            if (sendto(s, buf, BUFLEN, 0, (struct sockaddr*)&si_other, slen)==-1)
            diep("sendto()");
        }

        close(s);
        printf("UDP client closing\n");
    }
    return 0;
}
