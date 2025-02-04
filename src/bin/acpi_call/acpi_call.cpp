/*-
 *   Copyright (C) 2011 by Maxim Ignatenko
 *   gelraen.ua@gmail.com
 *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *    modification, are permitted provided that the following conditions   *
 *    are met:                                                             *
 *     * Redistributions of source code must retain the above copyright    *
 *       notice, this list of conditions and the following disclaimer.     *
 *     * Redistributions in binary form must reproduce the above copyright *
 *       notice, this list of conditions and the following disclaimer in   *
 *       the documentation and/or other materials provided with the        *
 *       distribution.                                                     *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR *
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, *
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      *
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, *
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY *
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   *
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE *
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  *
 *
 */

//#include "acpi_call_io.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SupportDefs.h>

#include <uacpi/acpi.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>


struct acpi_call_descr
{
	char*		path;
	uacpi_object_array	args;
	uacpi_status	retval;
	uacpi_data_view	result;
	size_t	reslen;
};


#define	MAX_ACPI_PATH	1024 // XXX
#define MAX_ACPI_ARGS	7

char dev_path[MAXPATHLEN] = "/dev/acpi/call";
char method_path[MAX_ACPI_PATH] = "";
size_t result_buf_size = 1024;
char output_format = 'o';

int verbose;

uacpi_object* args[MAX_ACPI_ARGS];
struct acpi_call_descr params;

void parse_opts(int, char *[]);
void show_help(FILE*);
int parse_buffer(uacpi_object**, char*);
void print_params(struct acpi_call_descr*);
void print_acpi_object(uacpi_object*);
void print_acpi_buffer(uacpi_data_view*, char);

int main(int argc, char * argv[])
{
	int fd;

	bzero(&params, sizeof(params));
	params.path = method_path;
	params.args.count = 0;
	params.args.objects = args;

	verbose = 0;

	parse_opts(argc, argv);

	params.result.length = result_buf_size;
	params.result.data = malloc(result_buf_size);

	if (params.result.data == NULL)
	{
		perror("malloc");
		return 1;
	}

	if (method_path[0] == 0)
	{
		fprintf(stderr, "Please specify path to method with -p flag\n");
		return 1;
	}

	if (verbose)
		print_params(&params);

	fd = open(dev_path, O_RDWR);
	if (fd < 0)
	{
		perror("open");
		return 1;
	}
	if (ioctl(fd, 'ACCA', &params) == -1)
	{
		perror("ioctl");
		return 1;
	}

	if (verbose)
		printf("Status: %d\nResult: ", params.retval);
	print_acpi_buffer(&params.result, output_format);
	printf("\n");

	return params.retval;
}

void parse_opts(int argc, char * argv[])
{
	char c;
	int i;

	while ((c = getopt(argc, argv, "hvd:p:i:s:b:o:")) != -1)
	{
		switch(c)
		{
		case 'h':
			show_help(stdout);
			exit(0);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'd':
			strlcpy(dev_path, optarg, MAXPATHLEN);
			break;
		case 'p':
			strlcpy(method_path, optarg, MAX_ACPI_PATH);
			break;
		case 'i':
		case 's':
		case 'b':
			i = params.args.count;
			if (i >= MAX_ACPI_ARGS)
			{
				fprintf(stderr, "Maximum number of arguments exceeded\n");
				exit(1);
			}
			switch (optopt)
			{
			case 'i':
				args[i] = uacpi_object_create_integer(strtol(optarg, NULL, 10));
				break;
			case 's':
				args[i] = uacpi_object_create_cstring(optarg);
				break;
			case 'b':
				if (parse_buffer(&args[i], optarg))
				{
					fprintf(stderr, "Unable to parse hexstring to buffer: %s\n", optarg);
					exit(1);
				}
				break;
			}
			params.args.count++;
			break;
		case 'o':
			output_format = optarg[0];
			switch (optarg[0])
			{
			case 'i':
			case 's':
			case 'b':
			case 'o':
				break;
			default:
				fprintf(stderr, "Incorrect output format: %c\n", optarg[0]);
				show_help(stderr);
				exit(1);
			}
			break;
		default:
			show_help(stderr);
			exit(1);
		}
	}
}

