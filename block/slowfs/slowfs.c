#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mnt_idmapping.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/stat.h>

#define MODEL_PREFIX "slowfs: "
#define p_err(fmt, ...) pr_err(MODEL_PREFIX fmt "\n", ##__VA_ARGS__)
#define p_info(fmt, ...) pr_info(MODEL_PREFIX fmt "\n", ##__VA_ARGS__)

#define FILENAME_LENGTH 255
#define MAX_FILES 1024

struct file_item {
    char filename[FILENAME_LENGTH];
    char *buffer;
    int buflen;
};

struct {
    struct file_item files[MAX_FILES];
    int file_count;
} slowfs_info = {.file_count = 0};

static int slowfs_next_file_idx(void) {
    if (slowfs_info.file_count >= MAX_FILES) {
        return -ENOSPC;
    }
    return slowfs_info.file_count;
}

static const struct inode_operations slowfs_inode_ops;
static const struct file_operations slowfs_file_ops;

static int slowfs_inode_create(struct mnt_idmap *idmap, struct inode *dir,
                               struct dentry *dentry, umode_t mode, bool excl) {
    if (!S_ISDIR(mode) && !S_ISREG(mode)) {
        return -EINVAL;
    }

    struct inode *inode = new_inode(dir->i_sb);
    inode->i_sb = dir->i_sb;
    inode->i_op = &slowfs_inode_ops;
    inode->i_fop = &slowfs_file_ops;

    int idx = slowfs_next_file_idx();
    if (idx < 0) {
        return idx;
    }

    strncpy(slowfs_info.files[idx].filename, dentry->d_name.name,
            FILENAME_LENGTH);
    slowfs_info.files[idx].buffer = kmalloc(1024, GFP_KERNEL);
    slowfs_info.files[idx].buflen = 0;

    inode->i_ino = idx;
    inode->i_private = &slowfs_info.files[idx];

    p_info("create file: %s", dentry->d_name.name);

    inode_init_owner(idmap, inode, dir, mode);

    d_add(dentry, inode);

    return 0;
}

static const struct inode_operations slowfs_inode_ops = {
    .create = slowfs_inode_create,
};

static const struct file_operations slowfs_file_ops = {

};

static int slowfs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode *root_inode = new_inode(sb);
    if (!root_inode) {
        return -ENOMEM;
    }

    inode_init_owner(&nop_mnt_idmap, root_inode, NULL, S_IFDIR);

    root_inode->i_sb = sb;
    root_inode->i_op = &slowfs_inode_ops;
    root_inode->i_fop = &slowfs_file_ops;

    struct timespec64 timespace = current_time(root_inode);
    inode_set_atime_to_ts(root_inode, timespace);
    inode_set_mtime_to_ts(root_inode, timespace);
    inode_set_ctime_to_ts(root_inode, timespace);

    sb->s_root = d_make_root(root_inode);

    return 0;
}

static struct dentry *slowfs_mount(struct file_system_type *fs_type, int flags,
                                   const char *dev_name, void *data) {
    return mount_bdev(fs_type, flags, dev_name, data, slowfs_fill_super);
}

static void slowfs_kill_sb(struct super_block *sb) {}

static struct file_system_type slowfs_type = {
    .owner = THIS_MODULE,
    .name = "slowfs",
    .mount = slowfs_mount,
    .kill_sb = slowfs_kill_sb,
    .next = NULL,
};

static int __init slowfs_init(void) {
    p_info("init");

    int ret = register_filesystem(&slowfs_type);
    if (ret) {
        p_err("register failed");
        return ret;
    }
    p_info("loaded");
    return 0;
}

static void __exit slowfs_exit(void) { p_info("slowfs: exit\n"); }

module_init(slowfs_init);
module_exit(slowfs_exit);
MODULE_LICENSE("GPL");
