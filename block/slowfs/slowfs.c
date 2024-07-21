#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>


static int __init slowfs_init(void) {
  printk("slowfs: init\n");
  return 0;
}

static void __exit slowfs_exit(void) {
  printk("slowfs: exit\n");
}

module_init(slowfs_init);
module_exit(slowfs_exit);
MODULE_LICENSE("GPL");
