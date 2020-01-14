/* one */
proto:
int nrf_setdnsaddr(const char *dns, size_t len);
usage:
nrf_setdnsaddr("8.8.8.8", strlen("8.8.8.8)"));

/* two */
proto:
int nrf_setdnsaddr(int family, void *in_addr);
usage:
struct nrf_in_addr addr;
nrf_inet_pton(AF_INET, "8.8.8.8", &addr);
nrf_setdnsaddr(AF_INET, &addr);

