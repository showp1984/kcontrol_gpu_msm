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
#include <linux/platform_device.h>
#include "mach-msm/clock-local.h"
#include "gpu_msm/adreno.h"
#include "gpu_msm/kgsl_device.h"

#define THIS_EXPERIMENTAL 0

#define DRIVER_AUTHOR "Dennis Rassmann <showp1984@gmail.com>"
#define DRIVER_DESCRIPTION "KControl GPU module for msm devices"
#define DRIVER_VERSION "1.1"
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

struct global_attr_kcontrol {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj,
			struct attribute *attr, char *buf);
	ssize_t (*store)(struct kobject *a, struct attribute *b,
			 const char *c, size_t count);
};

#define define_one_global_ro(_name)		\
static struct global_attr_kcontrol _name =		\
__ATTR(_name, 0444, show_##_name, NULL)

#define define_one_global_rw(_name)		\
static struct global_attr_kcontrol _name =		\
__ATTR(_name, 0644, show_##_name, store_##_name)

/* module parameters */
static uint dev_3d0 = 0x00000000;
module_param(dev_3d0, uint, 0444);

static uint gfx2d0_clk = 0x00000000;
module_param(gfx2d0_clk, uint, 0444);

static uint gfx3d_clk = 0x00000000;
module_param(gfx3d_clk, uint, 0444);

static struct adreno_device *adev = NULL;
static struct kgsl_device *kdev = NULL;
static struct kgsl_pwrctrl *kpwr = NULL;
static struct rcg_clk *rcg2d_clk = NULL;
static struct rcg_clk *rcg3d_clk = NULL;
static struct clk_freq_tbl *clk2dtbl = NULL;
static struct clk_freq_tbl *clk3dtbl = NULL;
static struct clk *clk2d = NULL;
static struct clk *clk3d = NULL;

struct kobject *kcontrol_gpu_msm_kobject;

static ssize_t show_kgsl_pwrlevels(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	int i = 0;
	if (kpwr != NULL) {
		if (kpwr->num_pwrlevels > 0) {
			for (i=0; i<kpwr->num_pwrlevels; i++) {
				len += sprintf(buf + len, "%u\n", kpwr->pwrlevels[i].gpu_freq);
			}
		} else {
			for (i=0; ; i++) {
				len += sprintf(buf + len, "%u\n", kpwr->pwrlevels[i].gpu_freq);
				if ((kpwr->pwrlevels[i].bus_freq == 0))
					break;
			}
		}
	} else {
		len += sprintf(buf + len, "Error! kpwr pointer is null!\n");
	}
	return len;
}
static ssize_t store_kgsl_pwrlevels(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	int i = 0;
	unsigned int pwrlvl = 0;
	long unsigned int hz = 0;
	const char *chz = NULL;
	bool found = false;

	if (kpwr != NULL) {
		for (i=0; i<count; i++) {
			if (buf[i] == ' ') {
				sscanf(&buf[(i-1)], "%u", &pwrlvl);
				chz = &buf[(i+1)];
				found = true;
			}
		}
		if (found == true) {
			sscanf(chz, "%lu", &hz);
			kpwr->pwrlevels[pwrlvl].gpu_freq = hz;
		} else {
			pr_err(LOGTAG"Wrong format! accepting only: <pwrlvl hz>, eg: <0 320000000>\n");
		}
	} else {
		pr_err(LOGTAG"Error! kpwr pointer is null!\n");
	}
	return count;
}
define_one_global_rw(kgsl_pwrlevels);

static ssize_t show_kgsl_iofraction(struct kobject *a, struct attribute *b,
				   char *buf)
{
	ssize_t len = 0;
	int i = 0;

	if (kpwr != NULL) {
		if (kpwr->num_pwrlevels > 0) {
			for (i=0; i<kpwr->num_pwrlevels; i++) {
				len += sprintf(buf + len, "%u\n", kpwr->pwrlevels[i].io_fraction);
			}
		} else {
			for (i=0; ; i++) {
				len += sprintf(buf + len, "%u\n", kpwr->pwrlevels[i].io_fraction);
				if ((kpwr->pwrlevels[i].bus_freq == 0))
					break;
			}
		}
	} else {
		len += sprintf(buf + len, "Error! kpwr pointer is null!\n");
	}
	return len;
}
static ssize_t store_kgsl_iofraction(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	int i = 0;
	unsigned int pwrlvl = 0;
	long unsigned int io = 0;
	const char *cio = NULL;
	bool found = false;

	if (kpwr != NULL) {
		for (i=0; i<count; i++) {
			if (buf[i] == ' ') {
				sscanf(&buf[(i-1)], "%u", &pwrlvl);
				cio = &buf[(i+1)];
				found = true;
			}
		}
		if (found == true) {
			sscanf(cio, "%lu", &io);
			kpwr->pwrlevels[pwrlvl].io_fraction = io;
		} else {
			pr_err(LOGTAG"Wrong format! accepting only: <pwrlvl iofrac>, eg: <0 66>\n");
		}
	} else {
		pr_err(LOGTAG"Error! kpwr pointer is null!\n");
	}
	return count;
}
define_one_global_rw(kgsl_iofraction);

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
	&kgsl_iofraction.attr,
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

	printk(KERN_WARNING LOGTAG "#######################################\n");
#if THIS_EXPERIMENTAL
	printk(KERN_WARNING LOGTAG "WARNING: THIS MODULE IS EXPERIMENTAL!\n");
	printk(KERN_WARNING LOGTAG "You have been warned.\n");
#endif
	printk(KERN_INFO LOGTAG "%s, version %s\n", DRIVER_DESCRIPTION,	DRIVER_VERSION);
	printk(KERN_INFO LOGTAG "author: %s\n", DRIVER_AUTHOR);
	printk(KERN_WARNING LOGTAG "#######################################\n");

	WARN(dev_3d0 == 0x00000000, LOGTAG "dev_3d0 == 0x00000000!");
	//WARN(gfx2d0_clk == 0x00000000, LOGTAG "gfx2d0_clk == 0x00000000!");
	//WARN(gfx3d_clk == 0x00000000, LOGTAG "gfx3d_clk == 0x00000000!");

	if (dev_3d0 != 0x00000000) {
		adev = (struct adreno_device *)dev_3d0;
		kdev = &adev->dev;
		kpwr = &kdev->pwrctrl;
		if (gfx3d_clk != 0x00000000) {
			rcg3d_clk = (struct rcg_clk *)gfx3d_clk;
			if (rcg3d_clk != NULL) {
				clk3dtbl = (struct clk_freq_tbl *)rcg3d_clk->freq_tbl;
				clk3d = &rcg3d_clk->c;
			}
		} else {
			pr_warn(LOGTAG"gfx3d_clk: No address given.\n");
		}

		if (gfx2d0_clk != 0x00000000) {
			rcg2d_clk = (struct rcg_clk *)gfx2d0_clk;
			if (rcg2d_clk != NULL) {
				clk2dtbl = (struct clk_freq_tbl *)rcg2d_clk->freq_tbl;
				clk2d = &rcg2d_clk->c;
			}
		} else {
			pr_warn(LOGTAG"gfx2d0_clk: No address given.\n");
		}

		if (kernel_kobj) {
			rc = sysfs_create_group(kernel_kobj, &kcontrol_gpu_msm_attr_group);
			if (rc) {
				pr_warn(LOGTAG"sysfs: ERROR, could not create sysfs group\n");
			}
		} else
			pr_warn(LOGTAG"sysfs: ERROR, could not find sysfs kobj\n");

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

