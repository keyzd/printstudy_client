#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PRINTER_NAME "Canon-MF3010"
#define PIN_LEN 4

#define FNAME_LEN 256

struct str_list
{
	char *name;
	char *print_count;
	struct str_list* next;
	struct str_list* prev;
};
void free_list_from_head(struct str_list *head);
void free_list_from_tail(struct str_list *tail);

void transfer_pin(int serv_sd, char *pin_str);
int get_files_num(int serv_sd);
struct str_list* download_files(int serv_sd, int files_num);
void print_files(struct str_list *fnames);
void convert2pdf(char *fname);

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		printf("Usage: ./client <address> <port> <PIN>\n");
		exit(1);
	}

	char *address = argv[1];
	short port = atoi(argv[2]);
	char *pin_str = argv[3];

	int files_num;
	struct str_list *fnames;

	int serv_sd = socket(AF_INET, SOCK_STREAM, 0);
	assert(serv_sd != -1);

	int iMode = 0;
	ioctl(serv_sd, FIONBIO, &iMode);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if(!inet_aton(address, &(addr.sin_addr)))
	{
		printf("Wrong server address\n");
		exit(2);
	}

	int connect_flag = connect(serv_sd, (struct sockaddr*)&addr, sizeof(addr));
	assert(connect_flag != -1);

	transfer_pin(serv_sd, pin_str);
	files_num = get_files_num(serv_sd);
	if(files_num == 0)
	{
		return 1;
	}
	fnames = download_files(serv_sd, files_num);

	for(; fnames->next != NULL; fnames = fnames->next)
	{
		convert2pdf(fnames->name);
	}
	for(; fnames->prev != NULL; fnames = fnames->prev){
//		printf("%s\n", fnames->name);
	}
	print_files(fnames);

//	free_list_from_tail(fnames);
	close(serv_sd);
	return 0;
}

void free_list_from_head(struct str_list *head)
{
	struct str_list *tmp;
	while(head != NULL)
	{
		tmp = head;
		head = head->next;
		free(tmp->name);
		free(tmp->print_count);
		free(tmp);
	}
}

void free_list_from_tail(struct str_list *tail)
{
	struct str_list *tmp;
	while(tail != NULL)
	{
		tmp = tail;
		tail = tail->prev;
		free(tmp->print_count);
		free(tmp);
	}
}

void transfer_pin(int serv_sd, char *pin_str)
{
	write(serv_sd, pin_str, PIN_LEN);
}

int get_files_num(int serv_sd)
{
	int num;
	read(serv_sd, &num, sizeof(num));
	return num;
}

struct str_list* download_files(int serv_sd, int files_num)
{
	struct str_list *fnames;
	struct str_list *prev;

	fnames = malloc(sizeof(struct str_list));
	fnames->next = NULL;
	fnames->prev = NULL;

	uint32_t fsize;
	char *fbuf;
	int fd;
	char fname[FNAME_LEN];
	char fcount[8];

	int iMode = 0;

	sleep(2);

	for(int i = 0; i < files_num;i++)
	{
		read(serv_sd, &fsize, sizeof(fsize));
		printf("%d\n", fsize);
		read(serv_sd, fname, FNAME_LEN);
		fbuf = malloc(fsize);
		read(serv_sd, fbuf, fsize);
		read(serv_sd, fcount, 8);

		fd = open(fname, O_CREAT | O_WRONLY, 0666);
		ioctl(fd, FIONBIO, &iMode);
		write(fd, fbuf, fsize);

		fnames->name = malloc(FNAME_LEN);
		memset(fnames->name, '\0', FNAME_LEN);
		strcpy(fnames->name, fname);

		fnames->print_count = malloc(8);;
		memset(fnames->print_count, '\0', 8);
		strcpy(fnames->print_count, fcount);

		fnames->next = malloc(sizeof(struct str_list));
		prev = fnames;
		fnames = fnames->next;
		fnames->next = NULL;
		fnames->prev = prev;

		free(fbuf);
		close(fd);
	}

	for(; fnames->prev != NULL; fnames = fnames->prev){}

	return fnames;
}

void print_files(struct str_list *fnames)
{
	int pid;

	for(; fnames->next != NULL; fnames = fnames->next)
	{
		pid = fork();

		if(pid == 0)
		{
			execlp("lp", "lp", "-d", PRINTER_NAME, "-n", fnames->print_count, fnames->name, NULL);
			exit(0);
		}

		wait(NULL);

	}
}

void convert2pdf(char *fname)
{
	int convert_pid;
	int flen; 
	char ftype[FNAME_LEN];
	char oldfname[FNAME_LEN];
	int i, lastdot_i;
	int cmpres_pdf, cmpres_txt;
	char pdf_type[] = "pdf";
	char txt_type[] = "txt";

	flen = strlen(fname);
	lastdot_i = 0;
	memset(ftype, '\0', FNAME_LEN);
	memset(oldfname, '\0', FNAME_LEN);

	for(i = 0; i < flen; i++)
	{
		if(fname[i] == '.')
			lastdot_i = i;
	}

	strcpy(ftype, fname+lastdot_i+1);

	cmpres_pdf = strcmp(ftype, pdf_type);
	cmpres_txt = strcmp(ftype, txt_type);

	if((cmpres_pdf != 0) && (cmpres_txt != 0))
	{
		strcpy(oldfname, fname);
		convert_pid = fork();

		if(convert_pid == 0)
		{
			execlp( "soffice", "soffice", "--convert-to", "pdf",
					fname, "--headless", NULL );
			exit(0);
		}

		wait(NULL);
		strcpy(fname+lastdot_i+1, pdf_type);

		int pid = fork();
		if(pid == 0)
		{
			execlp("rm", "rm", oldfname, NULL);
			exit(0);
		}
		wait(NULL);
		printf("Convert done\n");
	}
}
