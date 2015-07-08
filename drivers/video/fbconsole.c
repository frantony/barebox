#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fb.h>
#include <gui/image_renderer.h>
#include <gui/graphic_utils.h>
#include <linux/font.h>

enum state_t {
	LIT,				/* Literal input */
	ESC,				/* Start of escape sequence */
	CSI,				/* Reading arguments in "CSI Pn ;...*/
};

u32 get_pixel(struct fb_info *info, u32 color);

struct fbc_priv {
	struct console_device cdev;
	struct fb_info *fb;

	struct screen sc;
/* FIXME */
#define VIDEO_FONT_CHARS 256
	struct image *chars[VIDEO_FONT_CHARS];
	int font_width, font_height;
	const u8 *fontdata;
	unsigned int cols, rows;
	unsigned int x, y; /* cursor position */

	enum state_t state;

	u32 color;
	bool invert;

	int csipos;
	u8 csi[20];
};

static int fbc_getc(struct console_device *cdev)
{
	return 0;
}

static int fbc_tstc(struct console_device *cdev)
{
	return 0;
}

static void drawchar(struct fbc_priv *priv, int x, int y, char c)
{
	void *buf;
	int bpp = priv->fb->bits_per_pixel >> 3;
	void *adr;
	int i;
	const char *inbuf;
	int line_length;
	u32 color, bgcolor;

	buf = gui_screen_render_buffer(&priv->sc);

	inbuf = &priv->fontdata[c * priv->font_height];

	line_length = priv->fb->line_length;

	color = priv->invert ? 0xff000000 : priv->color;
	bgcolor = priv->invert ? priv->color : 0xff000000;

	for (i = 0; i < priv->font_height; i++) {
		uint8_t t = inbuf[i];
		int j;

		adr = buf + line_length * (y * priv->font_height + i) + x * priv->font_width * bpp;

		for (j = 0; j < priv->font_width; j++) {
			if (t & 0x80)
				gu_set_pixel(priv->fb, adr, color);
			else
				gu_set_pixel(priv->fb, adr, bgcolor);

			adr += priv->fb->bits_per_pixel >> 3;
			t <<= 1;
		}
	}
}

static void video_invertchar(struct fbc_priv *priv, int x, int y)
{
	void *buf;

	buf = gui_screen_render_buffer(&priv->sc);

	gu_invert_area(priv->fb, buf, x * priv->font_width, y * priv->font_height,
			priv->font_width, priv->font_height);
}

static void printchar(struct fbc_priv *priv, int c)
{
	video_invertchar(priv, priv->x, priv->y);

	switch (c) {
	case '\007': /* bell: ignore */
		break;
	case '\b':
		if (priv->x > 0) {
			priv->x--;
		} else if (priv->y > 0) {
			priv->x = priv->cols;
			priv->y--;
		}
		break;
	case '\n':
	case '\013': /* Vertical tab is the same as Line Feed */
		priv->y++;
		break;

	case '\r':
		priv->x = 0;
		break;

	case '\t':
		priv->x = (priv->x + 8) & ~0x3;
		break;

	default:
		drawchar(priv, priv->x, priv->y, c);
		gu_screen_blit(&priv->sc);

		priv->x++;
		if (priv->x > priv->cols) {
			priv->y++;
			priv->x = 0;
		}
	}

	if (priv->y > priv->rows) {
		void *buf;
		u32 line_length = priv->fb->line_length;
		int line_height = line_length * priv->font_height;

		buf = gui_screen_render_buffer(&priv->sc);

		memcpy(buf, buf + line_height, line_height * (priv->rows + 1));
		memset(buf + line_height * priv->rows, 0, line_height);
		priv->y = priv->rows;
	}

	video_invertchar(priv, priv->x, priv->y);

	return;
}

