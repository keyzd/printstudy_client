#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Wizard.H>
#include <FL/Fl_Group.H>

#define SUCCESS 0
#define NO_FILES 1

#define SOCKET_ERROR 253
#define WRONG_ADDRESS 254
#define CONNECTION_ERROR 255

#define MODE_PRINT 4
#define MODE_SCAN_LOGIN 8
#define MODE_SCAN_MAIN 16

char* itoa(int n);

enum
{
	button_w = 65,
	button_h = 65,
	buttons_count = 11,
	pin_len = 4,

	spacing = button_w/8,
	font_size = button_w/2,
	box_w = (button_w*3)/pin_len-spacing/3,
	box_h = box_w,
	box_labelsize = box_w/2,
	win_w = (button_w + spacing) * buttons_count/3,
	win_h = (button_h * 5 + spacing*9 + box_h)
};

struct dbutton_data
{
	Fl_Box** box;
	const char* num;

	dbutton_data(Fl_Box** b)
	{
		box = b;
	}
};

struct ebutton_data
{
	Fl_Box** box;
	char *serv_addr, *serv_port;

	ebutton_data(Fl_Box** b, char *serv, char *port)
	{
		box = b;
		serv_addr = serv;
		serv_port = port;
	}
};

struct ebutton2_data
{
	Fl_Box** box;
	Fl_Box* box_counter;
	Fl_Wizard *wizard;
	char *serv_addr, *serv_port;
	uint32_t scans_paid;
	char pin_str[pin_len+1];

	ebutton2_data(Fl_Box** b, char *serv, char *port, Fl_Wizard *wiz, Fl_Box *bc)
	{
		box = b;
		box_counter = bc;
		wizard = wiz;
		serv_addr = serv;
		serv_port = port;
	}
};

struct bmain_data
{
	Fl_Wizard *wizard;
	char *address, *port;
};

static const char *nums[] = 
{
	"1", "2", "3",
	"4", "5", "6",
	"7", "8", "9", "0"
};

int box_i = 0;
void dbutton_callback(Fl_Widget *w, void* user)
{
	dbutton_data* d = (dbutton_data*)user;
	const char* digit = d->num;

	d->box[box_i]->label(digit);
//	d->box[box_i]->label("*");

	box_i++;
	if(box_i%pin_len == 0) box_i = 0;
}

void ebutton_callback(Fl_Widget *w, void *user)
{
	ebutton_data* d = (ebutton_data*)user;
	char pin_str[pin_len+1];

	for(int i = 0; i < pin_len; i++)
	{
		pin_str[i] = (d->box[i]->label())[0];
	}
	pin_str[pin_len] = '\0';

	for(int i = 0; i < pin_len; i++)
	{
		d->box[i]->label("_");
	}
	box_i = 0;

	char cmd[128] = "./cl_print ";
	strcpy(cmd+strlen(cmd), d->serv_addr);
	strcpy(cmd+strlen(cmd), " ");
	strcpy(cmd+strlen(cmd), d->serv_port);
	strcpy(cmd+strlen(cmd), " ");
	strcpy(cmd+strlen(cmd), pin_str);

	int cmd_status = system(cmd);

	if(cmd_status != 0)
	{
		fl_alert("Wrong PIN or no files to print");
	}
}

void ebutton2_callback(Fl_Widget *w, void *user)
{
	ebutton2_data* d = (ebutton2_data*)user;
	char pin_str[pin_len+1];

	for(int i = 0; i < pin_len; i++)
	{
		pin_str[i] = (d->box[i]->label())[0];
	}
	pin_str[pin_len] = '\0';
	strcpy(d->pin_str, pin_str);

	for(int i = 0; i < pin_len; i++)
	{
		d->box[i]->label("_");
	}
	box_i = 0;

	char cmd[128] = "./cl_scan_login ";
	strcpy(cmd+strlen(cmd), d->serv_addr);
	strcpy(cmd+strlen(cmd), " ");
	strcpy(cmd+strlen(cmd), d->serv_port);
	strcpy(cmd+strlen(cmd), " ");
	strcpy(cmd+strlen(cmd), pin_str);

	int status, pid;
	pid = fork();

	if(pid == 0)
	{
		execlp("/bin/sh", "sh", "-c", cmd, NULL);
	}
	else
	{
		wait(&status);
		if(WIFEXITED(status))
		{
			int returned = WEXITSTATUS(status);
			if(returned == CONNECTION_ERROR || returned == WRONG_ADDRESS)
			{
				fl_alert("Wrong PIN or connection error");
			}
			else if(returned <= 0)
			{
				fl_alert("No paid scans");
			}
			else
			{
				d->scans_paid = returned;
				printf("%s\n", itoa(returned));
				d->box_counter->label(itoa(returned));
				d->wizard->next();
			}
		}
		else
		{
			fl_alert("cl_scan_login: bad status");
		}
	}
}