void show_help(FILE* f)
{
	fprintf(f, "Options:\n");
	fprintf(f, "  -h              - print this help\n");
	fprintf(f, "  -v              - be verbose\n");
	fprintf(f, "  -d filename     - specify path to ACPI control pseudo-device. Default: /dev/acpi/call\n");
	fprintf(f, "  -p path         - full path to ACPI method\n");
	fprintf(f, "  -i number       - add integer argument\n");
	fprintf(f, "  -s string       - add string argument\n");
	fprintf(f, "  -b hexstring    - add buffer argument\n");
	fprintf(f, "  -o i|s|b|o      - print result as integer|string|hexstring|object\n");
}

int parse_buffer(uacpi_object **dst, char *src)
{
	char tmp[3] = {0};
	size_t len = strlen(src)/2, i;

	uacpi_data_view dataview;
	dataview.length = len;
	if ((dataview.data = (uint8_t*)malloc(len)) == NULL)
	{
		fprintf(stderr, "parse_buffer: Failed to allocate %" B_PRIuSIZE " bytes\n", len);
		exit(1);
	}

	for(i = 0; i < len; i++)
	{
		tmp[0] = src[i*2];
		tmp[1] = src[i*2+1];
		dataview.bytes[i] = strtol(tmp, NULL, 16);
	}

	*dst = uacpi_object_create_buffer(dataview);
	return 0;
}

void print_params(struct acpi_call_descr* p)
{
	printf("Path: %s\n", p->path);
	printf("Number of arguments: %ld\n", p->args.count);
	for(uint32 i = 0; i < p->args.count; i++)
	{
		if (uacpi_object_is(p->args.objects[i], UACPI_OBJECT_INTEGER))
			printf("Argument %d type: Integer\n", i+1);
		else if (uacpi_object_is(p->args.objects[i], UACPI_OBJECT_STRING))
			printf("Argument %d type: String\n", i+1);
		else if (uacpi_object_is(p->args.objects[i], UACPI_OBJECT_BUFFER))
			printf("Argument %d type: Buffer\n", i+1);

		printf("Argument %d value: ", i+1);
		print_acpi_object((p->args.objects[i]));
		printf("\n");
	}
}

void print_acpi_object(uacpi_object* obj)
{
	if (uacpi_object_is(obj, UACPI_OBJECT_INTEGER)) {
		uacpi_u64 value;
		uacpi_object_get_integer(obj, &value);
		printf("%" B_PRIu64, value);
	} else if (uacpi_object_is(obj, UACPI_OBJECT_STRING)) {
		uacpi_data_view view;
		uacpi_object_get_string(obj, &view);
		printf("%s", view.text);
	} else if (uacpi_object_is(obj, UACPI_OBJECT_BUFFER)) {
		uacpi_data_view view;
		uacpi_object_get_buffer(obj, &view);
		for(uint32 i = 0; i < view.length; i++)
		{
			printf("%02X", view.bytes[i]);
		}
	} else {
		printf("Unknown object type '%d'", uacpi_object_get_type(obj));
	}
}

void print_acpi_buffer(uacpi_data_view* buf, char format)
{
	switch (format)
	{
	case 'i':
		printf("%" B_PRIu64, *((uint64_t*)(buf->data)));
		break;
	case 's':
		printf("%s", (char*)buf->text);
		break;
	case 'b':
		for(uint32 i = 0; i < buf->length; i++)
		{
			printf("%02X", ((uint8_t*)(buf->data))[i]);
		}
		break;
	case 'o':
		print_acpi_object((uacpi_object*)(buf->data));
		break;
	}
}
