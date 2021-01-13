/*
 * arch/xtensa/platforms/iss/simdisk.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001-2013 Tensilica Inc.
 *   Authors	Victor Prupis
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>

#define SIMDISK_MAJOR 240
#define SECTOR_SHIFT 9
#define SIMDISK_MINORS 1
#define MAX_SIMDISK_COUNT 10

static int hardsect_size = 512;
module_param(hardsect_size, int, 0);
static int nsectors = 10240;  /* How big the drive is */
module_param(nsectors, int, 0);

struct simdisk {
	spinlock_t lock;
	struct request_queue *queue;
	struct gendisk *gd;
	struct proc_dir_entry *procfile;
	int users;
	unsigned long size;
	char *diskdata;
};

static int errno;
static int simdisk_count = 1;
module_param(simdisk_count, int, S_IRUGO);
MODULE_PARM_DESC(simdisk_count, "Number of simdisk units.");

static int n_files;

static int simdisk_major = SIMDISK_MAJOR;

static void simdisk_transfer(struct simdisk *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
	loff_t offset = sector << SECTOR_SHIFT;
	unsigned long nbytes = nsect << SECTOR_SHIFT;

	if (offset > dev->size || dev->size - offset < nbytes) {
		pr_notice("Beyond-end %s (%ld %ld)\n",
				write ? "write" : "read", offset, nbytes);
		return;
	}

	spin_lock(&dev->lock);
	while (nbytes > 0) {
		ssize_t io=nbytes;

		if (write)
			memcpy(dev->diskdata + offset, buffer, nbytes);
		else
			memcpy(buffer, dev->diskdata + offset, nbytes);
		if (io == -1) {
			pr_err("SIMDISK: IO error %d\n", errno);
			break;
		}
		buffer += io;
		offset += io;
		nbytes -= io;
	}
	spin_unlock(&dev->lock);

}

static blk_qc_t simdisk_make_request(struct request_queue *q, struct bio *bio)
{
	struct simdisk *dev = q->queuedata;
	struct bio_vec bvec;
	struct bvec_iter iter;
	sector_t sector = bio->bi_iter.bi_sector;

	bio_for_each_segment(bvec, bio, iter) {
		char *buffer = __bio_kmap_atomic(bio, iter);
		unsigned len = bvec.bv_len >> SECTOR_SHIFT;

		simdisk_transfer(dev, sector, len, buffer,
				bio_data_dir(bio) == WRITE);
		sector += len;
		__bio_kunmap_atomic(buffer);
	}

	bio_endio(bio);

	return BLK_QC_T_NONE;
}

static int simdisk_open(struct block_device *bdev, fmode_t mode)
{
	struct simdisk *dev = bdev->bd_disk->private_data;

	spin_lock(&dev->lock);
	if (!dev->users)
		check_disk_change(bdev);
	++dev->users;
	spin_unlock(&dev->lock);
	return 0;
}

static void simdisk_release(struct gendisk *disk, fmode_t mode)
{
	struct simdisk *dev = disk->private_data;
	spin_lock(&dev->lock);
	--dev->users;
	spin_unlock(&dev->lock);
}

static const struct block_device_operations simdisk_ops = {
	.owner		= THIS_MODULE,
	.open		= simdisk_open,
	.release	= simdisk_release,
};

static struct simdisk *sddev;
static struct proc_dir_entry *simdisk_procdir;


static int __init simdisk_setup(struct simdisk *dev, int which,
		struct proc_dir_entry *procdir)
{
	spin_lock_init(&dev->lock);
	dev->users = 0;
	dev->size = nsectors*hardsect_size;
	dev->diskdata = vmalloc(dev->size);
    	if (dev->diskdata == NULL)
		return -ENOMEM;

	dev->queue = blk_alloc_queue(GFP_KERNEL);
	if (dev->queue == NULL) {
		pr_err("blk_alloc_queue failed\n");
		goto out_alloc_queue;
	}

	blk_queue_make_request(dev->queue, simdisk_make_request);
	dev->queue->queuedata = dev;

	dev->gd = alloc_disk(SIMDISK_MINORS);
	if (dev->gd == NULL) {
		pr_err("alloc_disk failed\n");
		goto out_alloc_disk;
	}
	dev->gd->major = simdisk_major;
	dev->gd->first_minor = which;
	dev->gd->fops = &simdisk_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf(dev->gd->disk_name, 32, "simdisk%d", which);
	set_capacity(dev->gd, dev->size >> SECTOR_SHIFT);
	add_disk(dev->gd);
	return 0;

out_alloc_disk:
	blk_cleanup_queue(dev->queue);
	dev->queue = NULL;
out_alloc_queue:
	return -EIO;
}

static int __init simdisk_init(void)
{
	int i;

	if (register_blkdev(simdisk_major, "simdisk") < 0) {
		pr_err("SIMDISK: register_blkdev: %d\n", simdisk_major);
		return -EIO;
	}
	pr_info("SIMDISK: major: %d\n", simdisk_major);

	if (n_files > simdisk_count)
		simdisk_count = n_files;
	if (simdisk_count > MAX_SIMDISK_COUNT)
		simdisk_count = MAX_SIMDISK_COUNT;

	sddev = kmalloc(simdisk_count * sizeof(struct simdisk),
			GFP_KERNEL);
	if (sddev == NULL)
		goto out_unregister;

	for (i = 0; i < simdisk_count; ++i) {
		if(simdisk_setup(sddev + i, i, simdisk_procdir) != 0) 
		{
			printk("simdisk_setup error\n");
		}
	}

	return 0;

	kfree(sddev);
out_unregister:
	unregister_blkdev(simdisk_major, "simdisk");
	return -ENOMEM;
}
module_init(simdisk_init);

static void simdisk_teardown(struct simdisk *dev, int which,
		struct proc_dir_entry *procdir)
{
	if (dev->gd)
		del_gendisk(dev->gd);
	if (dev->queue)
		blk_cleanup_queue(dev->queue);
}

static void __exit simdisk_exit(void)
{
	int i;

	for (i = 0; i < simdisk_count; ++i)
		simdisk_teardown(sddev + i, i, simdisk_procdir);
	kfree(sddev);
	unregister_blkdev(simdisk_major, "simdisk");
}
module_exit(simdisk_exit);

MODULE_ALIAS_BLOCKDEV_MAJOR(SIMDISK_MAJOR);

MODULE_LICENSE("GPL");
