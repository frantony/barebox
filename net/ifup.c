// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* ifup.c - bring up network interfaces */

#define pr_fmt(fmt)  "ifup: " fmt

#include <environment.h>
#include <command.h>
#include <common.h>
#include <complete.h>
#include <getopt.h>
#include <dhcp.h>
#include <net.h>
#include <fs.h>
#include <globalvar.h>
#include <string.h>
#include <driver.h>
#include <init.h>
#include <magicvar.h>
#include <linux/stat.h>

static int eth_discover(char *file)
{
	struct stat s;
	int ret;

	ret = stat(file, &s);
	if (ret) {
		ret = 0;
		goto out;
	}

	ret = run_command(file);
	if (ret) {
		pr_err("Running '%s' failed with %d\n", file, ret);
		goto out;
	}

out:
	free(file);

	return ret;
}

static int eth_discover_ethname(const char *ethname)
{
	return eth_discover(basprintf("/env/network/%s-discover", ethname));
}

static int eth_discover_file(const char *filename)
{
	return eth_discover(basprintf("/env/network/%s", filename));
}

static int source_env_network(struct eth_device *edev)
{
	char *vars[] = {
		"ipaddr",
		"netmask",
		"gateway",
		"serverip",
		"ethaddr",
		"ip",
		"linuxdevname",
	};
	IPaddr_t ipaddr, netmask, gateway, serverip;
	unsigned char ethaddr[6];
	char *file, *cmd;
	const char *ethaddrstr, *modestr, *linuxdevname;
	int ret, mode, ethaddr_valid = 0, i;
	struct stat s;

	file = basprintf("/env/network/%s", edev->devname);
	ret = stat(file, &s);
	if (ret) {
		free(file);
		return 0;
	}

	dev_info(&edev->dev, "/env/network/%s is deprecated.\n"
		 "Use nv.dev.%s.* nvvars to configure your network device instead\n",
		 edev->devname, edev->devname);

	env_push_context();

	for (i = 0; i < ARRAY_SIZE(vars); i++)
		unsetenv(vars[i]);

	cmd = basprintf("source /env/network/%s", edev->devname);
	ret = run_command(cmd);
	if (ret) {
		pr_err("Running '%s' failed with %d\n", cmd, ret);
		goto out;
	}

	ipaddr = getenv_ip("ipaddr");
	netmask = getenv_ip("netmask");
	gateway = getenv_ip("gateway");
	serverip = getenv_ip("serverip");
	linuxdevname = getenv("linuxdevname");
	ethaddrstr = getenv("ethaddr");
	if (ethaddrstr && *ethaddrstr) {
		ret = string_to_ethaddr(ethaddrstr, ethaddr);
		if (ret) {
			dev_err(&edev->dev, "Cannot parse ethaddr \"%s\"\n", ethaddrstr);
			ret = -EINVAL;
			goto out;
		}
		ethaddr_valid = 1;
	}

	modestr = getenv("ip");
	if (!modestr) {
		dev_err(&edev->dev, "No mode specified in \"ip\" variable\n");
		ret = -EINVAL;
		goto out;
	}

	if (!strcmp(modestr, "static")) {
		mode = ETH_MODE_STATIC;
	} else if (!strcmp(modestr, "dhcp")) {
		mode = ETH_MODE_DHCP;
	} else {
		dev_err(&edev->dev, "Invalid ip mode \"%s\" found\n", modestr);
		ret = -EINVAL;
		goto out;
	}

	edev->global_mode = mode;

	if (ethaddr_valid)
		memcpy(edev->ethaddr, ethaddr, 6);

	if (mode == ETH_MODE_STATIC) {
		edev->ipaddr = ipaddr;
		edev->netmask = netmask;
		if (gateway)
			net_set_gateway(gateway);
		if (serverip)
			net_set_serverip(serverip);
	}

	if (linuxdevname) {
		free(edev->linuxdevname);
		edev->linuxdevname = xstrdup(linuxdevname);
	}

	ret = 0;

out:
	env_pop_context();
	free(cmd);
	free(file);

	return ret;
}

