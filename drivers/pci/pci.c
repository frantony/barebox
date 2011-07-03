#include <common.h>
#include <init.h>
#include <linux/pci.h>

#include <command.h>
#include <debug_ll.h>

#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

unsigned int pci_scan_bus(struct pci_bus *bus);

static struct pci_controller *hose_head, **hose_tail = &hose_head;

struct pci_bus *pci_root;
struct pci_dev *pci_devices = NULL;
static struct pci_dev **pci_last_dev_p = &pci_devices;

static struct pci_bus * pci_alloc_bus(void)
{
	struct pci_bus *b;

	b = kzalloc(sizeof(*b), GFP_KERNEL);
	if (b) {
		INIT_LIST_HEAD(&b->node);
		INIT_LIST_HEAD(&b->children);
		INIT_LIST_HEAD(&b->devices);
		INIT_LIST_HEAD(&b->slots);
		INIT_LIST_HEAD(&b->resources);
		//b->max_bus_speed = PCI_SPEED_UNKNOWN;
		//b->cur_bus_speed = PCI_SPEED_UNKNOWN;
	}
	return b;
}

void register_pci_controller(struct pci_controller *hose)
{
	struct pci_bus *bus;

	printf("PCI: register_pci_controller() \n");

	*hose_tail = hose;
	hose_tail = &hose->next;

	bus = pci_alloc_bus();
	hose->bus = bus;
	bus->ops = hose->pci_ops;
	bus->resource[0] = hose->mem_resource;
	bus->resource[1] = hose->io_resource;

	pci_scan_bus(bus);

	pci_root = bus;

	return;
}

int
pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, u8 where, u8 *val)
{
	u32 data;
	int status;

	status = bus->ops->read(bus, devfn, where & 0xfc, 4, &data);
	*val = (u8) (data >> ((where & 3) << 3));
	return status;
}

int
pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, u8 where, u16 *val)
{
	u32 data;
	int status;

	status = bus->ops->read(bus, devfn, where & 0xfc, 4, &data);
	*val = (u16) (data >> ((where & 3) << 3));
	return status;
}

int
pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, u8 where, u32 *val)
{
	return bus->ops->read(bus, devfn, where, 4, val);
}

int
pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn, u8 where, u8 val)
{
	u32 data;
	int status;

	bus->ops->read(bus, devfn, where & 0xfc, 4, &data);
	data = (data & ~(0xff << ((where & 3) << 3))) | (val << ((where & 3) << 3));
	status = bus->ops->write(bus, devfn, where & 0xfc, 4, data);

	return status;
}

int
pci_bus_write_config_word(struct pci_bus *bus, unsigned int devfn, u8 where, u16 val)
{
	u32 data;
	int status;

	bus->ops->read(bus, devfn, where & 0xfc, 4, &data);
	data = (data & ~(0xffff << ((where & 3) << 3))) | (val << ((where & 3) << 3));
	status = bus->ops->write(bus, devfn, where & 0xfc, 4, data);

	return status;
}

int
pci_bus_write_config_dword(struct pci_bus *bus, unsigned int devfn, u8 where, u32 val)
{
	return bus->ops->write(bus, devfn, where, 4, val);
}

int
pci_read_config_byte(struct pci_dev *dev, u8 where, u8 *val)
{
	return pci_bus_read_config_byte(dev->bus, dev->devfn, where, val);
}

int
pci_read_config_word(struct pci_dev *dev, u8 where, u16 *val)
{
	return pci_bus_read_config_word(dev->bus, dev->devfn, where, val);
}

int
pci_read_config_dword(struct pci_dev *dev, u8 where, u32 *val)
{
	return pci_bus_read_config_dword(dev->bus, dev->devfn, where, val);
}

int
pci_write_config_byte(struct pci_dev *dev, u8 where, u8 val)
{
	return pci_bus_write_config_byte(dev->bus, dev->devfn, where, val);
}

int
pci_write_config_word(struct pci_dev *dev, u8 where, u16 val)
{
	return pci_bus_write_config_word(dev->bus, dev->devfn, where, val);
}

int
pci_write_config_dword(struct pci_dev *dev, u8 where, u32 val)
{
	return pci_bus_write_config_dword(dev->bus, dev->devfn, where, val);
}

struct pci_dev *alloc_pci_dev(void)
{
	struct pci_dev *dev;

