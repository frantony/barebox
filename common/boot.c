// SPDX-License-Identifier: GPL-2.0-only

#include <boot.h>
#include <globalvar.h>
#include <magicvar.h>
#include <watchdog.h>
#include <command.h>
#include <readkey.h>
#include <common.h>
#include <libgen.h>
#include <bootm.h>
#include <glob.h>
#include <init.h>
#include <menu.h>
#include <unistd.h>

#include <linux/stat.h>

int bootentries_add_entry(struct bootentries *entries, struct bootentry *entry)
{
	list_add_tail(&entry->list, &entries->entries);

	return 0;
}

struct bootentries *bootentries_alloc(void)
{
	struct bootentries *bootentries;

	bootentries = xzalloc(sizeof(*bootentries));
	INIT_LIST_HEAD(&bootentries->entries);

	if (IS_ENABLED(CONFIG_MENU)) {
		bootentries->menu = menu_alloc();
		menu_add_title(bootentries->menu, "boot");
	}

	return bootentries;
}

void bootentries_free(struct bootentries *bootentries)
{
	struct bootentry *be, *tmp;

	list_for_each_entry_safe(be, tmp, &bootentries->entries, list) {
		list_del(&be->list);
		free(be->title);
		free(be->description);
		free(be->me.display);
		be->release(be);
	}

	if (bootentries->menu) {
		int i;
		for (i = 0; i < bootentries->menu->display_lines; i++)
			free(bootentries->menu->display[i]);
		free(bootentries->menu->display);
	}
	free(bootentries->menu);
	free(bootentries);
}

struct bootentry_script {
	struct bootentry entry;
	char *scriptpath;
};

/*
 * Start a single boot script. 'path' is a full path to a boot script.
 */
static int bootscript_boot(struct bootentry *entry, int verbose, int dryrun)
{
	struct bootentry_script *bs = container_of(entry, struct bootentry_script, entry);
	int ret;

	struct bootm_data data = {};

	if (dryrun) {
		printf("Would run %s\n", bs->scriptpath);
		return 0;
	}

	globalvar_add_simple("linux.bootargs.dyn.ip", NULL);
	globalvar_add_simple("linux.bootargs.dyn.root", NULL);
	globalvar_set_match("linux.bootargs.dyn.", "");

	ret = run_command(bs->scriptpath);
	if (ret) {
		pr_err("Running script '%s' failed: %s\n", bs->scriptpath, strerror(-ret));
		return ret;
	}

	bootm_data_init_defaults(&data);

	if (verbose)
		data.verbose = verbose;
	if (dryrun)
		data.dryrun = dryrun;

	return bootm_boot(&data);
}

static unsigned int boot_watchdog_timeout;

void boot_set_watchdog_timeout(unsigned int timeout)
{
	boot_watchdog_timeout = timeout;
}

static struct watchdog *boot_enabled_watchdog;

struct watchdog *boot_get_enabled_watchdog(void)
{
	return boot_enabled_watchdog;
}

static char *global_boot_default;
static char *global_user;

static int init_boot(void)
{
	global_boot_default = xstrdup("net");
	globalvar_add_simple_string("boot.default", &global_boot_default);
	globalvar_add_simple_int("boot.watchdog_timeout",
				 &boot_watchdog_timeout, "%u");
	globalvar_add_simple("linux.bootargs.base", NULL);
	global_user = xstrdup("none");
	globalvar_add_simple_string("user", &global_user);

	return 0;
}
late_initcall(init_boot);

BAREBOX_MAGICVAR(global.boot.watchdog_timeout,
		"Watchdog enable timeout in seconds before booting");

int boot_entry(struct bootentry *be, int verbose, int dryrun)
{
	int ret;

	pr_info("Booting entry '%s'\n", be->title);

	if (IS_ENABLED(CONFIG_WATCHDOG) && boot_watchdog_timeout) {
		boot_enabled_watchdog = watchdog_get_default();

		ret = watchdog_set_timeout(boot_enabled_watchdog, boot_watchdog_timeout);
		if (ret) {
			pr_warn("Failed to enable watchdog: %s\n", strerror(-ret));
			boot_enabled_watchdog = NULL;
		}
	}

	ret = be->boot(be, verbose, dryrun);
	if (ret)
		pr_err("Booting entry '%s' failed\n", be->title);

	return ret;
}

static void bootsource_action(struct menu *m, struct menu_entry *me)
{
	struct bootentry *be = container_of(me, struct bootentry, me);

	boot_entry(be, 0, 0);

	printf("Press any key to continue\n");

	read_key();
}

static void bootscript_entry_release(struct bootentry *entry)
{
	struct bootentry_script *bs = container_of(entry, struct bootentry_script, entry);

	free(bs->scriptpath);
	free(bs);
}

