/*
 * Playground+ - dynamic.h
 * Header for dynamic file types (so it says here ...)
 * ---------------------------------------------------------------------------
 */

/* a couple of macros to save typing */

#define dynamic_seek_block(df,blocknum) \
  if (lseek(df->data_fd,(blocknum)*(df->granularity),SEEK_SET)<0) \
    handle_error("Failed to seek block");

#define pcp(df) printf("Current position = %d\n",lseek(df->data_fd,0,SEEK_CUR));


/* what the dfile structure looks like */

typedef struct {
  int data_fd;		/* file desc of the open data file */
  char fname[50];	/* the name of the file */
  int nkeys;		/* number of keys in the file */
  int *keylist;         /* pointer to the keys */
  int granularity;      /* size of each block in the data file */
  int first_free_block; /* start of the free block list */
  int first_free_key;   /* start of the free key list */
} dfile;

