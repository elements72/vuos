#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Path of the unreal folder
char unreal[PATH_MAX];

char *get_real_path(const char *path)
{
    char base_path[PATH_MAX];
    size_t unreal_len = strlen(unreal);
    strncpy(base_path, path, unreal_len);
    base_path[unreal_len] = '\0';
    if(base_path[unreal_len -1] != '/')
        strcat(base_path, "/");

    if (strcmp(base_path, unreal) == 0){
        if(strlen(path) != unreal_len - 1)
            return (char *) path + unreal_len - 1;
        return "/";
    }
    return (char *) path;
}

static int op_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    struct stat stats;
    if(lstat(get_real_path(path), &stats) == 0)
        *stbuf = stats;
    else
        return -ENOENT;
    return 0;
}

int op_readlink(const char *path, char *buf, size_t size)
{
    memset(buf, '\0', size);
    int rv = readlink(get_real_path(path), buf, size);

    return rv;
}

static int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags)
{
    DIR *d;
    struct dirent *dir;
    char *real_path = get_real_path(path);
    d = opendir(real_path);
    while ((dir = readdir(d)) != NULL) {
        struct stat stbuf;
        if (lstat(dir->d_name, &stbuf) >= 0)
        {
            filler(buf, dir->d_name, &stbuf, 0, flags);
        }
        else
            filler(buf, dir->d_name, NULL, 0, flags);
    }
    return 0;
}

static const struct fuse_operations unreal_ops = {
    .getattr = op_getattr,
    .readdir = op_readdir,
    .readlink = op_readlink,
};

int main(int argc, char *argv[]) {
    strcpy(unreal, argv[argc-1]);
    if(unreal[strlen(unreal)-1] != '/')
        strcat(unreal, "/");
    return fuse_main(argc, argv, &unreal_ops, NULL);
}