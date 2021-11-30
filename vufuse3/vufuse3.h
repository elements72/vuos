#ifndef VUFUSE3_H
#define VUFUSE3_H
#include <fuse3/fuse.h>

/*
#ifndef VUFUSE_FUSE_VERSION
#define VUFUSE_FUSE_VERSION 29
#endif
*/
/** Enable hard remove */
#define FUSE_HARDREMOVE  (1 << 0)

struct fuse {
	void *dlhandle;
	struct fuse_operations fops;

	pthread_mutex_t mutex;

	pthread_t thread;
	pthread_cond_t startloop;
	pthread_cond_t endloop;

	int inuse;
	int fake_chan_fd;
	unsigned long mountflags;
	unsigned long fuseflags;
	void *private_data;
};

struct fileinfo {
	//char *path;
	struct fuse_node *node;
	off_t pos;        /* file offset */
	struct fuse_file_info ffi;    /* includes open flags, file handle and page_write mode  */
	FILE *dirf;
};

struct fuse_context *fuse_push_context(struct fuse_context *new);
void fuse_pop_context(struct fuse_context *old);

#endif