void bscan_callback(Fl_Widget *w, void *user)
{
	ebutton2_data* d = (ebutton2_data*)user;

	if(d->scans_paid >= 1)
	{
		d->scans_paid--;
		d->box_counter->label(itoa(d->scans_paid));

		// Call cl_scan_main
		char cmd[128] = "./cl_scan_main ";
		strcpy(cmd+strlen(cmd), d->serv_addr);
		cmd[strlen(cmd)] = ' ';
		strcpy(cmd+strlen(cmd), d->serv_port);
		cmd[strlen(cmd)] = ' ';
		strcpy(cmd+strlen(cmd), d->pin_str);
		cmd[strlen(cmd)] = ' ';
		strcpy(cmd+strlen(cmd), itoa(d->scans_paid));

		system(cmd);
		wait(NULL);

	}
	else fl_alert("No paid scans");
}

int connect_to_serv(char* address, char* port)
{
	int serv_sd = socket(AF_INET, SOCK_STREAM, 0);
	assert(serv_sd != -1);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));

	if(!inet_aton(address, &(addr.sin_addr)))
	{
		printf("Wrong server address\n");
		exit(2);
	}

	int connect_flag = connect(serv_sd, (struct sockaddr*)&addr, sizeof(addr));
	assert(connect_flag != -1);

	return serv_sd;
}

void bmain_print_callback(Fl_Widget *w, void* user)
{
	int mode = MODE_PRINT;

	bmain_data* d = (bmain_data*)user;
	int serv_sd = connect_to_serv(d->address, d->port);

	write(serv_sd, &mode, sizeof(mode));

	d->wizard->next();
}

void bmain_scan_callback(Fl_Widget *w, void* user)
{
	int mode = MODE_SCAN_LOGIN;

	bmain_data* d = (bmain_data*)user;
	int serv_sd = connect_to_serv(d->address, d->port);

	write(serv_sd, &mode, sizeof(mode));

	d->wizard->next();
	d->wizard->next();
}

void bcancel_callback(Fl_Widget *w, void* user)
{
	for(int i = 0; i < 3; i++) ((Fl_Wizard*)user)->prev();
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("Usage: ./client <address> <port>\n");
		exit(1);
	}

	Fl_Window *win = new Fl_Window(win_w, win_h, "PrintStudy Terminal");
	Fl_Wizard *wiz = new Fl_Wizard(0, 0, win_w, win_h, NULL);

// MAIN WINDOW
	Fl_Group *group_main = new Fl_Group(0, 0, win_w, win_h, NULL);
	group_main->begin();
	Fl_Button *bmain_print, *bmain_scan;
	bmain_scan = new Fl_Button(spacing, win_h/2+spacing, win_w-spacing*2, button_h, "Scanning");
	bmain_print = new Fl_Button(spacing, win_h/2-button_h-spacing, win_w-spacing*2, button_h, "Printing");
	bmain_print->labelsize(font_size-0);
	bmain_scan->labelsize(font_size-0);

	bmain_data bmaind;
	bmaind.wizard = wiz;
	bmaind.address = argv[1];
	bmaind.port = argv[2];

	bmain_print->callback(bmain_print_callback, (void*)&bmaind);
	bmain_scan->callback(bmain_scan_callback, (void*)&bmaind);
	group_main->end();

// PRINTING WINDOW
	Fl_Group *group_print = new Fl_Group(0, 0, win_w, win_h, NULL);
	group_print->begin();

	Fl_Button* b[buttons_count];
	Fl_Box** box = new Fl_Box*[pin_len];
	Fl_Button* bcancel;

	int x = win_w/2 - (button_w + button_w/2 + spacing*1);
	int y = spacing*2;

	for(int i = 0; i < buttons_count-1; i++)
	{
		b[i] = new Fl_Button(x, y, button_w, button_h, nums[i]);
		b[i]->labelsize(font_size);
		x += button_w + spacing;

		if((i+1) % 3 == 0)
		{
			x = win_w/2 - (button_w + button_w/2 + spacing*1);
			y += button_h + spacing;
		}
	}

	b[buttons_count-1] = new Fl_Button(x, y, button_w*2+spacing, button_h, "Enter");
	b[buttons_count-1]->labelsize(font_size);
	bcancel = new Fl_Button(button_w/2, y+button_h*2, button_w*3, button_h, "Cancel");
	bcancel->labelsize(font_size);

	x = win_w/2 - (button_w + button_w/2 + spacing*1);
	y += button_h+spacing;

	for(int i = 0; i < pin_len; i++)
	{
		box[i] = new Fl_Box(x, y, box_w, box_h, "_");
		box[i]->box(FL_BORDER_BOX);
		box[i]->color(FL_LIGHT2);
		box[i]->labelsize(font_size);
		x += box_w + spacing;
	}

	group_print->end();

