//Ahmad Malik
//ECE357
//PSet1 -P3
//this program doesnt fully compile or read/write from a file, but I think the mechanics are there
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

struct MYSTREAM{
    int Fd;
    int Mode;
    int bufSize;
    int Index;
    int buffCount;
    char *buffer;
};

struct MYSTREAM *myfdopen(int filedesc, int mode, int bufsiz){
    struct MYSTREAM *fdStream = malloc(sizeof(struct MYSTREAM));
    if (fdStream == NULL){
        errno = ENOMEM;
        return NULL;
    }
    fdStream->Fd= filedesc;
    fdStream->Mode = mode;
    fdStream->bufSize = bufsiz;
    fdStream->Index = 0;
    fdStream->buffCount = 0;
    fdStream->buffer = malloc(bufsiz);
    return fdStream;
}

struct MYSTREAM *myfopen(const char *pathname, int mode, int bufsiz){
    //Error amd Mode Checking
    if (mode != O_RDONLY && mode != O_WRONLY){
        errno = EINVAL;
        return NULL;
    }
    if (bufsiz <= 0){
        errno = EINVAL;
        return NULL;
    }
    int fd ;
    if (mode == 0){
        fd = open(pathname , O_RDONLY);
    }
    else if (mode == 1){
       fd = open( pathname , O_WRONLY | O_CREAT , 0777 );
    }
   else{
        return NULL ;
    }
    struct MYSTREAM *stream = myfdopen(fd, mode, bufsiz);
    return stream;
};


int myfgetc(struct MYSTREAM *stream){
    if (stream->buffCount == stream->Index){
        int Read = read(stream->Fd, stream->buffer, stream->bufSize);
        if (Read < 0){
        errno = 0;
        return -1;
        }
    }
    return stream -> buffer [stream -> Index ++] ;
}


int myfputc ( int c , struct MYSTREAM *stream ){
    stream->buffer[ stream -> Index] = c;
    stream-> Index++;
    if ( stream -> bufSize == stream -> Index){
        int Write = write (stream -> Fd , stream -> buffer , stream -> bufSize);
        stream -> Index = 0 ;
        if (Write <= 0){
            return -1;
        }
        else{
            return c;
        }
}

int myfclose ( struct MYSTREAM *stream ){
    if (stream -> Mode == O_WRONLY){
        int Write = write ( stream -> Fd , stream -> buffer , stream -> bufSize ) ;
        if(Write<=0){
            return -1;
        }
    }
    int x = close (stream -> Fd) ;
    if (x == 0){
        return 0 ;
    }
    else{
        return -1;
    }
    free (stream);
   }
};

int main () {
   struct MYSTREAM *in = myfopen( "output.txt " , O_RDONLY , 1024);
   struct MYSTREAM *out = myfopen( "ouptut.txt" , O_WRONLY , 1);
   //myfclose(in) ;
   //myfclose(out) ;
   return 0 ;
}
