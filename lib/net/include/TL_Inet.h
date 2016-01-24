#include <arpa/inet.h>


#define ntoh_16(x) ntohs(x)
#define hton_16(x) htons(x)

#define ntoh_32(x) ntohl(x)
#define hton_32(x) htonl(x)

#define ntoh_64(x) (((uint64_t)(ntoh_32((uint32_t)((x << 32) >> 32))) << 32) | (uint32_t)ntoh_32(((uint32_t)(x >> 32))))
#define hton_64(x) ntoh_64(x)
