/*
 * driver.c - barebox driver model
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file
 * @brief barebox's driver model, and devinfo command
 */

#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <console.h>
#include <linux/ctype.h>
#include <errno.h>
#include <init.h>
#include <fs.h>
#include <of.h>
#include <linux/list.h>
#include <linux/err.h>
#include <complete.h>
#include <pinctrl.h>
#include <linux/clk/clk-conf.h>

LIST_HEAD(device_list);
EXPORT_SYMBOL(device_list);

LIST_HEAD(driver_list);
EXPORT_SYMBOL(driver_list);

static LIST_HEAD(active);
static LIST_HEAD(deferred);

struct device_d *get_device_by_name(const char *name)
{
	struct device_d *dev;

	for_each_device(dev) {
		if(!strcmp(dev_name(dev), name))
			return dev;
	}

	return NULL;
}

static struct device_d *get_device_by_name_id(const char *name, int id)
{
	struct device_d *dev;

	for_each_device(dev) {
		if(!strcmp(dev->name, name) && id == dev->id)
			return dev;
	}

	return NULL;
}

int get_free_deviceid(const char *name_template)
{
	int i = 0;

	while (1) {
		if (!get_device_by_name_id(name_template, i))
			return i;
		i++;
	};
}

int device_probe(struct device_d *dev)
{
	int ret;

	pinctrl_select_state_default(dev);
	of_clk_set_defaults(dev->device_node, false);

	list_add(&dev->active, &active);

	ret = dev->bus->probe(dev);
	if (ret == 0)
		return 0;

	if (ret == -EPROBE_DEFER) {
		list_del(&dev->active);
		list_add(&dev->active, &deferred);
		dev_dbg(dev, "probe deferred\n");
		return ret;
	}

	list_del(&dev->active);
	INIT_LIST_HEAD(&dev->active);

	if (ret == -ENODEV || ret == -ENXIO)
		dev_dbg(dev, "probe failed: %s\n", strerror(-ret));
	else
		dev_err(dev, "probe failed: %s\n", strerror(-ret));

	return ret;
}

int device_detect(struct device_d *dev)
{
	if (!dev->detect)
		return -ENOSYS;
	return dev->detect(dev);
}

int device_detect_by_name(const char *__devname)
{
	char *devname = xstrdup(__devname);
	char *str = devname;
	struct device_d *dev;
	int ret = -ENODEV;

	while (1) {
		strsep(&str, ".");

		dev = get_device_by_name(devname);
		if (dev)
			ret = device_detect(dev);

		if (!str)
			break;
		else
			*(str - 1) = '.';
	}

	free(devname);

	return ret;
}

void device_detect_all(void)
{
	struct device_d *dev;

	for_each_device(dev)
		device_detect(dev);
}

static int match(struct driver_d *drv, struct device_d *dev)
{
	int ret;

	if (dev->driver)
		return -1;

	dev->driver = drv;

	if (dev->bus->match(dev, drv))
		goto err_out;
	ret = device_probe(dev);
	if (ret)
		goto err_out;

	return 0;
err_out:
	dev->driver = NULL;
	return -1;
}

int register_device(struct device_d *new_device)
{
	struct driver_d *drv;

	if (new_device->id == DEVICE_ID_DYNAMIC) {
		new_device->id = get_free_deviceid(new_device->name);
	} else {
		if (get_device_by_name_id(new_device->name, new_device->id)) {
			pr_err("register_device: already registered %s\n",
				dev_name(new_device));
			return -EINVAL;
		}
	}

	if (new_device->id != DEVICE_ID_SINGLE)
		new_device->unique_name = basprintf(FORMAT_DRIVER_NAME_ID,
						    new_device->name,
						    new_device->id);

	debug ("register_device: %s\n", dev_name(new_device));

	list_add_tail(&new_device->list, &device_list);
	INIT_LIST_HEAD(&new_device->children);
	INIT_LIST_HEAD(&new_device->cdevs);
	INIT_LIST_HEAD(&new_device->parameters);
	INIT_LIST_HEAD(&new_device->active);
	INIT_LIST_HEAD(&new_device->bus_list);

	if (new_device->bus) {
		if (!new_device->parent)
			new_device->parent = new_device->bus->dev;

		list_add_tail(&new_device->bus_list, &new_device->bus->device_list);

		bus_for_each_driver(new_device->bus, drv) {
			if (!match(drv, new_device))
				break;
		}
	}

	if (new_device->parent)
		list_add_tail(&new_device->sibling, &new_device->parent->children);

	return 0;
}
EXPORT_SYMBOL(register_device);

int unregister_device(struct device_d *old_dev)
{
	struct cdev *cdev, *ct;
	struct device_d *child, *dt;

	dev_dbg(old_dev, "unregister\n");

	dev_remove_parameters(old_dev);

	if (old_dev->driver)
		old_dev->bus->remove(old_dev);

	list_for_each_entry_safe(child, dt, &old_dev->children, sibling) {
		dev_dbg(old_dev, "unregister child %s\n", dev_name(child));
		unregister_device(child);
	}

	list_for_each_entry_safe(cdev, ct, &old_dev->cdevs, devices_list) {
		if (cdev->master) {
			dev_dbg(old_dev, "unregister part %s\n", cdev->name);
			devfs_del_partition(cdev->name);
		}
	}

	list_del(&old_dev->list);
	list_del(&old_dev->bus_list);
	list_del(&old_dev->active);

	/* remove device from parents child list */
	if (old_dev->parent)
		list_del(&old_dev->sibling);

	return 0;
}
EXPORT_SYMBOL(unregister_device);

