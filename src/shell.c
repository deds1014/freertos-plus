#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h" // may be the key...

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

extern int romfs_r[256], fs_fs;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void deds1014_command(int, char **);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(deds1014, "done by deds1014")
};

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[])//well, this works weird...
{
	if(n==1)
	{
		fio_printf(2, "\r\nUsage: ls <directory/>, i.e: ls romfs/ \r\n");
		return;
	}


	char buf[1024*16];
	fio_printf(1, "\r\n");
	if(argv[1])//this will print the entire file names in the desired directory AND subdirectories.
	{		   //your terminal will be flooded with file names. I'll modify it later.
		int fn = fs_open(argv[1], O_LS, O_RDONLY);
		int count;

		if(fn == 0)
			fio_printf(1, "\r\n");
		else
			for(int i = 0 ; i < fn ; i++)
				while((count = fio_read(romfs_r[i], buf, sizeof(buf))) > 0)
				{
					fio_write(1, (buf+count-15), 14);
					fio_printf(1, "\r\n");
				}

	}
	else
		return;
}

int filedump(const char *filename)
{
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if(fd==OPENFAIL)
		return 0;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0)
	{
		fio_write(1, buf, count - 12);
	}
	
	//fio_printf(1, "\r\n %d \r\n",(int)buf[count]); // print the length of the file name. this has an memory alignment error, but it works ! (inappropriately)
	//fio_printf(1, "\r\n%d %d %d %d %d %d %d\r\n", romfs_r[0], romfs_r[1], romfs_r[2], romfs_r[3], romfs_r[4], romfs_r[5], romfs_r[6]);

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

	if(!filedump(argv[1]))
		fio_printf(2, "\r\n%s no such file or directory.\r\n", argv[1]);
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

	if(!filedump(buf))
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i=0;i<sizeof(cl)/sizeof(cl[0]); ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

void test_command(int n, char *argv[]) 
{
    //int handle, error;
    int pn = 15, lc, pg = 3;

    fio_printf(1, "\r\n");

    
    //pn = atoi(argv[1]);

    for(int cnt = 2 ; cnt <= pn ; )
    {
    	for(lc = 2 ; lc <= pg - 1 ; lc++)
    	{
    		if(pg % lc == 0)
    			break;
    	}
    	if(lc == pg)
    	{
    		fio_printf(1, "%d\r\n", pg);
    		cnt++;
    	}
    	pg++;
    }
    return;
    

#if 0
    handle = host_action(SYS_OPEN, "syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
#endif
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}

void deds1014_command(int n, char *argv[])
{
	fio_printf(1, "\r\nedited by deds1014\n\r");
	//fio_printf(1, "romfs brute test, fss[0]: %d\n\r", fss.hash/*MAX_FS*/); //no, do not violate the private params of other files...
	return;
}
