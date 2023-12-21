#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/time.h>

MODULE_DESCRIPTION("TSU lab");
MODULE_AUTHOR("Karim");
MODULE_LICENSE("GPL");

// Define the name for the proc file
#define PROCFS_NAME "tsu"
static struct proc_dir_entry *tsu_proc_file = NULL;
static time64_t last_view_time;

// Function to handle reading from the proc file
static ssize_t tsu_procfile_read(
    struct file *file_pointer, char __user *buffer,
    size_t buffer_length, loff_t *offset
) {
    // Get the current time
    time64_t now = ktime_get_real_seconds();

    // Check if the file has been read in the last 20 seconds
    if (now - last_view_time <= 20) {
        pr_info("procfile don't work\n");
        return 0;
    }
    // Update the last view time
    last_view_time = now;

    // Message to be written to the proc file
    char msg[] = "Hello\n";
    size_t msg_len = strlen(msg);

    // Copy the message to user space
    copy_to_user(buffer, msg, msg_len);

    // Print information about the read operation
    pr_info("procfile read(%lld): %s\n", now, file_pointer->f_path.dentry->d_name.name);

    return msg_len;
}

// Define proc file operations structure
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops tsu_proc_file_fops = {
    .proc_read = tsu_procfile_read,
};
#else
static const struct file_operations tsu_proc_file_fops = {
    .read = tsu_procfile_read,
};
#endif

// Module initialization function
int init_module(void) {
    pr_info("Welcome to Tomsk State University\n");

    // Initialize the last view time to 30 seconds ago
    last_view_time = ktime_get_real_seconds() - 30;

    // Create the proc file
    tsu_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &tsu_proc_file_fops);

    return 0;
}

// Module cleanup function
void cleanup_module(void) {
    // Remove the proc file
    proc_remove(tsu_proc_file);
    
    pr_info("Tomsk State University forever!\n");
}

