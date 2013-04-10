/*
 * KControl GPU module for msm devices
 *
 * Copyright (c) 2013 Dennis Rassmann
 * Author: Dennis Rassmann <showp1984@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kallsyms.h>
#include <linux/sysfs.h>
#include <mach/kgsl.h>
#include <linux/platform_device.h>
#include "clock-local.h"

#define THIS_EXPERIMENTAL 0

#define DRIVER_AUTHOR "Dennis Rassmann <showp1984@gmail.com>"
#define DRIVER_DESCRIPTION "KControl GPU module for msm devices"
#define DRIVER_VERSION "1.0"
#define LOGTAG "kcontrol_gpu_msm: "

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

enum vdd_dig_levels {
	VDD_DIG_NONE,
	VDD_DIG_LOW,
	VDD_DIG_NOMINAL,
	VDD_DIG_HIGH
};

struct global_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj,
			struct attribute *attr, char *buf);
	ssize_t (*store)(struct kobject *a, struct attribute *b,
			 const char *c, size_t count);
};

#define define_one_global_ro(_name)		\
static struct global_attr _name =		\
__ATTR(_name, 0444, show_##_name, NULL)

/* module parameters */
static uint kgsl_pdata = 0x00000000;
module_param(kgsl_pdata, uint, 0444);

static uint gfx2d0_clk = 0x00000000;
module_param(gfx2d0_clk, uint, 0444);

static uint gfx3d_clk = 0x00000000;
module_param(gfx3d_clk, uint, 0444);

static struct kgsl_device_platform_data *kpdata;
static struct rcg_clk *rcg2d_clk;
static struct rcg_clk *rcg3d_clk;
static struct clk_freq_tbl *clk2dtbl;
static struct clk_freq_tbl *clk3dtbl;
static struct clk *clk2d;
static struct clk *clk3d;

struct kobject *kcontrol_gpu_msm_kobject;

static ssize_t show_kgsl_pwrlevels(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	int i = 0;
	if (kpdata != NULL) {
		for (i=0; i<kpdata->num_levels; i++) {
			len += sprintf(buf + len, "%u\n", kpdata->pwrlevel[i].gpu_freq);
		}
	} else {
		len += sprintf(buf + len, "Error! kpdata pointer is null!\n");
	}
	return len;
}
define_one_global_ro(kgsl_pwrlevels);

static ssize_t show_version(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	len += sprintf(buf + len, DRIVER_VERSION);
	len += sprintf(buf + len, "\n");
	return len;
}
define_one_global_ro(version);

static ssize_t show_kgsl_avail_2d_clocks(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	int i = 0;
	if (clk2dtbl != NULL) {
		for (i=0; clk2dtbl[i].freq_hz != FREQ_END; i++) {
			len += sprintf(buf + len, "%u\n", clk2dtbl[i].freq_hz);
		}
	} else {
		len += sprintf(buf + len, "Error! clk2dtbl pointer is null!\n");
	}
	return len;
}
define_one_global_ro(kgsl_avail_2d_clocks);

static ssize_t show_kgsl_avail_3d_clocks(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	int i = 0;
	if (clk3dtbl != NULL) {
		for (i=0; clk3dtbl[i].freq_hz != FREQ_END; i++) {
			len += sprintf(buf + len, "%u\n", clk3dtbl[i].freq_hz);
		}
	} else {
		len += sprintf(buf + len, "Error! clk3dtbl pointer is null!\n");
	}
	return len;
}
define_one_global_ro(kgsl_avail_3d_clocks);

static ssize_t show_kgsl_2d_fmax_restraints(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	if (clk2d != NULL) {
		len += sprintf(buf + len, "LOW: %lu\n", clk2d->fmax[VDD_DIG_LOW]);
		len += sprintf(buf + len, "NOMINAL: %lu\n", clk2d->fmax[VDD_DIG_NOMINAL]);
		len += sprintf(buf + len, "HIGH: %lu\n", clk2d->fmax[VDD_DIG_HIGH]);
	} else {
		len += sprintf(buf + len, "Error! clk2d pointer is null!\n");
	}
	return len;
}
define_one_global_ro(kgsl_2d_fmax_restraints);

