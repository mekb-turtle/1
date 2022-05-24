#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#define STDIN "-"
#define STDOUT "-"
#define STDERR "!"
int usage(char *argv0) {
	fprintf(stderr, "\
Usage: %s <in files> -o <out files>\n\
	-h --hexdump : output in hex instead\n\
	-? --help    : return this help menu\n\
	-v --verbose : report what it's doing to stderr\n\
	          <in files> : use "STDIN " for stdin (default), or use a file name\n\
	-o --out <out files> : use "STDOUT" for stdout (default), use "STDERR" for stderr, or use a file name\n\
", argv0);
	return 2;
}
int main(int argc, char* argv[]) {
#define INVALID { return usage(argv[0]); }
	bool hexdump_flag = 0;
	bool verbose_flag = 0;
	char* input[argc];
	size_t input_len = 0;
	char* output[argc];
	size_t output_len = 0;
	{
		bool hexdump_done = 0;
		bool verbose_done = 0;
		bool out_flag = 0;
		bool flag_done = 0;
		for (int i = 1; i < argc; ++i) {
			if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
				if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1;
				else {
					if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--out") == 0) {
						if (out_flag) INVALID;
						out_flag = 1;
					}
					else if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) { INVALID; }
					else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--hexdump") == 0) {
						if (hexdump_done) INVALID;
						hexdump_flag = hexdump_done = 1;
					}
					else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
						if (verbose_done) INVALID;
						verbose_flag = verbose_done = 1;
					}
					else INVALID;
				}
			} else {
				if (out_flag) {
					output[output_len++] = argv[i];
				} else {
					input[input_len++] = argv[i];
				}
			}
		}
		if (input_len  == 0) { ++input_len;  input[0]  = "-"; }
		if (output_len == 0) { ++output_len; output[0] = "-"; }
	}
	FILE* input_streams[input_len];
	FILE* output_streams[output_len];
#define ERROR(x) { fprintf(stderr, "%s: %s\n", x, strerror(errno)); return errno; }
	{
		char* input_streams_names[input_len];
		char* output_streams_names[output_len];
		for (size_t i = 0; i < input_len; ++i) {
			if (strcmp(input[i], STDIN) == 0) {
				input_streams[i] = stdin;
				input_streams_names[i] = "stdin";
			} else {
				input_streams[i] = fopen(input[i], "r");
				input_streams_names[i] = input[i];
			}
			if (!input_streams[i]) ERROR(input[i]);
		}
		for (size_t i = 0; i < output_len; ++i) {
			if (strcmp(output[i], STDOUT) == 0) {
				output_streams[i] = stdout;
				output_streams_names[i] = "stdout";
			} else if (strcmp(output[i], STDERR) == 0) {
				output_streams[i] = stderr;
				output_streams_names[i] = "stderr";
			} else {
				output_streams[i] = fopen(output[i], "w");
				output_streams_names[i] = output[i];
			}
			if (!output_streams[i]) ERROR(output[i]);
		}
		if (verbose_flag) {
			if (input_len == 1) {
				fprintf(stderr, "Reading from: %s\n", input_streams_names[0]);
			} else {
				fprintf(stderr, "Reading from:\n");
				for (size_t i = 0; i < input_len; ++i) {
					fprintf(stderr, " %s\n", input_streams_names[i]);
				}
			}
			if (output_len == 1) {
				fprintf(stderr, "Writing to: %s\n", output_streams_names[0]);
			} else {
				fprintf(stderr, "Writing to:\n");
				for (size_t i = 0; i < output_len; ++i) {
					fprintf(stderr, " %s\n", output_streams_names[i]);
				}
			}
			fprintf(stderr, "\n");
		}
	}
	for (size_t i = 0; i < input_len; ++i) {
#define READ_SIZE 4096
		char* data = malloc(READ_SIZE);
		size_t len = 0;
		while ((len = fread(data, 1, READ_SIZE, input_streams[i])) > 0) {
			for (size_t j = 0; j < output_len; ++j) {
				fwrite(data, 1, len, output_streams[j]);
			}
		}
	}
	if (verbose_flag) {
		fprintf(stderr, "Finished\n");
	}
	return 0;
}
