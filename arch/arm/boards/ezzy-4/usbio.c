/*
 * This file is part of fpdoom
 * https://github.com/ilyakurdyukov/fpdoom/blob/main/fpdoom/usbio.c
 */

#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t count);
size_t strlen(const char *src);

void usb_debug_ll_init(void);
int usb_PUTC_LL(char ch);

#define ALIGN(n) __attribute__((aligned(n)))

#define readl(a) (*(volatile uint32_t *)(a))
#define writel(v,a)	(*(volatile uint32_t *)(a) = (v))

#define MEM4(addr) *(volatile uint32_t*)(addr)

#define READ32_BE(p) (uint32_t)( \
	((uint8_t*)(p))[0] << 24 | \
	((uint8_t*)(p))[1] << 16 | \
	((uint8_t*)(p))[2] << 8 | \
	((uint8_t*)(p))[3])

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline uint32_t swap_be32(uint32_t v) {
	uint32_t t = v >> 24 | v << 24, m = 0xff00;
	return t | (v >> 8 & m) | (v & m) << 8;
}
#endif

static unsigned fastchk16(unsigned crc, const void *src, int len)
{
	uint8_t *s = (uint8_t*)src;

	while (len > 1) {
		crc += s[1] << 8 | s[0]; s += 2;
		len -= 2;
	}

	if (len)
		crc += *s;

	crc = (crc >> 16) + (crc & 0xffff);
	crc += crc >> 16;

	return crc & 0xffff;
}

enum {
	HOST_CONNECT = 0x00,

	CMD_MESSAGE = 0x80,
	CMD_FOPEN = 0x81,
	CMD_FREAD = 0x82,
	CMD_FWRITE = 0x83,
	CMD_FCLOSE = 0x84,
	CMD_FSEEK = 0x85,
	CMD_FTELL = 0x86,
	CMD_GETARGS = 0x87,
};

#define CHECKSUM_INIT 0x5a5a
#define USB_WAIT 1
#define USB_NOWAIT 0

#define USB_BASE_INIT
#define USB_BASE 0x90000000

#define USB_CR(o) MEM4(USB_BASE + o)

#define USB_MAXREAD 64

// not necessary, because USB is already
// initialized by the bootloader
#define INIT_USB 1

#define USB_BUFSIZE 0x800

#if (USB_BUFSIZE) & (USB_BUFSIZE - 1)
#error
#endif

typedef struct {
	uint32_t rpos, wpos;
	uint8_t buf[USB_BUFSIZE];
} usb_buf_t;

usb_buf_t usb_buf;

static const uint8_t dev_desc[] ALIGN(4) = {
	0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x40,
	0x82, 0x17, 0x00, 0x4d, 0x02, 0x02, 0x00, 0x00,
	0x00, 0x01
};

static const uint8_t config_desc[] ALIGN(4) = {
	0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0, 0x32,
	0x09, 0x04, 0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x83, 0x02, 0x40, 0x00, 0x00,
	0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00
};

enum {
	USB_CTRL = 0,
	INT_STS = 0x18,
	INT_CLR = 0x1c,
	TIMEOUT_LMT = 0x28,

	TR_SIZE_IN_ENDP0 = 0x40,
	REQ_SETUP_LOW = 0x5c,
	REQ_SETUP_HIGH = 0x60,
	ENDP0_CTRL = 0x64,
	INT_CTRL_ENDP0 = 0x68,
	INT_STS_ENDP0 = 0x6c,
	INT_CLR_ENDP0 = 0x70,

	ENDP1_CTRL = 0xc0,
	TRANS_SIZE_ENDP1 = 0xc8,
	INT_CTRL_ENDP1 = 0xcc,
	INT_STS_ENDP1 = 0xd0,
	INT_CLR_ENDP1 = 0xd4,

