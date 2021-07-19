#define FUSE_USE_VERSION  26
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
int device;
struct dirent{
	uint8_t deleted;
	uint8_t entType;
	uint16_t reserved1;
	uint32_t firstBlock;
	uint32_t fileLength;
	uint32_t reserved2;
	uint32_t parentNumber;
	uint32_t selfNumber;
	uint32_t reserved3;
	uint32_t reserved4;
	char fileName[16];


}currentEntry;
static void readNextEntry(){
	read(device,&currentEntry.deleted,1);
	read(device,&currentEntry.entType,1);
	read(device,&currentEntry.reserved1,2);
	read(device,&currentEntry.firstBlock,4);
	read(device,&currentEntry.fileLength,4);
	read(device,&currentEntry.reserved2,4);
	read(device,&currentEntry.parentNumber,4);
	read(device,&currentEntry.selfNumber,4);
	read(device,&currentEntry.reserved3,4);
	read(device,&currentEntry.reserved4,4);
	read(device,&currentEntry.fileName,16);

	

}
static int findEntry(char *path,struct dirent** entry){
	char* token;
	int found;
	int currentDir=0;
	struct dirent* fileToOpen=malloc(sizeof(struct dirent));
	lseek(device,2048,SEEK_SET);
	printf("Path: %s\n",path);

	token=strtok(path,"/");
	//token=strtok(path,"/");
	//syslog(LOG_ERR, "Error %d reading dir id from ROM\n", errno);
	printf("firstfileName: %s\n",token);
	readNextEntry();
	int numEntries=currentEntry.fileLength/48;//Dirent length
	while(token!=NULL){
		lseek(device,2096,SEEK_SET);
		
		printf("token: %s currentDir: %i\n",token,currentDir);

		found=0;
		for(int i=1;i<numEntries;i++){
			readNextEntry();
			if(strcmp(currentEntry.fileName,token)==0&&currentEntry.parentNumber==currentDir){
				printf("currentToken: %s\ncurrent number : %i\n currentDir: %i parent: %i\n",token,currentEntry.selfNumber,currentDir,currentEntry.parentNumber);
				currentDir=currentEntry.selfNumber;
				found=1;
				printf("Found\n");

				break;
			}
			/*if(found==1){
				break;
			}*/
		
		}
		if(found==0){
			printf("NotFound\n");

			break;
		}else{
			printf("Found2\n");

		}
		token=strtok(NULL,"/");
		printf("endToken: %s\n",token);

	}
	if(found==1){
		memcpy(fileToOpen,&currentEntry,sizeof(struct dirent));
		*entry=fileToOpen;
		printf("fileLength: %i\n",(*entry)->fileLength);
		
		return 0;	
	}else{
		printf("ENOENT");
		return -ENOENT;
	}


}
static int testfs_getattr(const char *path, struct stat *stbuf) {
	struct dirent* fileToOpen;
	int ret=0;
	if(strcmp(path,"/")==0){
		stbuf->st_mode=S_IFDIR|0755;
		stbuf->st_nlink=2;
		


	}else{
	ret=findEntry(path,&fileToOpen);
	printf("fileLengthAttr: %i\n",fileToOpen->fileLength);
	if(ret==0){
		switch(fileToOpen->entType){
			case 1:	
				stbuf->st_mode=S_IFREG|0444;
				stbuf->st_nlink=1;
				stbuf->st_size=fileToOpen->fileLength;
				break;
			case 0:
				stbuf->st_mode=S_IFDIR|0755;
				stbuf->st_nlink=2;
				break;
			default:
				printf("Error: type not known");

		}
	}
	}
	return ret;
}
static int testfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	struct dirent* fileToOpen;
	int ret=0;
	int dirNum;
	lseek(device,2048,SEEK_SET);
	filler(buf,".",NULL,0);
	filler(buf,"..",NULL,0);
	
	readNextEntry();
	//filler(buf,currentEntry.fileName,NULL,0);
	int numEntries=currentEntry.fileLength/48;//Dirent length
	if(strcmp(path,"/")==0){
		dirNum=0;
	}else{
		ret=findEntry(path,&fileToOpen);
		dirNum=fileToOpen->selfNumber;
	}
	for(int i=1;i<numEntries;i++){
		readNextEntry();
		if(currentEntry.parentNumber==dirNum){
			filler(buf,currentEntry.fileName,NULL,0);
		}

		
	}
	return 0;

}
static int testfs_open(const char *path, struct fuse_file_info *fi) {
	/*char* token;
	int found;
	struct dirent* fileToOpen=malloc(sizeof(struct dirent));
	fseek(device,2048,SEEK_SET);
	token=strtok(path,"/");
	token=strtok(path,"/");
	readNextEntry();
	int numEntries=currentEntry.fileLength/48;//Dirent length
	for(int i=0;i<numEntries;i++){
		readNextEntry();
		if(strcmp(currentEntry.fileName,token)==0){
			found=1;
			break;
		}
		
	}
	if(found==1){
		memcpy(fileToOpen,currentEntry,sizeof(struct dirent));
		fi->fh=fileToOpen;
		return 0;	
	}else{
		return -ENOENT;
	}*/
	struct dirent* fileToOpen;
	int ret=findEntry(path,&fileToOpen);
	if(ret==0){
		fi->fh=(uint64_t)(size_t)fileToOpen;
	}
	return ret;

}
static int testfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	struct dirent* fileToOpen=(struct dirent*)(size_t)fi->fh;
	//lseek(device,(2048*fileToOpen->firstBlock)+offset,SEEK_SET);
	pread(device,buf,size,(2048*fileToOpen->firstBlock)+offset);
}
static struct fuse_operations testfs_oper = {
	.getattr = testfs_getattr,
	.readdir = testfs_readdir,
	.open    = testfs_open,
	.read    = testfs_read,
};
int main(int argc, char** argv){
	if(argc<3){
		printf("Usage: testfs <dev/file> <mountpoint> <options>\nError: Too few args\n");
		return 1;
	}
	device=open(argv[1],O_RDONLY);
	//printf("%s\n",argv[1]);
	argv[1]=argv[0];
	fuse_main(argc-1, &argv[1], &testfs_oper, NULL);
	return 0;

}