static void set_linux_bootarg(struct eth_device *edev)
{
	char *bootarg;
	if (edev->global_mode == ETH_MODE_STATIC) {
		IPaddr_t serverip;
		IPaddr_t gateway;

		serverip = net_get_serverip();
		gateway = net_get_gateway();

		bootarg = basprintf("ip=%pI4:%pI4:%pI4:%pI4::%s:",
				&edev->ipaddr,
				&serverip,
				&gateway,
				&edev->netmask,
				edev->linuxdevname ? edev->linuxdevname : "");
		dev_set_param(&edev->dev, "linux.bootargs", bootarg);
		free(bootarg);
	} else if (edev->global_mode == ETH_MODE_DHCP) {
		bootarg = basprintf("ip=::::%s:%s:dhcp",
				barebox_get_hostname(),
				edev->linuxdevname ? edev->linuxdevname : "");
		dev_set_param(&edev->dev, "linux.bootargs", bootarg);
		free(bootarg);
	}
}

static int ifup_edev_conf(struct eth_device *edev, unsigned flags)
{
	int ret;

	if (edev->global_mode == ETH_MODE_DHCP) {
		if (IS_ENABLED(CONFIG_NET_DHCP)) {
			ret = dhcp(edev, NULL);
		} else {
			dev_err(&edev->dev, "DHCP support not available\n");
			ret = -ENOSYS;
		}
		if (ret)
			return ret;
	}

	set_linux_bootarg(edev);

	edev->ifup = true;

	return 0;
}

int ifup_edev(struct eth_device *edev, unsigned flags)
{
	int ret;

	if (edev->global_mode == ETH_MODE_DISABLED) {
		edev->ipaddr = 0;
		edev->netmask = 0;
		edev->ifup = false;
		return 0;
	}

	if (edev->ifup) {
		if (flags & IFUP_FLAG_FORCE)
			edev->ifup = false;
		else
			return 0;
	}

	ret = source_env_network(edev);
	if (ret)
		return ret;

	ret = eth_open(edev);
	if (ret)
		return ret;

	if (flags & IFUP_FLAG_SKIP_CONF)
		return 1;

	return ifup_edev_conf(edev, flags);
}

void ifdown_edev(struct eth_device *edev)
{
	eth_close(edev);
	edev->ifup = false;
}

int ifup(const char *ethname, unsigned flags)
{
	struct eth_device *edev;
	int ret;

	ret = eth_discover_ethname(ethname);
	if (ret)
		return ret;

	edev = eth_get_byname(ethname);
	if (!edev)
		return -ENODEV;

	return ifup_edev(edev, flags);
}

int ifdown(const char *ethname)
{
	struct eth_device *edev;

	edev = eth_get_byname(ethname);
	if (!edev)
		return -ENODEV;

	ifdown_edev(edev);

	return 0;
}

static int net_ifup_force_detect;

static bool ifup_edev_need_conf(struct eth_device *edev)
{
	return edev->active && !edev->ifup &&
		edev->global_mode != ETH_MODE_DISABLED;
}

static int __ifup_all_parallel(unsigned flags)
{
	struct eth_device *edev;
	unsigned netdev_count = 0;
	u64 start;
	int ret;

	for_each_netdev(edev) {
		ret = ifup_edev(edev, flags | IFUP_FLAG_SKIP_CONF);
		if (ret == 1)
			netdev_count++;
	}

	start = get_time_ns();
	while (netdev_count && !is_timeout(start, PHY_AN_TIMEOUT * SECOND)) {
		for_each_netdev(edev) {
			if ((flags & IFUP_FLAG_UNTIL_NET_SERVER) && net_get_server())
				return 0;

			if (ctrlc())
				return -EINTR;

			if (!ifup_edev_need_conf(edev))
				continue;

			ret = eth_carrier_poll_once(edev);
			if (ret)
				continue;

			ifup_edev_conf(edev, flags);
			if (!edev->ifup)
				continue;

			netdev_count--;
		}
	}

	return 0;
}

static int __ifup_all_sequence(unsigned flags)
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		if ((flags & IFUP_FLAG_UNTIL_NET_SERVER) && net_get_server())
			return 0;

		if (ctrlc())
			return -EINTR;

		ifup_edev(edev, flags);
	}

	return 0;
}