static ssize_t show_kgsl_3d_fmax_restraints(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	if (clk3d != NULL) {
		len += sprintf(buf + len, "LOW: %lu\n", clk3d->fmax[VDD_DIG_LOW]);
		len += sprintf(buf + len, "NOMINAL: %lu\n", clk3d->fmax[VDD_DIG_NOMINAL]);
		len += sprintf(buf + len, "HIGH: %lu\n", clk3d->fmax[VDD_DIG_HIGH]);
	} else {
		len += sprintf(buf + len, "Error! clk3d pointer is null!\n");
	}
	return len;
}
define_one_global_ro(kgsl_3d_fmax_restraints);

static struct attribute *kcontrol_gpu_msm_attributes[] = {
	&version.attr,
	&kgsl_pwrlevels.attr,
	&kgsl_avail_2d_clocks.attr,
	&kgsl_avail_3d_clocks.attr,
	&kgsl_2d_fmax_restraints.attr,
	&kgsl_3d_fmax_restraints.attr,
	NULL
};

static struct attribute_group kcontrol_gpu_msm_attr_group = {
	.attrs = kcontrol_gpu_msm_attributes,
	.name = "kcontrol_gpu_msm",
};

static int __init kcontrol_gpu_msm_init(void)
{
	int rc = 0;

#if THIS_EXPERIMENTAL
    printk(KERN_WARNING LOGTAG "#######################################");
    printk(KERN_WARNING LOGTAG "WARNING: THIS MODULE IS EXPERIMENTAL!\n");
    printk(KERN_WARNING LOGTAG "You have been warned.\n");
	printk(KERN_INFO LOGTAG "%s, version %s\n", DRIVER_DESCRIPTION,	DRIVER_VERSION);
	printk(KERN_INFO LOGTAG "author: %s\n", DRIVER_AUTHOR);
    printk(KERN_WARNING LOGTAG "#######################################");
#else
	printk(KERN_INFO LOGTAG "%s, version %s\n", DRIVER_DESCRIPTION,	DRIVER_VERSION);
	printk(KERN_INFO LOGTAG "author: %s\n", DRIVER_AUTHOR);
#endif

	WARN(kgsl_pdata == 0x00000000, LOGTAG "kgsl_pdata == 0x00000000!");
	WARN(gfx2d0_clk == 0x00000000, LOGTAG "gfx2d0_clk == 0x00000000!");
	WARN(gfx3d_clk == 0x00000000, LOGTAG "gfx3d_clk == 0x00000000!");

	if ((kgsl_pdata != 0x00000000) && (gfx2d0_clk != 0x00000000) && (gfx3d_clk != 0x00000000)) {
		kpdata = (struct kgsl_device_platform_data *)kgsl_pdata;
		rcg2d_clk = (struct rcg_clk *)gfx2d0_clk;
		rcg3d_clk = (struct rcg_clk *)gfx3d_clk;
		if (rcg2d_clk != NULL) {
			clk2dtbl = (struct clk_freq_tbl *)rcg2d_clk->freq_tbl;
			clk2d = &rcg2d_clk->c;
		}
		if (rcg3d_clk != NULL) {
			clk3dtbl = (struct clk_freq_tbl *)rcg3d_clk->freq_tbl;
			clk3d = &rcg3d_clk->c;
		}

		if (kernel_kobj) {
			rc = sysfs_create_group(kernel_kobj, &kcontrol_gpu_msm_attr_group);
			if (rc) {
				pr_warn(LOGTAG"sysfs: ERROR, could not create sysfs group");
			}
		} else
			pr_warn(LOGTAG"sysfs: ERROR, could not find sysfs kobj");

		pr_info(LOGTAG "everything done, have fun!\n");
	} else {
		pr_err(LOGTAG "Error, you need to insert this module WITH parameters!\n");
		pr_err(LOGTAG "Nothing modified, removing myself!\n");
		return -EAGAIN;
	}
	return 0;
}

static void __exit kcontrol_gpu_msm_exit(void)
{
	sysfs_remove_group(kernel_kobj, &kcontrol_gpu_msm_attr_group);
	printk(KERN_INFO LOGTAG "unloaded\n");
}

module_init(kcontrol_gpu_msm_init);
module_exit(kcontrol_gpu_msm_exit);

