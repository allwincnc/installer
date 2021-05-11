#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <asm-generic/fcntl.h> // O_RDONLY
#include <asm-generic/errno.h>
#include <asm/uaccess.h>




MODULE_LICENSE("GPL");
MODULE_AUTHOR("MX_Master");
MODULE_DESCRIPTION("Allwinner RISC co-processor (ARISC) admin module.");
MODULE_VERSION("1.0");




#define TEST 0 // 0 = don't write any data to the sys memory
#define DEBUG 1 // 1 = print debug messages

#define DEVICE_CLASS "arisc"
#define DEVICE_NAME "arisc_admin"
#define BUF_LEN 256




static char out_buf[BUF_LEN+2] = {0};
static int out_buf_len = 0;
static int lkm_major_num;
static int lkm_dev_opened = 0;
static struct class* lkm_dev_class = NULL;
static struct device* lkm_dev = NULL;

typedef struct
{
    const char *name;
    unsigned int fw_dest_addr;
    unsigned int fw_max_size;
    unsigned int fw_reset_mem_addr;
    unsigned int pll6_periph0_addr;
    unsigned int cpus_clkcfg_addr;
} cpu_t;

enum { H2, H3, H5, CPU_CNT };

static const cpu_t cpu[CPU_CNT] =
{
    {"H2", 0x00040000, (8+8+32)*1024, 0x01F01C00, 0x01c20028, 0x01f01400},
    {"H3", 0x00040000, (8+8+32)*1024, 0x01F01C00, 0x01c20028, 0x01f01400},
    {"H5", 0x00040000, (8+8+64)*1024, 0x01F01C00, 0x01c20028, 0x01f01400}
};

static int cpu_id = H3;
static char fw_file[BUF_LEN+2] = {0};

// Prototypes for device functions
static int lkm_dev_open(struct inode *, struct file *);
static int lkm_dev_release(struct inode *, struct file *);
static ssize_t lkm_dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t lkm_dev_write(struct file *, const char *, size_t, loff_t *);
static loff_t lkm_dev_llseek(struct file *, loff_t, int);

// This structure points to all of the device functions
static struct file_operations file_ops = {
    .owner   = THIS_MODULE,
    .read    = lkm_dev_read,
    .write   = lkm_dev_write,
    .llseek  = lkm_dev_llseek,
    .open    = lkm_dev_open,
    .release = lkm_dev_release
};




// When a process seek our device, this gets called.
static loff_t
lkm_dev_llseek(struct file *flip, loff_t offset, int whence)
{
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "lkm_dev_llseek\n");
    #endif
    return -ESPIPE;
}

// When a process reads from our device, this gets called.
static ssize_t
lkm_dev_read(struct file *flip, char *buffer, size_t len, loff_t *offset)
{
    int todo_len, i;

    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "lkm_dev_read\n");
    #endif

    if ( !len || !out_buf_len ) return 0;

    todo_len = (len < out_buf_len ? len : out_buf_len);
    if ( (i = copy_to_user(buffer, out_buf, todo_len)) )
    {
        printk(KERN_INFO DEVICE_NAME": " "failed to put some user data\n");
        todo_len = i > todo_len ? 0 : (todo_len - i);
    }
    *offset = 0;

    // cleanup the output buffer
    out_buf_len = 0;
    out_buf[out_buf_len] = 0;

    return (ssize_t) todo_len;
}

