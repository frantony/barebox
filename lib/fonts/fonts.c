/*
 * `Soft' font definitions
 *
 *    Created 1995 by Geert Uytterhoeven
 *    Rewritten 1998 by Martin Mares <mj@ucw.cz>
 *
 *	2001 - Documented with DocBook
 *	- Brad Douglas <brad@neruo.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/font.h>

#define NO_FONTS

static const struct font_desc *fonts[] = {
#ifdef CONFIG_FONT_8x16
#undef NO_FONTS
    &font_vga_8x16,
#endif
#ifdef CONFIG_FONT_MINI_4x6
#undef NO_FONTS
    &font_mini_4x6,
#endif
};

#define num_fonts ARRAY_SIZE(fonts)

#ifdef NO_FONTS
#error No fonts configured.
#endif


/**
 *	find_font - find a font
 *	@name: string name of a font
 *
 *	Find a specified font with string name @name.
 *
 *	Returns %NULL if no font found, or a pointer to the
 *	specified font.
 *
 */
const struct font_desc *find_font(const char *name)
{
	unsigned int i;

	for (i = 0; i < num_fonts; i++)
		if (!strncmp(fonts[i]->name, name, MAX_FONT_NAME))
			  return fonts[i];

	return NULL;
}

EXPORT_SYMBOL(find_font);

MODULE_AUTHOR("James Simmons <jsimmons@users.sf.net>");
MODULE_DESCRIPTION("Console Fonts");
MODULE_LICENSE("GPL");
