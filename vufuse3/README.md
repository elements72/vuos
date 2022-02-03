# Vufuse3
Vufuse3 is an incomplete porting of Vufuse to the new major release (3.x.x) of [Fuse](https://github.com/libfuse/libfuse).
This major release is incompatible with the previous and for this reason a new module of vufuse is required for providing the new API interface.

## Changes from vufuse
Fuse3 has changed several API function signature, most of them present new parameters that gives more information to the implementation of the file system.

Others function were simply removed because no more useful.

A complete changelog [here](https://github.com/libfuse/libfuse/releases?page=4).

libfuse docs can be find [here](https://libfuse.github.io/doxygen/).

### Function dropped
In vufuse3 this functions were removed from fuse_operations:
* fgetattr
* ftruncate
* utime: now in vu_vufuse3_utimensat is only present the call to `utimensat`, the call to the old `utime` has been removed 
* getdir
### Function signature changed
This API functions receive an additional `struct fuse_file_info` pointer:
* getattr
* chmod
* chown
* truncate
* utimens

In vufuse3 this pointer is set to NULL when passed to these functions, a none NULL value could be passed only if the file is open. 

The `rename` function now takes and additional flags parameters, the values of this flags corresponds to the one in the `renameat2`.

The `filler` function takes an additional [flag](https://libfuse.github.io/doxygen/include_2fuse_8h.html#af2bcf2a473b41b3cc8da8c079656a074) argument. This flag in vufuse3 is ignored passing a standard value and also from the docs of libfuse it seems to be still under development.

The `init` API function requires an additional `struct fuse_config` to adjust high-level specific configuration options. This parameter is set with a memset to 0, because in vufuse [fuse_config](https://libfuse.github.io/doxygen/structfuse__config.html) is still not supperted.

## Version control
The version of the libfuse used for the compilation is automatically selected by a specific script.
```
execute_process(COMMAND bash "-c" "gcc -Wall ${CMAKE_CURRENT_SOURCE_DIR}/fuse_version.c `pkg-config fuse3 --cflags --libs` -o fuse_version && ./fuse_version" OUTPUT_VARIABLE FUSE_VERSION)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=${FUSE_VERSION}")


~ CMake file in vufuse3 folder
```

## Testing modules
For debugging we have chosen to use the letter `Z`. For details see Vudebug documentation.

We have provided 3 testing modules inspired by some of those already present for the previous version. In particular:
### real
Vufuse3real's job is to bypass layer given by modules using libfuse, mapping each "virtual" system call into its "real" implementation.
### null
This module takes care of hiding all files in the directories of the subtree of the mounted folder.
In each directory you will only be able to find `.` And `..`
### unreal
Provides a view of the entire file system in the mount directory and in the same directory under the virtualized file system, for example:
```
    > vumount -t vufuse3unreal none /tmp/mnt
    > ls /tmp/mnt
    bin    core  home   lib64       media  proc  sbin  swapfile  usr
    boot   dev   lib    libx32      mnt    root  snap  sys       var
    cdrom  etc   lib32  lost+found  opt    run   srv   tmp
    > ls /tmp/mnt/tmp/mnt
    bin    core  home   lib64       media  proc  sbin  swapfile  usr
    boot   dev   lib    libx32      mnt    root  snap  sys       var
    cdrom  etc   lib32  lost+found  opt    run   srv   tmp
    > ls /tmp/mnt/tmp/mnt/tmp/mnt
    
```
This modules implements `readdir, getattr, readlik` more `fuse_operations` can be implemented in the future.

### sshfs
Allows you to mount and interact with remote directories using SFTP (Secure File Transfert Protocol) through the use of a client.

#### Compile sshfs for vufuse
For compile sshfs for vufuse3 we added to `meson.build`:
```
    shared_library('sshfs', sshfs_sources,
           include_directories: include_dirs,
           dependencies: sshfs_deps,
           c_args: ['-DFUSE_USE_VERSION=31', '-D_FILE_OFFSET_BITS=64'])
```

Sshfs use the `struct fuse_session` that is not used and supported in vufuse. Fortunately this struct is used only in the `main` function and is not foundamental for sshfs. For make it compatible with vufuse3 all the references to `struct fuse_session` have been removed.

Another not supported structure is `fuse_config`, in the `sshfs_init` all the configuration steps made with this structure were deleted.

#### Testing with sshfs

The module has been tested with the following commands:
* mkdir
* cat
* stat
* vi
* ls (made on a single existing file)
* rm

Problems come (when the cache flag is active) with the `ls` command and, to be more specific, with the `open` and the `close` syscalls. When the `close` syscall occurs we face a segmentation fault during the `releasedir` in sshfs.

After some debugging we discovered that with the cache turned on the real opendir is called only during the `readdir` and not when the `open` syscall is performed, and here comes the troubles.
The cause of the seg. fault is an incorrect use of the `fh` field in  `struct file_info`, this field:
1. Is initialized in `cache_opendir` as a pointer to `struct file_handle`
2. During the `cache_opendir` the real opendir (the one in sshfs.c) is called and `fh` is now a pointer to `struct dir_handle`. The old value of `fh` is mantained in a temporary variable called `cfi`
3. When the `close` syscall occurs `cache_releasedir` is called and it expect to have a `file_handle` (the one saved in cfi), but this has been replace with a `dir_handle` in the real `opendir`. For this reason when `fh` is used as `file_info` it provides unconsistent information and in the next function (releasedir of sshfs) a seg. fault occurs.

The following images shows a naive solution for this problem using a temporary variable `tmp`, that is a copy of `struct file_info`, after the call of opendir.
![](https://i.imgur.com/gT9FD3K.png)

In the original libfuse there is a more complex management of the dir handle and the best approach to solve this problem is understand how Fuse manage the `fh` field and the `dh`. 
A starting point are the following functions in libfuse:
* fuse_lib_opendir
* get_dirhandle
* fuse_lib_releasedir



Note: During testing with this module it emerged that we need to force the `FUSE_VERSION`
to 31 during the compilation of vufuse. (See version control paragraph and version issue in open problems)

## Open problems and related projects


### Sshfs related problem
#### fuse_session issue
Find a better way then delete `fuse_session` references. Adding support to vufuse?
#### Cache issue
Find a way to leave the cache active, understanding how to fix the `fh` problem presented in the sshfs paragraph.
#### Version issue
Sshfs use 3.1 as default `FUSE_USE_VERSION`. When vufuse is compiled with the last version of the library installed (in our tests the 3.2) the following error occurs:
```
umvu: tpp.c:82: __pthread_tpp_change_priority: Assertion `new_prio == -1 || (new_prio >= fifo_min_prio && new_prio <= fifo_max_prio)' failed.
Aborted (core dumped)
```
Sometimes it only ends in seg. fault.


### More accurate handling when working with open files
Using the `private data` field and the `fdprivate` parameter, it is possible to go back to the fuse-type structure relating to the file in question. This structure can be used to pass information about the file itself, useful to initialize some parameters and flags for the system calls (especially if file is open).

### Handling the Fuse parameteres
Vufuse needs to support the fuse options of Fuse in order to properly initialize `fuse_config`, for example:
```
FUSE options:
    ...
    -o hard_remove         immediate removal (don't hide files)
    -o use_ino             let filesystem set inode numbers
    -o readdir_ino         try to fill in d_ino in readdir
    -o direct_io           use direct I/O
    -o kernel_cache        cache files in kernel
    -o [no]auto_cache      enable caching based on modification times (off)
    -o umask=M             set file permissions (octal)
    -o uid=N               set file owner
    -o gid=N               set file group
    ...
```
Vufuse does not handle this parameters and consequently the `fuse_config` parameter, passed to the `init` of a fuse module, is not consistent.

### Manage the enum fuse_readdir_flags
The filler function now takes this new flag that is an enum. Fuse's documentation explain that this flags could be also ignored from the Fuse modules.
### Extend the vufuse3unreal module
The vufuse3unreal module implements only a few functions of `fuse_operations`, adding others functions can be useful for testing. 

## Other useful information

For testing a fuse module (outside vuos) could be useful to print on terminal using the `-d` flag:
```
    ./sshfs -d user@localhost:/home/ /tmp/mnt/
```

An additional file about this project could be found [here](https://docs.google.com/document/d/1nCqck5tbkCClAO85qh4V07V5llt_yEPzpzTNYTdZ1tI/edit#heading=h.28l5q74odxxt).