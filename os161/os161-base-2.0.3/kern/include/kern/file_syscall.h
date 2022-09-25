#ifndef _FILE_SYSCALL_H_
#define _FILE_SYSCALL_H_

struct file_handle {
        char *name;
        int flags;
        off_t offset;        // create a offset type for manage append
        struct lock *lock;   // create a lock struct
        struct vnode *vnode; // create a struct vnode
};

int sys_open(const char *filename, int flags);
int sys_read(int fd, void *buff, size_t buff_len);
int sys_write(int fd, const void *buff, size_t buff_len);
int sys_lseek(int fd, off_t pos, int whence);
int sys_close(int fd);
int sys_dup2(int oldfd, int newfd);
int sys_chdir(const char *pathname);
int sys_getcwd(char *buff, size_t buff_len);

#endif
