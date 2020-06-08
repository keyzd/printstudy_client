#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PIN_LEN 4
#define SCAN_NAME_LEN 19

struct scan_t
{
	char *name;
	uint32_t size;
	char* buf;
};

int make_connection(char* address, char* port_str);
void make_scan(struct scan_t* scan);
char* make_scan_name();
void make_scan_buf(struct scan_t* scan);
void send_data(int serv_sd, char* pin_str, char* scan_paid, struct scan_t* scan);
void send_scan(int serv_sd, struct scan_t* scan);

int main(int argc, char *argv[])
{
	if(argc < 5)
	{
		printf("Usage: ./cl_scan_main <address> <port> <PIN> <scans_paid>\n");
		return 0;
	}

	char* address = argv[1];
	char* port_str = argv[2];
	char* pin_str = argv[3];
	char* scans_paid_str = argv[4];

	int serv_sd = make_connection(address, port_str);

	struct scan_t scan;

	make_scan(&scan);
	send_data(serv_sd, pin_str, scans_paid_str, &scan);

	free(scan.name);
	free(scan.buf);
	return 0;
}

int make_connection(char* address, char* port_str)
{
	int serv_sd = socket(AF_INET, SOCK_STREAM, 0);
	assert(serv_sd != -1);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port_str));

	if(!inet_aton(address, &(addr.sin_addr)))
	{
		printf("Wrong server address\n");
		exit(1);
	}

	int connect_flag = connect(serv_sd, (struct sockaddr*)&addr, sizeof(addr));
	assert(connect_flag != -1);

	return serv_sd;
}

void make_scan(struct scan_t* scan)
{
	int pid;
	char* scan_name = make_scan_name();

	scan->name = scan_name;
	system("scanimage --format png > scan.png");

	pid = fork();
	if(pid == 0)
	{
		execlp("mv", "mv", "scan.png", scan_name, NULL); 
		exit(0);
	}
	wait(NULL);

	make_scan_buf(scan);
}

char* make_scan_name()
{
	char *scan_name = malloc(SCAN_NAME_LEN);
	time_t rawtime = time(NULL);
	struct tm* t = localtime(&rawtime);
	strftime(scan_name, SCAN_NAME_LEN, "PS-%d%m%y-%H%M.png", t);
	return scan_name;
}

void make_scan_buf(struct scan_t* scan)
{
	FILE *f = fopen(scan->name, "r");
	char *scan_buf;
	int c;
	uint32_t i = 0;

	while( (c = fgetc(f)) != EOF ) i++;
	scan->size = i;
	scan_buf = malloc(i);
	rewind(f);

	i = 0;
	while( (c = fgetc(f)) != EOF )
	{
		scan_buf[i] = c;
		i++;
	}

	scan->buf = scan_buf;
}

void send_scan(int serv_sd, struct scan_t* scan)
{
	write(serv_sd, scan->name, SCAN_NAME_LEN);
	write(serv_sd, &(scan->size), sizeof(scan->size));
}

void send_data(int serv_sd, char* pin_str, char* scans_paid_str, struct scan_t* scan)
{
	uint32_t scans_paid = atoi(scans_paid_str);
	send_scan(serv_sd, scan);
	write(serv_sd, pin_str, PIN_LEN);
	write(serv_sd, &scans_paid, sizeof(scans_paid));

	write(serv_sd, scan->buf, scan->size);
}