/*
 * bootscript_create_entry - create a boot entry from a script name
 */
static int bootscript_create_entry(struct bootentries *bootentries, const char *name)
{
	struct bootentry_script *bs;
	enum filetype type;

	type = file_name_detect_type(name);
	if (type != filetype_sh)
		return -EINVAL;

	bs = xzalloc(sizeof(*bs));
	bs->entry.me.type = MENU_ENTRY_NORMAL;
	bs->entry.release = bootscript_entry_release;
	bs->entry.boot = bootscript_boot;
	bs->scriptpath = xstrdup(name);
	bs->entry.title = xstrdup(basename(bs->scriptpath));
	bs->entry.description = basprintf("script: %s", name);
	bootentries_add_entry(bootentries, &bs->entry);

	return 0;
}

/*
 * bootscript_scan_path - create boot entries from a path
 *
 * path can either be a full path to a bootscript or a full path to a directory
 * containing bootscripts.
 */
static int bootscript_scan_path(struct bootentries *bootentries, const char *path)
{
	struct stat s;
	char *files;
	int ret, i;
	int found = 0;
	glob_t globb;

	ret = stat(path, &s);
	if (ret)
		return ret;

	if (!S_ISDIR(s.st_mode)) {
		ret = bootscript_create_entry(bootentries, path);
		if (ret)
			return ret;
		return 1;
	}

	files = basprintf("%s/*", path);

	glob(files, 0, NULL, &globb);

	for (i = 0; i < globb.gl_pathc; i++) {
		char *bootscript_path = globb.gl_pathv[i];

		if (*basename(bootscript_path) == '.')
			continue;

		bootscript_create_entry(bootentries, bootscript_path);
		found++;
	}

	globfree(&globb);
	free(files);

	ret = found;

	return ret;
}

static LIST_HEAD(bootentry_providers);

struct bootentry_provider {
	int (*fn)(struct bootentries *bootentries, const char *name);
	struct list_head list;
};

int bootentry_register_provider(int (*fn)(struct bootentries *bootentries, const char *name))
{
	struct bootentry_provider *p;

	p = xzalloc(sizeof(*p));
	p->fn = fn;

	list_add_tail(&p->list, &bootentry_providers);

	return 0;
}

/*
 * bootentry_create_from_name - create boot entries from a name
 *
 * name can be:
 * - a name of a boot script under /env/boot
 * - a full path of a boot script
 * - a device name
 * - a cdev name
 * - a full path of a directory containing bootloader spec entries
 * - a full path of a directory containing bootscripts
 * - a nfs:// path
 *
 * Returns the number of entries found or a negative error code.
 */
int bootentry_create_from_name(struct bootentries *bootentries,
				      const char *name)
{
	struct bootentry_provider *p;
	int found = 0, ret;

	list_for_each_entry(p, &bootentry_providers, list) {
		ret = p->fn(bootentries, name);
		if (ret > 0)
			found += ret;
	}

	if (IS_ENABLED(CONFIG_COMMAND_SUPPORT) && !found) {
		char *path;

		if (*name != '/')
			path = basprintf("/env/boot/%s", name);
		else
			path = xstrdup(name);

		ret = bootscript_scan_path(bootentries, path);
		if (ret > 0)
			found += ret;

		free(path);
	}

	return found;
}

/*
 * bootsources_menu - show a menu from an array of names
 */
void bootsources_menu(struct bootentries *bootentries, int timeout)
{
	struct bootentry *entry;
	struct menu_entry *back_entry;

	if (!IS_ENABLED(CONFIG_MENU)) {
		pr_warn("no menu support available\n");
		return;
	}

	bootentries_for_each_entry(bootentries, entry) {
		if (!entry->me.display)
			entry->me.display = xstrdup(entry->title);
		entry->me.action = bootsource_action;
		menu_add_entry(bootentries->menu, &entry->me);
	}

	back_entry = xzalloc(sizeof(*back_entry));
	back_entry->display = "back";
	back_entry->type = MENU_ENTRY_NORMAL;
	back_entry->non_re_ent = 1;
	menu_add_entry(bootentries->menu, back_entry);

	if (timeout >= 0)
		bootentries->menu->auto_select = timeout;

	menu_show(bootentries->menu);

	menu_remove_entry(bootentries->menu, back_entry);
	free(back_entry);
}

/*
 * bootsources_list - list boot entries from an array of names
 */
void bootsources_list(struct bootentries *bootentries)
{
	struct bootentry *entry;

	printf("title\n------\n");

	bootentries_for_each_entry(bootentries, entry)
		printf("%s\n\t%s\n", entry->title, entry->description);
}

BAREBOX_MAGICVAR(global.boot.default, "default boot order");