// Called when a process tries to write to our device
static ssize_t
lkm_dev_write(struct file *flip, const char *buffer, size_t len, loff_t *offset)
{
    int todo_len, i;
    char *string, *token;
    void __iomem * mmap_addr;
    struct file *f;
    size_t n;
    mm_segment_t fs;
    loff_t file_offset;
    char buf[BUF_LEN+2];
    char in_buf[BUF_LEN+2];
    unsigned long reg_val;

    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "lkm_dev_write\n");
    #endif

    if ( !len ) return 0;

    // cleanup the output buffer
    out_buf_len = 0;
    out_buf[out_buf_len] = 0;

    // get new data string
    todo_len = (len < BUF_LEN ? len : BUF_LEN);
    if ( (i = copy_from_user(in_buf, buffer, todo_len)) )
    {
        printk(KERN_INFO DEVICE_NAME": " "failed to get some user data\n");
        todo_len = i > todo_len ? 0 : (todo_len - i);
    }
    in_buf[todo_len] = 0;
    *offset = 0;

    if ( !todo_len ) return 0;

    // cleanup new data string
    string = strim(in_buf);
    for ( i = strlen(string); i--; )
        if ( string[i] == 10 || string[i] == 13 ) string[i] = 32;

    // parse new data string
    while ( (token = strsep(&string," ")) != NULL )
    {
        // get PLL reg info
        if ( !strcmp(token, "pll6_periph0_get") )
        {
            #define real_addr   (cpu[cpu_id].pll6_periph0_addr)
            #define page_addr   (real_addr & PAGE_MASK)
            #define pages       (real_addr / PAGE_SIZE + 1)
            #define off         (real_addr & ~PAGE_MASK)
            mmap_addr = ioremap(page_addr, pages*PAGE_SIZE);
            reg_val = readl(mmap_addr + off);
            iounmap(mmap_addr);
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:%lu\n", token, reg_val);
            #endif
            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len,
                                    "%s:%lu\n", token, reg_val);
            #undef real_addr
            #undef page_addr
            #undef pages
            #undef off
        }
        else if ( !strcmp(token, "cpus_clkcfg_get") )
        {
            #define real_addr   (cpu[cpu_id].cpus_clkcfg_addr)
            #define page_addr   (real_addr & PAGE_MASK)
            #define pages       (real_addr / PAGE_SIZE + 1)
            #define off         (real_addr & ~PAGE_MASK)
            mmap_addr = ioremap(page_addr, pages*PAGE_SIZE);
            reg_val = readl(mmap_addr + off);
            iounmap(mmap_addr);
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:%lu\n", token, reg_val);
            #endif
            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len,
                                    "%s:%lu\n", token, reg_val);
            #undef real_addr
            #undef page_addr
            #undef pages
            #undef off
        }
        #define real_addr   (cpu[cpu_id].fw_reset_mem_addr)
        #define page_addr   (real_addr & PAGE_MASK)
        #define pages       (real_addr / PAGE_SIZE + 1)
        #define off         (real_addr & ~PAGE_MASK)
        // get arisc core status
        else if ( !strcmp(token, "status") )
        {
            mmap_addr = ioremap(page_addr, pages*PAGE_SIZE);
            reg_val = readl(mmap_addr + off);
            iounmap(mmap_addr);
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:%lu\n", token, reg_val);
            #endif
            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len,
                                    "%s:%lu\n", token, reg_val);
        }
        // stop the arisc core
        else if ( !strcmp(token, "stop") )
        {
            mmap_addr = ioremap(page_addr, pages*PAGE_SIZE);
            #if !TEST
                writel(0UL, mmap_addr + off);
            #endif
            iounmap(mmap_addr);
            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len, "%s:ok\n", token);
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:ok\n", token);
            #endif
        }
        // start the arisc core
        else if ( !strcmp(token, "start") )
        {
            mmap_addr = ioremap(page_addr, pages*PAGE_SIZE);
            #if !TEST
                writel(1UL, mmap_addr + off);
            #endif
            iounmap(mmap_addr);
            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len, "%s:ok\n", token);
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:ok\n", token);
            #endif
        }
        #undef real_addr
        #undef page_addr
        #undef pages
        #undef off
        // erase the firmware
        else if ( !strcmp(token, "erase") )
        {
            mmap_addr = ioremap(cpu[cpu_id].fw_dest_addr,
                               cpu[cpu_id].fw_max_size);
            #if !TEST
                memset_io(mmap_addr, 0, cpu[cpu_id].fw_max_size);
            #endif
            iounmap(mmap_addr);
            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len, "%s:ok\n", token);
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:ok\n", token);
            #endif
        }
        // upload the firmware
        else if ( !strcmp(token, "upload") )
        {
            mmap_addr = ioremap(cpu[cpu_id].fw_dest_addr,
                               cpu[cpu_id].fw_max_size);

            fs = get_fs();
            set_fs(KERNEL_DS);
            f = filp_open(fw_file, O_RDONLY, 0);
            i = 0;
            n = 0;
            file_offset = 0;

            if ( IS_ERR(f) )
            {
                i = 1;
                printk(KERN_INFO DEVICE_NAME": "
                       "failed to open file %s\n", fw_file);
            }
            else
            {
                while ( (n = vfs_read(f, buf, 256, &file_offset)) )
                {
                    if ( file_offset >= cpu[cpu_id].fw_max_size ) break;
                    if ( (file_offset + n) >= cpu[cpu_id].fw_max_size )
                        n = cpu[cpu_id].fw_max_size - file_offset;
                    #if DEBUG
                        printk(KERN_INFO DEVICE_NAME": "
                               "%u bytes copied from file offset %u"
                               "to the mem address 0x%08x\n",
                               (unsigned int) n,
                               (unsigned int) (file_offset - n),
                               (unsigned int) (cpu[cpu_id].fw_dest_addr +
                                               file_offset - n) );
                    #endif
                    #if !TEST
                        memcpy_toio(mmap_addr + file_offset - n, buf, n);
                    #endif
                }
                filp_close(f, NULL);
            }

            set_fs(fs);
            iounmap(mmap_addr);

            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len,
                                    "%s:%s\n", token, (i?"error":"ok"));
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:%s\n",
                       token, (i?"error":"ok"));
            #endif
        }
        // update the firmware file path
        else if ( token[0] == '/' )
        {
            strncpy(fw_file, token, BUF_LEN);

            fs = get_fs();
            set_fs(KERNEL_DS);
            f = filp_open(fw_file, O_RDONLY, 0);
            i = 0;

            if ( IS_ERR(f) )
            {
                i = 1;
                printk(KERN_INFO DEVICE_NAME": "
                       "failed to open file %s\n", fw_file);
            }
            else filp_close(f, NULL);

            set_fs(fs);

            out_buf_len += snprintf(&out_buf[out_buf_len],
                                    BUF_LEN - out_buf_len,
                                    "%s:%s\n", token, (i?"error":"ok"));
            #if DEBUG
                printk(KERN_INFO DEVICE_NAME": " "%s:ok\n", token);
            #endif
        }
        // update CPU id
        else
        {
            for ( i = CPU_CNT; i--; ) if ( !strcmp(token, cpu[i].name) )
            {
                cpu_id = i;
                out_buf_len += snprintf(&out_buf[out_buf_len],
                                        BUF_LEN - out_buf_len,
                                        "%s:ok\n", token);
                #if DEBUG
                    printk(KERN_INFO DEVICE_NAME": " "%s:ok\n", token);
                #endif
                break;
            }
        }
    }

    return (ssize_t) todo_len;
}

