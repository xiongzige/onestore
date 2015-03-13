/*
 * Copyright (C) 2015 Xiongzi Ge. OneStore, extracting file hints
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * 
 */

/*
 *  
 *          Xiongzi Ge, xiongzi@cs.umn.edu;
 
           Here is the interface in the user space to migrate the file data up/down between local and cloud
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/fs.h>
#include <linux/fiemap.h>

#define BLKLIST 20

void syntax(char **argv)
{
	fprintf(stderr, "%s [filename]...\n",argv[0]);
}

struct fiemap *read_fiemap(int fd)
{
	struct fiemap *fiemap;
	int extents_size;

	if ((fiemap = (struct fiemap*)malloc(sizeof(struct fiemap))) == NULL) {
		fprintf(stderr, "Out of memory allocating fiemap\n");	
		return NULL;
	}
	memset(fiemap, 0, sizeof(struct fiemap));

	fiemap->fm_start = 0;
	fiemap->fm_length = ~0;		/* Lazy */
	fiemap->fm_flags = 0;
	fiemap->fm_extent_count = 0;
	fiemap->fm_mapped_extents = 0;

	/* Find out how many extents there are */
	if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
		fprintf(stderr, "fiemap ioctl() failed\n");
		return NULL;
	}

	/* Read in the extents */
	extents_size = sizeof(struct fiemap_extent) * 
                              (fiemap->fm_mapped_extents);

	/* Resize fiemap to allow us to read in the extents */
	if ((fiemap = (struct fiemap*)realloc(fiemap,sizeof(struct fiemap) + 
                                         extents_size)) == NULL) {
		fprintf(stderr, "Out of memory allocating fiemap\n");	
		return NULL;
	}

	memset(fiemap->fm_extents, 0, extents_size);
	fiemap->fm_extent_count = fiemap->fm_mapped_extents;
	fiemap->fm_mapped_extents = 0;

	if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
		fprintf(stderr, "fiemap ioctl() failed\n");
		return NULL;
	}
	
	return fiemap;
}

void dump_fiemap(struct fiemap *fiemap, char *filename)
{
	int i;

	printf("File %s has %d extents:\n",filename, fiemap->fm_mapped_extents);

	printf("#\tLogical          Physical         Length           Flags\n");
	for (i=0;i<fiemap->fm_mapped_extents;i++) {
		printf("%d:\t%-16.16llx %-16.16llx %-16.16llx %-4.4x\n",
			i,
			fiemap->fm_extents[i].fe_logical,
			fiemap->fm_extents[i].fe_physical,
			fiemap->fm_extents[i].fe_length,
			fiemap->fm_extents[i].fe_flags);
		
	}
	printf("\n Now, let's look at them in MB");
		for (i=0;i<fiemap->fm_mapped_extents;i++) {
		printf("%d:\t%lld MB %lld MB %-16.16lld %-4.4x\n",
			i,
			fiemap->fm_extents[i].fe_logical >> 20,
			fiemap->fm_extents[i].fe_physical >> 20 ,
			fiemap->fm_extents[i].fe_length >> 10,
			fiemap->fm_extents[i].fe_flags);
			}	
	printf("\n");
	
}

/*
  Here, let's roughly calculate the mapping from tiered storage block to file extent
*/

int onestore_migrate(struct fiemap *fiemap, char *filename)
{
  
  int i; 
  int n = 0 ;
  char* buf;
  char extent_num[10]; /*maybe less or more...*/
  FILE * fp_mg_down;

  printf("File %s has %d extents:\n",filename, fiemap->fm_mapped_extents);
  n=snprintf(extent_num, 10, "%d/", fiemap->fm_mapped_extents);
  printf("Total extents: %s+ %d\n", extent_num,n); 
    
 __u64 blocknr[fiemap->fm_mapped_extents], phy_blocknr[fiemap->fm_mapped_extents],nrlength[fiemap->fm_mapped_extents];

  buf= (char *) malloc(fiemap->fm_mapped_extents*17*2+n+1);

  if (!buf)
  	{printf("Failed for memory allocation!\n");
    goto enderr;
  	}
  snprintf(buf, n+1, "%s",extent_num);
  
  for (i=0;i<fiemap->fm_mapped_extents;i++)
  {
     snprintf(buf+n+17*2*i,17*2+1,"%-16.16llx/%-16.16llx/",fiemap->fm_extents[i].fe_physical,fiemap->fm_extents[i].fe_length);
  }
  /*
  
   for (i=0;i<fiemap->fm_mapped_extents;i++) {
         	blocknr[i] = fiemap->fm_extents[i].fe_logical >> 20;
			phy_blocknr[i] =fiemap->fm_extents[i].fe_physical >> 20,
		    nrlength[i]=fiemap->fm_extents[i].fe_length >> 20,
			printf("%d:\t%lld lbanr %lld pbanr  %lld length\n",
			i,
			blocknr[i],
			phy_blocknr[i]  ,
			nrlength[i]);
   	}
   */

   /*write to the /sys/sdtiera/btier/migrate_file_down*/

  //assert(n);
  printf("We have : %s \n", buf);
 #if 1 
  fp_mg_down=fopen("/sys/block/sdtiera/tier/migrate_file_down","w");
 #else 
 fp_mg_down=fopen("test","w");
 #endif 
  fputs(buf, fp_mg_down);
  // int f = open("/sys/block/");
  // write(fp_mg_down,buf,1); 
fclose(fp_mg_down);
 
  //system("echo %s >/sys/sdtiera/tier/migrate_file_down", buf); 
  sscanf(buf, "%d/",&i);
  printf("i: %d\n",i);
  __u64 bstart, boffset;
  
  for(n=0;n<i;n++)
 {
   sscanf(buf+2+n*17*2,"%llx/%llx/",&bstart,&boffset);
   printf("extent start %llu offset: %llu\n ", bstart,boffset);
  } 
  
     
  free(buf);
  return 1;
  enderr:
  return 0	;
   
}
int main(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		syntax(argv);
		exit(EXIT_FAILURE);
	}

	for (i=1;i<argc;i++) {
		int fd;

		if ((fd = open(argv[i], O_RDONLY)) < 0) {
			fprintf(stderr, "Cannot open file %s\n", argv[i]);
		}
		else {
			struct fiemap *fiemap;

			if ((fiemap = read_fiemap(fd)) != NULL) 
				dump_fiemap(fiemap, argv[i]);
			    onestore_migrate(fiemap,argv[i]);
			close(fd);
		}
	}
	exit(EXIT_SUCCESS);
}