static void fbc_parse_csi(struct fbc_priv *priv)
{
	int a, b = -1;
	char *end;

	a = simple_strtoul(priv->csi, &end, 10);
	if (*end == ';')
		b = simple_strtoul(end + 1, &end, 10);

	if (*end == 'm' && b == -1) {
		switch (a) {
		case 0:
			priv->color = 0xffffffff;
			priv->invert = false;
			break;
		case 7:
			priv->invert = true;
			break;
		}
		return;
	}

	if (*end == 'J' && a == 2 && b == -1) {
		void *buf = gui_screen_render_buffer(&priv->sc);

		memset(buf, 0, priv->fb->line_length * priv->fb->yres);

		priv->x = 0;
		priv->y = 0;
		video_invertchar(priv, priv->x, priv->y);
	}

	if (*end == 'm' && a == 1) {
		switch (b) {
		case 32:
			priv->color = get_pixel(priv->fb, 0xff00ff00);
			break;
		case 31:
			priv->color = get_pixel(priv->fb, 0xffff0000);
			break;
		case 34:
			priv->color = get_pixel(priv->fb, 0xff0000ff);
			break;
		case 36:
			priv->color = get_pixel(priv->fb, 0xff54ffff);
			break;
		case 37:
			priv->color = get_pixel(priv->fb, 0xffffffff);
			break;
		default:
			break;
		}
		return;
	}

	if (*end == 'H') {
		video_invertchar(priv, priv->x, priv->y);
		priv->x = b - 1;
		priv->y = a - 1;
		video_invertchar(priv, priv->x, priv->y);
		return;
	}
}

static void fbc_putc(struct console_device *cdev, char c)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);

	switch (priv->state) {
	case LIT:
		switch (c) {
		case '\033':
			priv->state = ESC;
			break;
		default:
			printchar(priv, c);
		}
		break;
	case ESC:
		switch (c) {
		case '[':
			priv->state = CSI;
			priv->csipos = 0;
			memset(priv->csi, 0, 6);
			break;
		}
		break;
	case CSI:
		priv->csi[priv->csipos++] = c;

		switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case ';':
		case ':':
			break;
		default:
			fbc_parse_csi(priv);
			priv->state = LIT;
		}
		break;
	}
}

static int fbc_set_active(struct console_device *cdev, unsigned flags)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);
	struct fb_info *fb = priv->fb;
	int fd;

	const struct font_desc *font;

	font = find_font("VGA8x16");
	if (!font) {
		return -ENOENT;
	}

	priv->font_width = font->width;
	priv->font_height = font->height;
	priv->fontdata = font->data;

	priv->rows = fb->yres / priv->font_height - 1;
	priv->cols = fb->xres / priv->font_width - 1;

	/* FIXME */
	fd = fb_open("/dev/fb0", &priv->sc, 0);
	if (fd < 0) {
		perror("fd_open");
		return fd;
	}

	fb->fbops->fb_enable(fb);

	priv->state = LIT;

	dev_info(priv->cdev.dev, "framebuffer console %dx%d activated\n",
		priv->cols + 1, priv->rows + 1);

	return 0;
}

int register_fbconsole(struct fb_info *fb)
{
	struct fbc_priv *priv;
	struct console_device *cdev;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->fb = fb;
	priv->x = 0;
	priv->y = 0;
	priv->color = 0xff00ff00;

	cdev = &priv->cdev;
	cdev->dev = &fb->dev;
	cdev->tstc = fbc_tstc;
	cdev->putc = fbc_putc;
	cdev->getc = fbc_getc;
	cdev->devname = "fbconsole";
	cdev->devid = DEVICE_ID_DYNAMIC;
	cdev->set_active = fbc_set_active;

	ret = console_register(cdev);
	if (ret) {
		pr_err("registering failed with %s\n", strerror(-ret));
		kfree(priv);
		return ret;
	}

	pr_info("registered as %s%d\n", cdev->class_dev.name, cdev->class_dev.id);

	return 0;
}