int ifup_all(unsigned flags)
{
	DIR *dir;
	struct dirent *d;

	dir = opendir("/env/network");
	if (dir) {

		while ((d = readdir(dir))) {
			if (*d->d_name == '.')
				continue;
			if (!strstr(d->d_name, "-discover"))
				continue;

			eth_discover_file(d->d_name);
		}
	}

	closedir(dir);

	if ((flags & IFUP_FLAG_FORCE) || net_ifup_force_detect ||
	    list_empty(&eth_class.devices))
		device_detect_all();

	/*
	 * In the future, we may add an iproute -r option that tries to
	 * resolve $global.net.server using each interface. For now,
	 * we only support the special case of $global.net.server being
	 * empty, i.e. the first DHCP lease setting $global.net.server
	 * will be what we're going with.
	 */
	if (net_get_server() && !net_get_gateway())
		flags &= ~IFUP_FLAG_UNTIL_NET_SERVER;

	if (flags & IFUP_FLAG_PARALLEL)
		return __ifup_all_parallel(flags);
	else
		return __ifup_all_sequence(flags);
}

void ifdown_all(void)
{
	struct eth_device *edev;

	for_each_netdev(edev)
		ifdown_edev(edev);
}

static int ifup_all_init(void)
{
	globalvar_add_simple_bool("net.ifup_force_detect", &net_ifup_force_detect);

	return 0;
}
late_initcall(ifup_all_init);

BAREBOX_MAGICVAR(global.net.ifup_force_detect,
                 "net: force detection of devices on ifup -a");

#if IS_ENABLED(CONFIG_NET_CMD_IFUP)

static int do_ifup(int argc, char *argv[])
{
	int opt;
	unsigned flags = IFUP_FLAG_PARALLEL;
	int all = 0;

	while ((opt = getopt(argc, argv, "asf1")) > 0) {
		switch (opt) {
		case 'f':
			flags |= IFUP_FLAG_FORCE;
			break;
		case '1':
			flags |= IFUP_FLAG_UNTIL_NET_SERVER;
			break;
		case 's':
			flags &= ~IFUP_FLAG_PARALLEL;
			break;
		case 'a':
			all = 1;
			break;
		}
	}

	if (all)
		return ifup_all(flags);

	if (argc == optind)
		return COMMAND_ERROR_USAGE;

	return ifup(argv[optind], flags);
}



BAREBOX_CMD_HELP_START(ifup)
BAREBOX_CMD_HELP_TEXT("Network interfaces are configured with a NV variables or a")
BAREBOX_CMD_HELP_TEXT("/env/network/<intf> file. See Documentation/user/networking.rst")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",  "bring up all interfaces")
BAREBOX_CMD_HELP_OPT ("-s",  "bring up interfaces in sequence, not in parallel")
BAREBOX_CMD_HELP_OPT ("-f",  "Force. Configure even if ip already set")
BAREBOX_CMD_HELP_OPT ("-1",  "Early exit if DHCP sets $global.net.server")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ifup)
	.cmd		= do_ifup,
	BAREBOX_CMD_DESC("bring a network interface up")
	BAREBOX_CMD_OPTS("[-asf1] [INTF]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_COMPLETE(eth_complete)
	BAREBOX_CMD_HELP(cmd_ifup_help)
BAREBOX_CMD_END

static int do_ifdown(int argc, char *argv[])
{
	int opt;
	int all = 0;

	while ((opt = getopt(argc, argv, "a")) > 0) {
		switch (opt) {
		case 'a':
			all = 1;
			break;
		}
	}

	if (all) {
		ifdown_all();
		return 0;
	}

	if (argc == optind)
		return COMMAND_ERROR_USAGE;

	return ifdown(argv[optind]);
}



BAREBOX_CMD_HELP_START(ifdown)
BAREBOX_CMD_HELP_TEXT("Disable a network interface")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",  "disable all interfaces")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ifdown)
	.cmd		= do_ifdown,
	BAREBOX_CMD_DESC("disable a network interface")
	BAREBOX_CMD_OPTS("[-a] [INTF]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_COMPLETE(eth_complete)
	BAREBOX_CMD_HELP(cmd_ifdown_help)
BAREBOX_CMD_END

#endif
