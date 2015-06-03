obj-$(CONFIG_NET)	+= eth.o
obj-$(CONFIG_NET)	+= net.o
obj-$(CONFIG_NET_NFS)	+= nfs.o
obj-$(CONFIG_NET_DHCP)	+= dhcp.o
obj-$(CONFIG_CMD_PING)	+= ping.o
obj-$(CONFIG_NET_RESOLV)+= dns.o
obj-$(CONFIG_NET_NETCONSOLE) += netconsole.o
obj-$(CONFIG_NET_IFUP)	+= ifup.o

obj-$(CONFIG_NET_PICOTCP) += picotcp/
obj-$(CONFIG_NET_PICOTCP) += picotcp.o

obj-$(CONFIG_CMD_PICOTCP_TEST_IPV4) += picotcp_test_ipv4.o
obj-$(CONFIG_CMD_PICOTCP_DHCP_CLIENT) += picotcp_dhclient.o
obj-$(CONFIG_CMD_PICOTCP_IFCONFIG) += picotcp_ifconfig.o
obj-$(CONFIG_CMD_PICOTCP_PING) += picotcp_ping.o
obj-$(CONFIG_CMD_PICOTCP_ROUTE) += picotcp_route.o

ifeq ($(CONFIG_NET_PICO_SUPPORT_UDP),y)
CPPFLAGS += -DPICO_SUPPORT_UDP
endif
