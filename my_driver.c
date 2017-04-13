#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h> /* needed for _IOW etc */
#include <asm/uaccess.h>


#define MAJOR_NUMBER 61
#define DRIVER_SIZE 4194304
#define MSG_SIZE 20
#define SCULL_IOC_MAGIC 'k'
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_SET _IOW(SCULL_IOC_MAGIC, 2, char *)
#define SCULL_READ _IOR(SCULL_IOC_MAGIC, 3, char *)
#define SCULL_READ_WRITE _IOWR(SCULL_IOC_MAGIC, 4, char *)
#define SCULL_IOC_MAXNR 4

/* forward declaration */
int fourMB_open(struct inode *inode, struct file *filep);
int fourMB_release(struct inode *inode, struct file *filep);
ssize_t fourMB_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
loff_t fourMB_llseek(struct file *fp, loff_t off, int whence);
long ioctl_example(struct file *fp, unsigned int cmd, unsigned long arg);
static void fourMB_exit(void);

/* definition of file_operation structure */
struct file_operations fourMB_fops = {
      read: fourMB_read,
      write: fourMB_write,
      open: fourMB_open,
      release: fourMB_release,
      llseek: fourMB_llseek,
      .unlocked_ioctl = ioctl_example
};

char *fourMB_data = NULL;
char *dev_msg = NULL;
long data_size; // size of valid data
int msg_size; // size of valid msg

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

	if (*f_pos >= data_size) // reading behind file
		return 0;
	if (count > data_size)
		count = data_size; // reading more than file
	
	// read according to fpos
	copy_to_user(buf, fourMB_data + *f_pos, count);
		
	*f_pos += count; // update fpos
	return count;

}

ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{
	/*please complete the function on your own*/
	printk(KERN_ALERT "Trying to write %lu, size: %d\n", count, DRIVER_SIZE);

	// if non-zero, no-space error
	copy_from_user(fourMB_data + *f_pos, buf, count);
	if (*f_pos + count > DRIVER_SIZE) {
		data_size = DRIVER_SIZE;
		printk(KERN_ALERT "Err %d: No space left on device!", -ENOSPC);
		data_size = DRIVER_SIZE;
		return -ENOSPC; 
	}
	
	// update current data size
	if (*f_pos + count > data_size)
		data_size = *f_pos + count;

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


long ioctl_example(struct file *fp, unsigned int cmd, unsigned long arg)
{

	int err = 0, i;
	int retval = 0;
	char *user_msg;
	char tmp[MSG_SIZE];

	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) * _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
	switch (cmd) {
		case SCULL_HELLO:
			printk(KERN_WARNING "hello\n");
			break;
		case SCULL_SET:
			// copy to capacity
			user_msg = (char __user *) arg;
			copy_from_user(dev_msg, user_msg, MSG_SIZE);
			printk(KERN_WARNING "dev_msg is now: %s\n", dev_msg);
			// get length of user msg
			for (i=0; i<MSG_SIZE; i++)
				if (user_msg[i] == '\0')
					break;
			msg_size = i;
			retval = i;
			break;
		case SCULL_READ:
			// copy msg to user space
			user_msg = (char __user *) arg;
			retval = copy_to_user(user_msg, dev_msg, msg_size+1);
			break;
		case SCULL_READ_WRITE:
			// copy dev msg to swap later
			user_msg =  (char __user *) arg;
			i = 0;
			for (i=0; i<msg_size; i++)
				tmp[i] = dev_msg[i];
			tmp[msg_size] = '\0';

			copy_from_user(dev_msg, user_msg, MSG_SIZE);
			copy_to_user(user_msg, tmp, msg_size+1);
			printk(KERN_WARNING "dev_msg is now: %s\n", dev_msg);
			msg_size = i;
			retval = i;
			break;
			
		default:
			return -ENOTTY;
	}
	return retval;
}


static int fourMB_init(void)
{
	int result;
	// register the device
	result = register_chrdev(MAJOR_NUMBER, "fourMB", &fourMB_fops);
	if (result < 0) {
		return result;
	}
	// allocate 4MB of memory for storage
	// kmalloc is just like malloc, the second parameter is
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	fourMB_data = kmalloc(sizeof(char)*DRIVER_SIZE, GFP_KERNEL);
	dev_msg = kmalloc(sizeof(char)*MSG_SIZE, GFP_KERNEL);
	if (!fourMB_data || !dev_msg) {
		fourMB_exit();
		// cannot allocate memory
		// return no memory error, negative signify a failure
		return -ENOMEM;
	}

	// initialize the value to be X
	*fourMB_data = 'X';
	data_size = 1;

	*dev_msg = 'A';
	msg_size = 1;
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
		kfree(dev_msg);
		dev_msg = NULL;
	}
	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "fourMB");
	printk(KERN_ALERT "4MB device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(fourMB_init);
module_exit(fourMB_exit);
