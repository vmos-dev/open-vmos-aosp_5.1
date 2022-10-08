/* this program is used to test that getaddrinfo() works correctly
 * without a 'hints' argument
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>  /* for printf */
#include <string.h> /* for memset */
#include <netinet/in.h>  /* for IPPROTO_TCP */

#define  SERVER_NAME  "www.android.com"
#define  PORT_NUMBER  "9999"

int main(void)
{
    struct addrinfo  hints;
    struct addrinfo* res;
    int              ret;

    /* first, try without any hints */
    ret = getaddrinfo( SERVER_NAME, PORT_NUMBER, NULL, &res);
    if (ret != 0) {
        printf("first getaddrinfo returned error: %s\n", gai_strerror(ret));
        return 1;
    }

    freeaddrinfo(res);

    /* now try with the hints */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    ret = getaddrinfo( SERVER_NAME, PORT_NUMBER, &hints, &res );
    if (ret != 0) {
        printf("second getaddrinfo returned error: %s\n", gai_strerror(ret));
        return 1;
    }

    return 0;
}
