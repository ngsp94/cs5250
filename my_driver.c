#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAJOR_NUMBER 61
#define DRIVER_SIZE 4194304

/* forward declaration */
int fourMB_open(struct inode *inode, struct file *filep);
int fourMB_release(struct inode *inode, struct file *filep);
ssize_t fourMB_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
loff_t fourMB_llseek(struct file *fp, loff_t off, int whence);
static void fourMB_exit(void);

/* definition of file_operation structure */
struct file_operations fourMB_fops = {
      read: fourMB_read,
      write: fourMB_write,
      open: fourMB_open,
      release: fourMB_release,
      llseek: fourMB_llseek
};

char *fourMB_data = NULL;
long data_size; // size of valid data

int fourMB_open(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}

int fourMB_release(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}

ssize_t fourMB_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
{
	/*please complete the function on your own*/
	printk(KERN_ALERT "Trying to read %lu\n", count);

	if (*f_pos >= sizeof(fourMB_data)) // reading behind file
		return 0;
	if (count > data_size)
		count = data_size; // reading more than file
	copy_to_user(buf, fourMB_data, count);
	if (*f_pos == 0) { // reading from the start of the byte
		*f_pos += count;
		return count;
	}
	return 0; // exceed 1 byte, nothing read
}

ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{
	/*please complete the function on your own*/
	printk(KERN_ALERT "Trying to write %lu, size: %d\n", count, DRIVER_SIZE);

	// if non-zero, no-space error
	copy_from_user(fourMB_data, buf, count);
	data_size = count;
	if (count > DRIVER_SIZE) {
		data_size = DRIVER_SIZE;
		printk(KERN_ALERT "Err %d: No space left on device!", -ENOSPC);
		return -ENOSPC; 
	}
	return count;
}

loff_t fourMB_llseek(struct file *fp, loff_t off, int whence) {
	loff_t newpos;
	switch (whence) { // type of seek
		case SEEK_SET:
			newpos = off;
			break;
		case SEEK_CUR:
			newpos = fp->f_pos + off;
			break;
		case SEEK_END:
			newpos = data_size + off;
			break;
		default:
			return -EINVAL;
	}
	if (newpos < 0)
		return -EINVAL;
	fp->f_pos = newpos;
	return newpos;
}

static int fourMB_init(void)
{
	int result;
	// register the device
	result = register_chrdev(MAJOR_NUMBER, "fourMB", &fourMB_fops);
	if (result < 0) {
		return result;
	}
	// allocate one byte of memory for storage
	// kmalloc is just like malloc, the second parameter is
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	fourMB_data = kmalloc(sizeof(char)*DRIVER_SIZE, GFP_KERNEL);
	if (!fourMB_data) {
		fourMB_exit();
		// cannot allocate memory
		// return no memory error, negative signify a failure
		return -ENOMEM;
	}
	// initialize the value to be X
	*fourMB_data = 'X';
	data_size = 1;
	printk(KERN_ALERT "This is a 4MB device module\n");
	return 0;
}

static void fourMB_exit(void)
{
	// if the pointer is pointing to something
	if (fourMB_data) {
		// free the memory and assign the pointer to NULL
		kfree(fourMB_data);
		fourMB_data = NULL;
	}
	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "fourMB");
	printk(KERN_ALERT "4MB device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(fourMB_init);
module_exit(fourMB_exit);