// Called when a process opens our device
static int
lkm_dev_open(struct inode *inode, struct file *file)
{
    if ( lkm_dev_opened ) return -EBUSY;
    lkm_dev_opened = 1;
    try_module_get(THIS_MODULE);
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "lkm_dev_open\n");
    #endif
    return 0;
}

// Called when a process closes our device
static int
lkm_dev_release(struct inode *inode, struct file *file)
{
    lkm_dev_opened = 0;
    module_put(THIS_MODULE);
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "lkm_dev_release\n");
    #endif
    return 0;
}




static int __init
lkm_init(void)
{
    // Try to dynamically allocate a major number for the device
    lkm_major_num = register_chrdev(0, DEVICE_NAME, &file_ops);
    if ( lkm_major_num < 0 )
    {
        printk(KERN_ALERT DEVICE_NAME": "
               "failed to register a major number\n");
        return lkm_major_num;
    }
    // cat /proc/devices
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": "
               "registered correctly with major number %d\n", lkm_major_num);
    #endif

    // Register the device class
    lkm_dev_class = class_create(THIS_MODULE, DEVICE_CLASS);
    if ( IS_ERR(lkm_dev_class) )
    {
        unregister_chrdev(lkm_major_num, DEVICE_NAME);
        printk(KERN_ALERT DEVICE_NAME": " "Failed to register device class\n");
        return PTR_ERR(lkm_dev_class);
    }
    // ls /sys/class
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "device class registered correctly\n");
    #endif

    // Register the device driver
    lkm_dev = device_create(lkm_dev_class, NULL, MKDEV(lkm_major_num, 0),
                            NULL, DEVICE_NAME);
    if ( IS_ERR(lkm_dev) )
    {
        class_destroy(lkm_dev_class);
        unregister_chrdev(lkm_major_num, DEVICE_NAME);
        printk(KERN_ALERT DEVICE_NAME": " "Failed to create the device\n");
        return PTR_ERR(lkm_dev);
    }
    // ls /dev/
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "device class created correctly\n");
    #endif

    return 0;
}

static void __exit
lkm_exit(void)
{
    if ( !IS_ERR(lkm_dev_class) )
    {
        if ( lkm_major_num >= 0 )
            device_destroy(lkm_dev_class, MKDEV(lkm_major_num, 0));
        class_unregister(lkm_dev_class);
        class_destroy(lkm_dev_class);
    }
    if ( lkm_major_num >= 0 )
        unregister_chrdev(lkm_major_num, DEVICE_NAME);
    #if DEBUG
        printk(KERN_INFO DEVICE_NAME": " "module unloaded.\n");
    #endif
}

// Register module functions
module_init(lkm_init);
module_exit(lkm_exit);
