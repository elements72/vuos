#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>

int main(){ 
    printf("%d", FUSE_VERSION);
}