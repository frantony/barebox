// SPDX-License-Identifier: GPL-2.0-only
/*
 * resource.c - barebox resource management
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <linux/ioport.h>
#include <linux/err.h>
#include <asm/io.h>

static int init_resource(struct resource *res, const char *name)
{
	INIT_LIST_HEAD(&res->children);
	res->parent = NULL;
	res->name = xstrdup(name);

	return 0;
}

/*
 * request a region.
 * This will succeed when the requested region is completely inside
 * the parent resource and does not conflict with any of the child
 * resources.
 */
struct resource *__request_region(struct resource *parent,
		const char *name, resource_size_t start,
		resource_size_t end)
{
	struct resource *r, *new;

	if (end < start) {
		debug("%s: request region 0x%08llx:0x%08llx: end < start\n",
				__func__,
				(unsigned long long)start,
				(unsigned long long)end);
		return ERR_PTR(-EINVAL);
	}

	/* outside parent resource? */
	if (start < parent->start || end > parent->end) {
		debug("%s: 0x%08llx:0x%08llx outside parent resource 0x%08llx:0x%08llx\n",
				__func__,
				(unsigned long long)start,
				(unsigned long long)end,
				(unsigned long long)parent->start,
				(unsigned long long)parent->end);
		return ERR_PTR(-EINVAL);
	}

	/*
	 * We keep the list of child resources ordered which helps
	 * us searching for conflicts here.
	 */
	list_for_each_entry(r, &parent->children, sibling) {
		if (end < r->start)
			goto ok;
		if (start > r->end)
			continue;
		debug("%s: 0x%08llx:0x%08llx (%s) conflicts with 0x%08llx:0x%08llx (%s)\n",
				__func__,
				(unsigned long long)start,
				(unsigned long long)end,
				name,
				(unsigned long long)r->start,
				(unsigned long long)r->end,
				r->name);
		return ERR_PTR(-EBUSY);
	}

ok:
	debug("%s ok: 0x%08llx:0x%08llx\n", __func__,
			(unsigned long long)start,
			(unsigned long long)end);

	new = xzalloc(sizeof(*new));
	init_resource(new, name);
	new->start = start;
	new->end = end;
	new->parent = parent;
	list_add_tail(&new->sibling, &r->sibling);

	return new;
}

/*
 * release a region previously requested with request_*_region
 */
int release_region(struct resource *res)
{
	if (!list_empty(&res->children))
		return -EBUSY;

	list_del(&res->sibling);
	free((char *)res->name);
	free(res);

	return 0;
}


/*
 * merge two adjacent sibling regions.
 */
int __merge_regions(const char *name,
		struct resource *resa, struct resource *resb)
{
	if (!resource_adjacent(resa, resb))
		return -EINVAL;

	if (resa->start < resb->start)
		resa->end = resb->end;
	else
		resa->start = resb->start;

	free((char *)resa->name);
	resa->name = xstrdup(name);
	release_region(resb);

	return 0;
}

/* The root resource for the whole memory-mapped io space */
struct resource iomem_resource = {
	.start = 0,
	.end = ~(resource_size_t)0,
	.name = "iomem",
	.children = LIST_HEAD_INIT(iomem_resource.children),
};

/*
 * request a region inside the io space (memory)
 */
struct resource *request_iomem_region(const char *name,
		resource_size_t start, resource_size_t end)
{
	return __request_region(&iomem_resource, name, start, end);
}

/* The root resource for the whole io-mapped io space */
struct resource ioport_resource = {
	.start = 0,
	.end = IO_SPACE_LIMIT,
	.name = "ioport",
	.children = LIST_HEAD_INIT(ioport_resource.children),
};

/*
 * request a region inside the io space (i/o port)
 */
struct resource *request_ioport_region(const char *name,
		resource_size_t start, resource_size_t end)
{
	struct resource *res;

	res = __request_region(&ioport_resource, name, start, end);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return res;
}