/*
 * Loop over list of deferred devices as long as at least one
 * device is successfully probed. Devices that again request
 * deferral are re-added to deferred list in device_probe().
 * For devices finally left in deferred list -EPROBE_DEFER
 * becomes a fatal error.
 */
static int device_probe_deferred(void)
{
	struct device_d *dev, *tmp;
	struct driver_d *drv;
	bool success;

	do {
		success = false;

		if (list_empty(&deferred))
			return 0;

		list_for_each_entry_safe(dev, tmp, &deferred, active) {
			list_del(&dev->active);
			INIT_LIST_HEAD(&dev->active);

			dev_dbg(dev, "re-probe device\n");
			bus_for_each_driver(dev->bus, drv) {
				if (match(drv, dev))
					continue;
				success = true;
				break;
			}
		}
	} while (success);

	list_for_each_entry(dev, &deferred, active)
		dev_err(dev, "probe permanently deferred\n");

	return 0;
}
late_initcall(device_probe_deferred);

struct driver_d *get_driver_by_name(const char *name)
{
	struct driver_d *drv;

	for_each_driver(drv) {
		if(!strcmp(name, drv->name))
			return drv;
	}

	return NULL;
}

int register_driver(struct driver_d *drv)
{
	struct device_d *dev = NULL;

	if (!drv->name)
		return -EINVAL;

	debug("register_driver: %s\n", drv->name);

	BUG_ON(!drv->bus);

	list_add_tail(&drv->list, &driver_list);
	list_add_tail(&drv->bus_list, &drv->bus->driver_list);

	bus_for_each_device(drv->bus, dev)
		match(drv, dev);

	return 0;
}
EXPORT_SYMBOL(register_driver);

struct resource *dev_get_resource(struct device_d *dev, unsigned long type,
				  int num)
{
	int i, n = 0;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *res = &dev->resource[i];
		if (resource_type(res) == type) {
			if (n == num)
				return res;
			n++;
		}
	}

	return ERR_PTR(-ENOENT);
}

void *dev_get_mem_region(struct device_d *dev, int num)
{
	struct resource *res;

	res = dev_get_resource(dev, IORESOURCE_MEM, num);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return (void __force *)res->start;
}
EXPORT_SYMBOL(dev_get_mem_region);

struct resource *dev_get_resource_by_name(struct device_d *dev,
					  unsigned long type,
					  const char *name)
{
	int i;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *res = &dev->resource[i];
		if (resource_type(res) != type)
			continue;
		if (!res->name)
			continue;
		if (!strcmp(name, res->name))
			return res;
	}

	return ERR_PTR(-ENOENT);
}

struct resource *dev_request_mem_resource_by_name(struct device_d *dev, const char *name)
{
	struct resource *res;

	res = dev_get_resource_by_name(dev, IORESOURCE_MEM, name);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return request_iomem_region(dev_name(dev), res->start, res->end);
}
EXPORT_SYMBOL(dev_request_mem_resource_by_name);

void __iomem *dev_request_mem_region_by_name(struct device_d *dev, const char *name)
{
	struct resource *res;

	res = dev_request_mem_resource_by_name(dev, name);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_request_mem_region_by_name);

struct resource *dev_request_mem_resource(struct device_d *dev, int num)
{
	struct resource *res;

	res = dev_get_resource(dev, IORESOURCE_MEM, num);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return request_iomem_region(dev_name(dev), res->start, res->end);
}

void __iomem *dev_request_mem_region_err_null(struct device_d *dev, int num)
{
	struct resource *res;

	res = dev_request_mem_resource(dev, num);
	if (IS_ERR(res))
		return NULL;

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_request_mem_region_err_null);

void __iomem *dev_request_mem_region(struct device_d *dev, int num)
{
	struct resource *res;

	res = dev_request_mem_resource(dev, num);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_request_mem_region);

int generic_memmap_rw(struct cdev *cdev, void **map, int flags)
{
	if (!cdev->dev)
		return -EINVAL;

	*map = dev_get_mem_region(cdev->dev, 0);

	return PTR_ERR_OR_ZERO(*map);
}

int generic_memmap_ro(struct cdev *cdev, void **map, int flags)
{
	if (flags & PROT_WRITE)
		return -EACCES;

	return generic_memmap_rw(cdev, map, flags);
}

/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 *
 * NOTE: This function expects dev->name to be free()-able, so extra
 * precautions needs to be taken when mixing its usage with manual
 * assignement of device_d.name.
 */
int dev_set_name(struct device_d *dev, const char *fmt, ...)
{
	va_list vargs;
	int err;
	/*
	 * Save old pointer in case we are overriding already set name
	 */
	char *oldname = dev->name;

	va_start(vargs, fmt);
	err = vasprintf(&dev->name, fmt, vargs);
	va_end(vargs);

	/*
	 * Free old pointer, we do this after vasprintf call in case
	 * old device name was in one of vargs
	 */
	free(oldname);

	return WARN_ON(err < 0) ? err : 0;
}
EXPORT_SYMBOL_GPL(dev_set_name);

static void devices_shutdown(void)
{
	struct device_d *dev;

	list_for_each_entry(dev, &active, active) {
		if (dev->bus->remove)
			dev->bus->remove(dev);
	}
}
devshutdown_exitcall(devices_shutdown);

int dev_get_drvdata(struct device_d *dev, const void **data)
{
	if (dev->of_id_entry) {
		*data = dev->of_id_entry->data;
		return 0;
	}

	if (dev->id_entry) {
		*data = (const void **)dev->id_entry->driver_data;
		return 0;
	}

	return -ENODEV;
}

const void *device_get_match_data(struct device_d *dev)
{
	if (dev->of_id_entry)
		return dev->of_id_entry->data;

	if (dev->id_entry)
		return (void *)dev->id_entry->driver_data;

	return NULL;
}
