#include <common.h>
#include <malloc.h>
#include <driver.h>
#include <init.h>
#include <errno.h>
#include <fs.h>
#include <xfuncs.h>

#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/pagemap.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs_fs_i.h"
#include "squashfs.h"

char *squashfs_devread(struct squashfs_sb_info *fs, int byte_offset,
		int byte_len)
{
	ssize_t size;
	char *buf;

	buf = malloc(byte_len);
	if (buf == NULL)
		return NULL;

	size = cdev_read(fs->cdev, buf, byte_len, byte_offset, 0);
	if (size < 0) {
		dev_err(fs->dev, "read error: %s\n",
				strerror(-size));
		return NULL;
	}

	return buf;
}

static struct inode *duplicate_inode(struct inode *inode)
{
	struct squashfs_inode_info *ei;
	ei = malloc(sizeof(struct squashfs_inode_info));
	if (ei == NULL) {
		ERROR("Error allocating memory for inode\n");
		return NULL;
	}
	memcpy(ei, squashfs_i(inode),
		sizeof(struct squashfs_inode_info));

	return &ei->vfs_inode;
}

struct squashfs_priv {
	struct super_block *sb;
};

static struct inode *squashfs_findfile(struct super_block *sb,
		const char *filename, char *buf)
{
	char *next;
	char fpath[128];
	char *name = fpath;
	struct inode *inode;
	struct inode *t_inode = NULL;

	strcpy(fpath, filename);

	/* Remove all leading slashes */
	while (*name == '/')
		name++;

	inode = duplicate_inode(sb->s_root->d_inode);

	/*
	 * Handle root-direcoty ('/')
	 */
	if (!name || *name == '\0')
		return inode;

	for (;;) {
		/* Extract the actual part from the pathname.  */
		next = strchr(name, '/');
		if (next) {
			/* Remove all leading slashes.  */
			while (*next == '/')
				*(next++) = '\0';
		}

		t_inode = squashfs_lookup(inode, name, 0);
		if (t_inode == NULL)
			break;

		/*
		 * Check if directory with this name exists
		 */

		/* Found the node!  */
		if (!next || *next == '\0') {
			if (buf != NULL)
				sprintf(buf, "%s", name);

			free(squashfs_i(inode));
			return t_inode;
		}

		name = next;

		free(squashfs_i(inode));
		inode = t_inode;
	}

	free(squashfs_i(inode));
	return NULL;
}

static int squashfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev;
	struct squashfs_priv *priv;
	int ret;

	fsdev = dev_to_fs_device(dev);

	priv = xmalloc(sizeof(struct squashfs_priv));
	dev->priv = priv;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_out;


	priv->sb = squashfs_mount(fsdev, 0);
	if (IS_ERR(priv->sb)) {
		dev_info(dev, "no valid squashfs found\n");
		ret = PTR_ERR(priv->sb);
		goto err_out;
	}

	return 0;

err_out:
	free(priv);

	return ret;
}

static void squashfs_remove(struct device_d *dev)
{
	struct squashfs_priv *priv = dev->priv;

	squashfs_put_super(priv->sb);
	free(priv->sb);
	free(priv);
}

static int squashfs_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct squashfs_priv *priv = dev->priv;
	struct inode *inode;
	struct squashfs_page *page;
	int i;

	inode = squashfs_findfile(priv->sb, filename, NULL);
	if (!inode)
		return -ENOENT;

	page = malloc(sizeof(struct squashfs_page));
	page->buf = calloc(32, sizeof(*page->buf));
	for (i = 0; i < 32; i++)
		page->buf[i] = malloc(PAGE_CACHE_SIZE);

	page->data_block = 0;
	page->idx = 0;
	page->real_page.inode = inode;
	file->size = inode->i_size;
	file->priv = page;

	return 0;
}

static int squashfs_close(struct device_d *dev, FILE *f)
{
	struct squashfs_page *page = f->priv;
	int i;

	for (i = 0; i < 32; i++)
		free(page->buf[i]);

	free(page->buf);
	free(squashfs_i(page->real_page.inode));
	free(page);

	return 0;
}

static int squashfs_read(struct device_d *_dev, FILE *f, void *buf,
		size_t insize)
{
	unsigned int size = insize;
	int offset, idx;
	int data_block_pos;
	struct squashfs_page *page = f->priv;

	if (f->pos >= (page->data_block + 1) * 32 * PAGE_CACHE_SIZE) {
		page->data_block++;
		page->idx = 0;
	}

	data_block_pos = f->pos - page->data_block * 32 * PAGE_CACHE_SIZE;
	idx = data_block_pos / PAGE_CACHE_SIZE;
	page->real_page.index = (page->data_block)*32;
	offset = data_block_pos - idx * PAGE_CACHE_SIZE;

	if (page->idx == 0)
		squashfs_readpage(NULL, &page->real_page);

	memcpy(buf, page->buf[idx] + offset, size);

	return insize;
}

static loff_t squashfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;

	return pos;
}

struct squashfs_dir {
	struct file file;
	struct dentry dentry;
	struct dentry root_dentry;
	struct inode inode;
	struct qstr nm;
	DIR dir;
	char d_name[256];
	char root_d_name[256];
};

static DIR *squashfs_opendir(struct device_d *dev, const char *pathname)
{
	struct squashfs_priv *priv = dev->priv;
	struct inode *inode;
	struct squashfs_dir *dir;
	char buf[256];

	inode = squashfs_findfile(priv->sb, pathname, buf);
	if (!inode)
		return NULL;

	dir = xzalloc(sizeof(struct squashfs_dir));
	dir->dir.priv = dir;

	dir->root_dentry.d_inode = inode;

	sprintf(dir->d_name, "%s", buf);
	sprintf(dir->root_d_name, "%s", buf);

	return &dir->dir;
}

static struct dirent *squashfs_readdir(struct device_d *dev, DIR *_dir)
{
	struct squashfs_dir *dir = _dir->priv;
	struct dentry *root_dentry = &dir->root_dentry;

	if (squashfs_lookup_next(root_dentry->d_inode,
				 dir->root_d_name,
				 dir->d_name))
		return NULL;

	strcpy(_dir->d.d_name, dir->d_name);

	return &_dir->d;
}

static int squashfs_closedir(struct device_d *dev, DIR *_dir)
{
	struct squashfs_dir *dir = _dir->priv;

	free(squashfs_i(dir->root_dentry.d_inode));
	free(dir);

	return 0;
}

static int squashfs_stat(struct device_d *dev, const char *filename,
		struct stat *s)
{
	struct squashfs_priv *priv = dev->priv;
	struct inode *inode;

	inode = squashfs_findfile(priv->sb, filename, NULL);
	if (!inode)
		return -ENOENT;

	s->st_size = inode->i_size;
	s->st_mode = inode->i_mode;

	free(squashfs_i(inode));

	return 0;
}

static struct fs_driver_d squashfs_driver = {
	.open		= squashfs_open,
	.close		= squashfs_close,
	.read		= squashfs_read,
	.lseek		= squashfs_lseek,
	.opendir	= squashfs_opendir,
	.readdir	= squashfs_readdir,
	.closedir	= squashfs_closedir,
	.stat		= squashfs_stat,
	.drv = {
		.probe = squashfs_probe,
		.remove = squashfs_remove,
		.name = "squashfs",
	}
};

static int squashfs_init(void)
{
	return register_fs_driver(&squashfs_driver);
}

device_initcall(squashfs_init);