// SCANNING WINDOW LOGIN

	Fl_Group *group_scan_login = new Fl_Group(0, 0, win_w, win_h, NULL);
	group_scan_login->begin();

	Fl_Button* b2[buttons_count];
	Fl_Box** box2 = new Fl_Box*[pin_len];
	Fl_Button* bcancel2;

	x = win_w/2 - (button_w + button_w/2 + spacing*1);
	y = spacing*2;

	for(int i = 0; i < buttons_count-1; i++)
	{
		b2[i] = new Fl_Button(x, y, button_w, button_h, nums[i]);
		b2[i]->labelsize(font_size);
		x += button_w + spacing;

		if((i+1) % 3 == 0)
		{
			x = win_w/2 - (button_w + button_w/2 + spacing*1);
			y += button_h + spacing;
		}
	}

	b2[buttons_count-1] = new Fl_Button(x, y, button_w*2+spacing, button_h, "Enter");
	b2[buttons_count-1]->labelsize(font_size);
	bcancel2 = new Fl_Button(button_w/2, y+button_h*2, button_w*3, button_h, "Cancel");
	bcancel2->labelsize(font_size);

	x = win_w/2 - (button_w + button_w/2 + spacing*1);
	y += button_h+spacing;

	for(int i = 0; i < pin_len; i++)
	{
		box2[i] = new Fl_Box(x, y, box_w, box_h, "_");
		box2[i]->box(FL_BORDER_BOX);
		box2[i]->color(FL_LIGHT2);
		box2[i]->labelsize(font_size);
		x += box_w + spacing;
	}


	for(int i = 0; i < buttons_count-1; i++)
	{
		dbutton_data *d = new dbutton_data(box);
		d->box = box;
		d->num = nums[i];
		b[i]->callback(dbutton_callback, (void*)d);

		dbutton_data *d2 = new dbutton_data(box);
		d2->box = box2;
		d2->num = nums[i];
		b2[i]->callback(dbutton_callback, (void*)d2);
	}

	ebutton_data *d = new ebutton_data(box, argv[1], argv[2]);
	b[buttons_count-1]->callback(ebutton_callback, (void*)d);
	bcancel->callback(bcancel_callback, (void*)wiz);

	int tx = win_w/2 - button_w;
	int ty = spacing*7 + button_h;

	Fl_Box* box_counter = new Fl_Box(tx, ty, button_w*2, button_h, NULL);

	ebutton2_data *d2 = new ebutton2_data(box2, argv[1], argv[2], wiz, box_counter);
	b2[buttons_count-1]->callback(ebutton2_callback, (void*)d2);
	bcancel2->callback(bcancel_callback, (void*)wiz);

	group_scan_login->remove(box_counter);
	group_scan_login->end();

// SCANNING WINDOW MAIN
	Fl_Group *group_scan_main = new Fl_Group(0, 0, win_w, win_h, NULL);
	group_scan_main->begin();

	x = win_w/2 - button_w;
	y = spacing*6;

	Fl_Box* box_scan_left = new Fl_Box(x, y, button_w*2, button_h, "Scans left:");
	box_scan_left->labelsize(font_size);
	y += button_h + spacing;

	box_counter->labelsize(font_size);
	box_counter->box(FL_BORDER_BOX);
	box_counter->color(FL_LIGHT2);
	group_scan_main->add(box_counter);
	y += button_h + spacing;

	Fl_Button* bscan = new Fl_Button(x, y, button_w*2, button_h, "Scan");
	bscan->labelsize(font_size);
	bscan->callback(bscan_callback, (void*)d2);
	y += button_h + spacing;

	Fl_Button* bcancel3 = new Fl_Button(x, y, button_w*2, button_h, "Cancel");
	bcancel3->labelsize(font_size);
	bcancel3->callback(bcancel_callback, (void*)wiz);

	group_scan_main->end();

	win->show();
	return Fl::run();
}

char* itoa(int n)
{
	int len = snprintf(NULL, 0, "%u", n);
	char *n_str = (char*)malloc(len+1);
	snprintf(n_str, len+1, "%u", n);
	return n_str;
}