	ENDP2_CTRL = 0x100,
	RCV_DATA_ENDP2 = 0x104,
	INT_CTRL_ENDP2 = 0x10c,
	INT_STS_ENDP2 = 0x110,
	INT_CLR_ENDP2 = 0x114,

	ENDP3_CTRL = 0x140,
	TRANS_SIZE_ENDP3 = 0x148,
	INT_CTRL_ENDP3 = 0x14c,
	INT_STS_ENDP3 = 0x150,
	INT_CLR_ENDP3 = 0x154,
};

#define FIFO_entry_endp0_in (uint32_t*)(USB_BASE + 0x80000)
#define FIFO_entry_endp1 (FIFO_entry_endp0_in + 1)
#define FIFO_entry_endp3 (FIFO_entry_endp0_in + 2)

#define FIFO_entry_endp_out (uint32_t*)(USB_BASE + 0x8000c)
#define FIFO_entry_endp2 (FIFO_entry_endp_out + 1)

/* max packet size */
#define USB_MAXPSIZE(o, n) \
	(USB_CR(o) = (USB_CR(o) & ~0x7ff000) | (n) << 12)
/* transfer size */
#define USB_TRSIZE(o, n) \
	(USB_CR(o) = (USB_CR(o) & ~0x1ffff) | (n))

#if INIT_USB
static void usb_init_endp0(void) {
	USB_BASE_INIT
	USB_MAXPSIZE(ENDP0_CTRL, 8);
	USB_CR(INT_CLR_ENDP0) |= 1 << 8;
	USB_CR(INT_CTRL_ENDP0) |= 1 << 8;
	USB_CR(ENDP0_CTRL) |= 1 << 28; // buffer ready
}

static void usb_init_endp2(void) {
	USB_BASE_INIT
	USB_MAXPSIZE(ENDP2_CTRL, 0x40);
	USB_TRSIZE(RCV_DATA_ENDP2, 0x2000);
	USB_CR(INT_CLR_ENDP2) = 0x3fff;
	USB_CR(INT_CTRL_ENDP2) = 0;
	USB_CR(INT_CLR_ENDP2) |= 1;
	USB_CR(INT_CTRL_ENDP2) |= 1;
	USB_CR(ENDP2_CTRL) |= 1 << 25; // endpoint enable
	USB_CR(ENDP2_CTRL) |= 1 << 28; // buffer ready
}

static void usb_init_endp3(void) {
	USB_BASE_INIT
	USB_MAXPSIZE(ENDP3_CTRL, 0x40);
	USB_TRSIZE(TRANS_SIZE_ENDP3, 0x40);
	USB_CR(INT_CLR_ENDP3) = 0x3fff;
	USB_CR(INT_CTRL_ENDP3) = 0;
	USB_CR(INT_CLR_ENDP3) |= 1 << 9;
	USB_CR(INT_CTRL_ENDP3) |= 1 << 9;
	USB_CR(ENDP3_CTRL) |= 1 << 25; // endpoint enable
}
#endif

// len = 0..0x7ff
static void usb_send(uint32_t ep, const void *src, uint32_t len) {
	uint32_t i, ctrl, tr_size; uint32_t *fifo;
	const uint32_t *s = (const uint32_t*)src;
	USB_BASE_INIT
	do {
		if (ep == 0) {
			ctrl = ENDP0_CTRL;
			tr_size = TR_SIZE_IN_ENDP0;
			fifo = FIFO_entry_endp0_in;
		} else if (ep == 4) {
			ctrl = ENDP3_CTRL;
			tr_size = TRANS_SIZE_ENDP3;
			fifo = FIFO_entry_endp3;
		} else break;

		USB_MAXPSIZE(ctrl, len);
		USB_TRSIZE(tr_size, len);

		for (i = 0; i < len; i += 4, s++)
			*(volatile uint32_t*)fifo = READ32_BE(s);

		USB_CR(ctrl) |= 1 << 27;

		if (ep == 4) {
			// TRANSFER_END
			while ((USB_CR(INT_STS_ENDP3) & 1 << 9) == 0);
			USB_CR(INT_CLR_ENDP3) |= 1 << 9;
		}
	} while (0);
}

