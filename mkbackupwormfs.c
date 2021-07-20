#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
struct fsdirent{
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


}dent;
FILE* output;
int fileNumber=1;
static void writeEntry(FILE* f,struct fsdirent *d){
	fwrite(&d->deleted,1,1,f);
	fwrite(&d->entType,1,1,f);
	fwrite(&d->reserved1,2,1,f);
	fwrite(&d->firstBlock,4,1,f);
	fwrite(&d->fileLength,4,1,f);
	fwrite(&d->reserved2,4,1,f);
	fwrite(&d->parentNumber,4,1,f);
	fwrite(&d->selfNumber,4,1,f);
	fwrite(&d->reserved3,4,1,f);
	fwrite(&d->reserved4,4,1,f);
	fwrite(&d->fileName,1,16,f);

	

}
int writeZeroes(FILE* f,int amount){
	char nul=0;
	for(int i=0;i<amount;i++){
		fwrite(&nul,1,1,f);
	}
}
int getNumberOfFiles(char *path,int* num){
	struct stat statbuf;
	int dirnum=0;
	struct dirent *dp;
    	DIR *dir = opendir(path);
	if (!dir)
        	return 1;
	chdir(path);
	while((dp = readdir(dir)) != NULL) {
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        	{
            		printf("Name: %s\n",dp->d_name);

			dirnum++;
			lstat(dp->d_name,&statbuf);
			if(S_ISDIR(statbuf.st_mode)){
				printf("Isdir\n");
            			getNumberOfFiles(dp->d_name,num);
			}
		}
        }
	chdir("..");
	closedir(dir);
	printf("Dirnum: %i\n",dirnum);

	*num+=dirnum;
	return 0;

}
int writeDirEnt(FILE* f,char* path,int parentNum,int firstSector){
	int sect=firstSector;
	//int curNum=parentNum
	struct stat statbuf;
	
	struct dirent *dp;
    	DIR *dir = opendir(path);
	if (!dir)
        	return 1;
	chdir(path);
	while((dp = readdir(dir)) != NULL) {
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        	{
            		
			if(strlen(dp->d_name)<15){
				
			
				memset(&dent,0,sizeof(struct fsdirent));
				dent.deleted=1;
				dent.reserved1=0;
				dent.reserved2=0;
				dent.reserved3=0;
				dent.reserved4=0;
				dent.selfNumber=fileNumber;
				dent.parentNumber=parentNum;
				strcpy(dent.fileName,dp->d_name);
				lstat(dp->d_name,&statbuf);
				fileNumber++;
				if(S_ISDIR(statbuf.st_mode)){
					dent.entType=0;
					dent.firstBlock=0;
					dent.fileLength=0;
					writeEntry(f,&dent);

            				writeDirEnt(f,dp->d_name,fileNumber-1,sect);
				}else{
					dent.entType=1;
					writeEntry(f,&dent);


				}
				

			}
		}
        }
	chdir("..");
	closedir(dir);



}
int writeSystemEntry(FILE* f,int dirLength){
	memset(&dent,0,sizeof(struct fsdirent));
	dent.deleted=1;
	dent.entType=2;
	dent.firstBlock=1;
	dent.fileLength=dirLength;
	dent.selfNumber=1;
	dent.parentNumber=0;
	strcpy(dent.fileName,"DIR.SYS");
	writeEntry(f,&dent);

}
int main(int argc, char** argv){
	char nul=0;
	int num=0;
	output=fopen("test.img","wb");
	getNumberOfFiles("/home/alex/Desktop/testfs",&num);
	int directorySize=(num*48)+48;//first special entry
	writeZeroes(output,2048);//Reserved cluster
	printf("Num: %i",num);
	writeSystemEntry(output,directorySize);
	fileNumber++;
	writeDirEnt(output,"/home/alex/Desktop/testfs",0,3);
	return 0;
}
