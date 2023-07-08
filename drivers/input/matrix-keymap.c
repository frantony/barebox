// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <input/matrix_keypad.h>

/**
 * matrix_keypad_parse_properties() - Read properties of matrix keypad
 *
 * @dev: Device containing properties
 * @rows: Returns number of matrix rows
 * @cols: Returns number of matrix columns
 * @return 0 if OK, <0 on error
 */
int matrix_keypad_parse_properties(struct device *dev,
				   unsigned int *rows, unsigned int *cols)
{
	struct device_node *np = dev->of_node;

	*rows = *cols = 0;

	of_property_read_u32(np, "keypad,num-rows", rows);
	of_property_read_u32(np, "keypad,num-columns", cols);

	if (!*rows || !*cols) {
		dev_err(dev, "number of keypad rows/columns not specified\n");
		return -EINVAL;
	}

	return 0;
}

static int matrix_keypad_parse_of_keymap(struct device *dev,
					 unsigned int row_shift,
					 unsigned short *keymap)
{
	unsigned int proplen, i, size;
	const __be32 *prop;
	struct device_node *np = dev->of_node;
	const char *propname = "linux,keymap";

	prop = of_get_property(np, propname, &proplen);
	if (!prop) {
		dev_err(dev, "OF: %s property not defined in %pOF\n",
			propname, np);
		return -ENOENT;
	}

	if (proplen % sizeof(u32)) {
		dev_err(dev, "OF: Malformed keycode property %s in %pOF\n",
			propname, np);
		return -EINVAL;
	}

	size = proplen / sizeof(u32);

	for (i = 0; i < size; i++) {
		unsigned int key = be32_to_cpup(prop + i);

		unsigned int row = KEY_ROW(key);
		unsigned int col = KEY_COL(key);
		unsigned short code = KEY_VAL(key);

		if (row >= MATRIX_MAX_ROWS || col >= MATRIX_MAX_COLS) {
			dev_err(dev, "rows/cols out of range\n");
			continue;
		}

		keymap[MATRIX_SCAN_CODE(row, col, row_shift)] = code;
	}

	return 0;
}
/**
 * matrix_keypad_build_keymap - convert platform keymap into matrix keymap
 * @keymap_data: keymap supplied by the platform code
 * @row_shift: number of bits to shift row value by to advance to the next
 * line in the keymap
 * @keymap: expanded version of keymap that is suitable for use by
 * matrix keyboad driver
 * This function converts platform keymap (encoded with KEY() macro) into
 * an array of keycodes that is suitable for using in a standard matrix
 * keyboard driver that uses row and col as indices.
 */
int matrix_keypad_build_keymap(struct device *dev,
			       const struct matrix_keymap_data *keymap_data,
			       unsigned int row_shift,
			       unsigned short *keymap)
{
	int i;

	if (IS_ENABLED(CONFIG_OFDEVICE) && dev->of_node)
		return matrix_keypad_parse_of_keymap(dev, row_shift, keymap);

	if (!keymap_data)
		return -EINVAL;

	for (i = 0; i < keymap_data->keymap_size; i++) {
		unsigned int key = keymap_data->keymap[i];
		unsigned int row = KEY_ROW(key);
		unsigned int col = KEY_COL(key);
		unsigned short code = KEY_VAL(key);

		keymap[MATRIX_SCAN_CODE(row, col, row_shift)] = code;
	}

	return 0;
}