static void usb_recv(uint32_t ep, uint32_t *dst, uint32_t len) {
	uint32_t i, ctrl; uint32_t *fifo;
	USB_BASE_INIT
	do {
		fifo = FIFO_entry_endp_out;
		if (ep == 1) {
			ctrl = ENDP0_CTRL;
		} else if (ep == 3) {
			ctrl = ENDP2_CTRL;
			fifo += 1;	// FIFO_entry_endp2
		} else break;

		for (i = 0; i < len; i += 8) {
			*dst++ = swap_be32(*(volatile uint32_t*)fifo);
			*dst++ = swap_be32(*(volatile uint32_t*)fifo);
		}

		USB_CR(ctrl) |= 1 << 28;
	} while (0);
}

#define USB_REC_DEVICE     0
#define USB_REC_INTERFACE  1
#define USB_REC_MASK       0x1f

#define USB_REQ_STANDARD   (0 << 5)
#define USB_REQ_CLASS      (1 << 5)
#define USB_REQ_VENDOR     (2 << 5)
#define USB_REQ_MASK       (3 << 5)

#define USB_REQUEST_GET_DESCRIPTOR  6

#define USB_DEVICE_DESCRIPTOR_TYPE  1
#define USB_CONFIGURATION_DESCRIPTOR_TYPE  2

static void usb_send_desc(int type, int len) {
	const void *p; int n;
	if (type == USB_DEVICE_DESCRIPTOR_TYPE) {
		p = dev_desc; n = sizeof(dev_desc);
	} else if (type == USB_CONFIGURATION_DESCRIPTOR_TYPE) {
		p = config_desc; n = sizeof(config_desc);
	} else return;

	if (len > n) len = n;
	usb_send(0, p, len);
}

static void usb_int_endp0(void) {
	uint32_t a, b, len, req;
	USB_BASE_INIT
	if (USB_CR(INT_STS_ENDP0) & 1 << 8) { // SETUP_TRANS_END
		a = USB_CR(REQ_SETUP_LOW);
		len = USB_CR(REQ_SETUP_HIGH) >> 16; // wLength
		req = (a >> 8) & 0xff;

		b = a & (USB_REC_MASK | USB_REQ_MASK);
		if (b == (USB_REC_DEVICE | USB_REQ_STANDARD)) {
			if (req == USB_REQUEST_GET_DESCRIPTOR)
				usb_send_desc(a >> 24, len);
		}
	}
	USB_CR(INT_CLR_ENDP0) = 0x3fff;
}

static void usb_int_endp2(void) {
	usb_buf_t *p; int i, len, wpos;
	USB_BASE_INIT
	if (USB_CR(INT_STS_ENDP2) & 1) { // TRANSACTION_END
		uint8_t buf[USB_MAXREAD];
		len = USB_CR(ENDP2_CTRL) & (USB_BUFSIZE - 1);
		if (len > USB_MAXREAD) len = USB_MAXREAD;
		usb_recv(3, (uint32_t*)buf, len);
		p = &usb_buf;
		wpos = p->wpos;
		for (i = 0; i < len; i++) {
			p->buf[wpos++] = buf[i];
			wpos &= (USB_BUFSIZE - 1);
		}
		p->wpos = wpos;
		USB_CR(ENDP2_CTRL) |= 1 << 28;
	}
	USB_CR(INT_CLR_ENDP2) = 0x3fff;
}

static void usb_int_endp3(void) {
	USB_BASE_INIT
	USB_CR(INT_CLR_ENDP3) = 0x3fff;
}

static void usb_check_int(void) {
	USB_BASE_INIT
	if ( readl(0x80001004) & (1 << 5) /* SC6531(DA/E) */ ) {
		int mask = USB_CR(INT_STS);
		if (mask & 0x3fff) {
			if (mask & 1 << 10) usb_int_endp2();
			if (mask & 1 << 11) usb_int_endp3();
			if (mask & 1 << 8) usb_int_endp0();
		}
		USB_CR(INT_CLR) = 0x7f;
	}
}

