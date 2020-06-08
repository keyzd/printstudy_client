#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PIN_LEN 4

#define SOCKET_ERROR 253
#define WRONG_ADDRESS 254
#define CONNECTION_ERROR 255

void transfer_pin(int serv_sd, char *pin_str);
uint32_t get_scans_paid(int serv_sd);

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		printf("Usage: ./client <address> <port> <PIN>\n");
		return 0;
	}

	char *address = argv[1];
	short port = atoi(argv[2]);
	char *pin_str = argv[3];

	int serv_sd = socket(AF_INET, SOCK_STREAM, 0);
	if(serv_sd == -1)
	{
		return SOCKET_ERROR;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if(!inet_aton(address, &(addr.sin_addr)))
	{
		return WRONG_ADDRESS;
	}

	int connect_flag = connect(serv_sd, (struct sockaddr*)&addr, sizeof(addr));
	if(connect_flag == -1)
	{
		return CONNECTION_ERROR;
	}

	transfer_pin(serv_sd, pin_str);
	uint32_t scans_paid = get_scans_paid(serv_sd);

	return scans_paid;
}

void transfer_pin(int serv_sd, char *pin_str)
{
	write(serv_sd, pin_str, PIN_LEN);
}

uint32_t get_scans_paid(int serv_sd)
{
	uint32_t scans_paid;
	read(serv_sd, &scans_paid, sizeof(scans_paid));
	return scans_paid;
}
