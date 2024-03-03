obj-$(CONFIG_NET_PICO_SUPPORT_DEVLOOP) += pico_dev_loop.o
obj-y += pico_dev_null.o
obj-y += pico_ethernet.o

obj-$(CONFIG_NET_PICO_SUPPORT_ETH) += pico_arp.o
obj-$(CONFIG_NET_PICO_SUPPORT_ICMP4) += pico_icmp4.o
obj-$(CONFIG_NET_PICO_SUPPORT_IPV4) += pico_ipv4.o
obj-$(CONFIG_NET_PICO_SUPPORT_UDP) += pico_udp.o pico_socket_udp.o
obj-$(CONFIG_NET_PICO_SUPPORT_TCP) += pico_tcp.o pico_socket_tcp.o

obj-$(CONFIG_NET_PICO_SUPPORT_DHCP_CLIENT) += pico_dhcp_client.o \
						pico_dhcp_common.o
obj-$(CONFIG_NET_PICO_SUPPORT_DNS_CLIENT) += pico_dns_client.o pico_dns_common.o
obj-$(CONFIG_NET_PICO_SUPPORT_HTTP_CLIENT) += pico_http_client.o \
						pico_http_util.o