static void usb_init(void) {
	usb_buf_t *p = &usb_buf;
	p->rpos = 0;
	p->wpos = 0;

#if INIT_USB
	USB_CR(USB_CTRL) |= 1; // USB_ENABLE
	usb_init_endp0();
	usb_init_endp2();
	usb_init_endp3();
	// 12MHz / 15 = 800kHz
	USB_CR(TIMEOUT_LMT) = 15;
#endif
}

static int usb_read(void *dst, unsigned len, int wait) {
	usb_buf_t *p = &usb_buf;
	uint8_t *d = (uint8_t*)dst;
	unsigned rpos, n, n2;

	while (len) {
		// usb_buf_free - 1 >= USB_MAXREAD
		if (((p->rpos - p->wpos - 1) & (USB_BUFSIZE - 1)) >= USB_MAXREAD)
			usb_check_int();

		rpos = p->rpos;
		n = (p->wpos - rpos) & (USB_BUFSIZE - 1);
		if (n) {
			if (n > len) n = len;
			len -= n;
			n2 = USB_BUFSIZE - rpos;
			if (n <= n2) {
				memcpy(d, p->buf + rpos, n);
			} else {
				memcpy(d, p->buf + rpos, n2);
				memcpy(d + n2, p->buf, n - n2);
			}
			d += n;
			p->rpos = (rpos + n) & (USB_BUFSIZE - 1);
		} else if (!wait) break;
	}
	return d - (uint8_t*)dst;
}

static int usb_write(const void *src, unsigned len) {
	const uint8_t *s = (const uint8_t*)src;
	for (; len > USB_MAXREAD; len -= USB_MAXREAD) {
		usb_send(4, s, USB_MAXREAD);
		s += USB_MAXREAD;
	}
	if (len) {
		if (len == USB_MAXREAD) {
			len >>= 1;
			usb_send(4, s, len);
			s += len;
		}
		usb_send(4, s, len);
	}
	return s - (uint8_t*)src;
}

static void _debug_msg(const char *msg)
{
	union { uint8_t u8[4]; uint16_t u16[2]; } buf;
	int len;
	unsigned tmp;

	len = strlen(msg);
	if (len > 255)
		len = 255;

	tmp = CMD_MESSAGE | len << 8;
	buf.u16[0] = tmp;

	tmp += CHECKSUM_INIT;
	tmp = fastchk16(tmp, msg, len);
	buf.u16[1] = tmp;

	usb_write(buf.u16, 4);
	usb_write(msg, len);
}

int usb_PUTC_LL(char ch)
{
	union { uint8_t u8[6]; uint16_t u16[3]; } buf;
	unsigned tmp;

	size_t size = 1;
	void *src;

	// stdout: handle = 1
	int handle = 1;

	src = &ch;

	tmp = CMD_FWRITE | handle << 8;
	buf.u16[0] = tmp;
	buf.u16[1] = size;
	tmp += CHECKSUM_INIT + size;
	tmp = fastchk16(tmp, src, size);
	buf.u16[2] = tmp;
	usb_write(buf.u16, 6);
	usb_write(src, size);
	usb_read(buf.u16, 2, USB_WAIT);

	return buf.u16[0];
}

void usb_debug_ll_init(void)
{
	static const uint8_t fdl_ack[8] = {
		0x7e, 0, 0x80, 0, 0, 0xff, 0x7f, 0x7e };

	usb_init();

	usb_write(fdl_ack, sizeof(fdl_ack));

	for (;;) {
		char buf[4];

		usb_read(buf, 1, USB_WAIT);

		if (buf[0] == HOST_CONNECT)
			break;
	}

	_debug_msg("debug_ll\n");
}