	dev = kzalloc(sizeof(struct pci_dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	INIT_LIST_HEAD(&dev->bus_list);

	return dev;
}

unsigned int pci_scan_bus(struct pci_bus *bus)
{
	unsigned int devfn, l, max, class;
	unsigned char cmd, irq, tmp, hdr_type, is_multi = 0;
	struct pci_dev *dev, **bus_last;
	struct pci_bus *child;
	resource_size_t last_mem;
	resource_size_t last_io;

	//last_mem = bus->ops->res_start(bus,
	last_mem = bus->resource[0]->start;
	last_io = bus->resource[1]->start;
	// FIXME
	// add 0x1000: skip malta serial port
	last_io += 0x1000;

	DBG("pci_scan_bus for bus %d\n", bus->number);
	DBG(" last_io = 0x%08x, last_mem = 0x%08x\n", last_io, last_mem);
	bus_last = &bus->devices;
	max = bus->secondary;

	for (devfn = 0; devfn < 0xff; ++devfn) {
		if (PCI_FUNC(devfn) && !is_multi) {
			/* not a multi-function device */
			continue;
		}
		if (pci_bus_read_config_byte(bus, devfn, PCI_HEADER_TYPE, &hdr_type))
			continue;
		if (!PCI_FUNC(devfn))
			is_multi = hdr_type & 0x80;

		if (pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &l) ||
		    /* some broken boards return 0 if a slot is empty: */
		    l == 0xffffffff || l == 0x00000000 || l == 0x0000ffff || l == 0xffff0000)
			continue;

		dev = alloc_pci_dev();
		if (!dev)
			return 0;

		dev->bus = bus;
		dev->devfn = devfn;
		dev->vendor = l & 0xffff;
		dev->device = (l >> 16) & 0xffff;

		/* non-destructively determine if device can be a master: */
		pci_read_config_byte(dev, PCI_COMMAND, &cmd);
		pci_write_config_byte(dev, PCI_COMMAND, cmd | PCI_COMMAND_MASTER);
		pci_read_config_byte(dev, PCI_COMMAND, &tmp);
		//dev->master = ((tmp & PCI_COMMAND_MASTER) != 0);
		pci_write_config_byte(dev, PCI_COMMAND, cmd);

		pci_read_config_dword(dev, PCI_CLASS_REVISION, &class);
		dev->revision = class & 0xff;
		class >>= 8;				    /* upper 3 bytes */
		dev->class = class;
		class >>= 8;
		dev->hdr_type = hdr_type;

		DBG("PCI: class = %08x, hdr_type = %08x\n", class, hdr_type);

		switch (hdr_type & 0x7f) {		    /* header type */
		case PCI_HEADER_TYPE_NORMAL:		    /* standard header */
			if (class == PCI_CLASS_BRIDGE_PCI)
				goto bad;
			/*
			 * If the card generates interrupts, read IRQ number
			 * (some architectures change it during pcibios_fixup())
			 */
			pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq);
			if (irq)
				pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq);
			dev->irq = irq;
			/*
			 * read base address registers, again pcibios_fixup() can
			 * tweak these
			 */
	//		pci_read_bases(dev, 6);
			pci_read_config_dword(dev, PCI_ROM_ADDRESS, &l);
			dev->rom_address = (l == 0xffffffff) ? 0 : l;
			break;
		#if 0
		case PCI_HEADER_TYPE_BRIDGE:		    /* bridge header */
			if (class != PCI_CLASS_BRIDGE_PCI)
				goto bad;
			pci_read_bases(dev, 2);
			pcibios_read_config_dword(bus->number, devfn, PCI_ROM_ADDRESS1, &l);
			dev->rom_address = (l == 0xffffffff) ? 0 : l;
			break;
		case PCI_HEADER_TYPE_CARDBUS:		    /* CardBus bridge header */
			if (class != PCI_CLASS_BRIDGE_CARDBUS)
				goto bad;
			pci_read_bases(dev, 1);
			break;
#endif
		default:				    /* unknown header */
		bad:
			printk(KERN_ERR "PCI: %02x:%02x [%04x/%04x/%06x] has unknown header type %02x, ignoring.\n",
			       bus->number, dev->devfn, dev->vendor, dev->device, class, hdr_type);
			continue;
		}

		DBG("PCI: %02x:%02x [%04x/%04x]\n", bus->number, dev->devfn, dev->vendor, dev->device);

		list_add_tail(&dev->bus_list, &bus->devices);
		pci_register_device(dev);

		if (class == PCI_CLASS_BRIDGE_HOST) {
			DBG("PCI: skip pci host bridge\n");
			continue;
		}

		{
			int bar;
			u32 old_bar, mask;
			int size;

			for (bar = 0; bar < 6; bar++) {
			pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &old_bar);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, 0xfffffffe);
			pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &mask);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, old_bar);

			if (mask == 0 || mask == 0xffffffff) {
				//DBG("  PCI: pbar%d set bad mask\n", bar);
				continue;
			}

			if (mask & 0x01) { /* IO */
				size = -(mask & 0xfffffffe);
				DBG("  PCI: pbar%d: mask=%08x io %d bytes\n", bar, mask, size);
				DBG("       mapped to 0x%08x (0x%08x)\n", last_io, bus->ops->res_start(bus, last_io));
				pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_io);
				last_io += size;

			} else { /* MEM */
				size = -(mask & 0xfffffff0);
				DBG("  PCI: pbar%d: mask=%08x memory %d bytes\n", bar, mask, size);
				DBG("       mapped to 0x%08x (0x%08x)\n", last_mem, bus->ops->res_start(bus, last_mem));
				pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_mem);
				last_mem += size;
			}
			}
		}
	}

	/*
	 * After performing arch-dependent fixup of the bus, look behind
	 * all PCI-to-PCI bridges on this bus.
	 */
	//pcibios_fixup_bus(bus);

	/*
	 * We've scanned the bus and so we know all about what's on
	 * the other side of any bridges that may be on this bus plus
	 * any devices.
	 *
	 * Return how far we've got finding sub-buses.
	 */
	DBG("PCI: pci_scan_bus returning with max=%02x\n", max);
	return max;
}

static int pci_init(void)
{
	pcibios_init();

	if (!pci_present()) {
		printf("PCI: No PCI bus detected\n");
		return 0;
	}

	/* give BIOS a chance to apply platform specific fixes: */
	//pcibios_fixup();

	return 0;
}
postcore_initcall(pci_init);
