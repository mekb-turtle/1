#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#define STDIN "-"
#define STDOUT "-"
#define STDERR "!"
int usage(char *argv0) {
	fprintf(stderr, "\
Usage: %s <in files> -o <out files>\n\n\
	-? --help             : return this help menu\n\
	-v --verbose          : report what it's doing to stderr\n\
	<in files>            : use "STDIN" for stdin (default), or use a file name\n\
	-o --out <out files>  : use "STDOUT" for stdout (default), use "STDERR" for stderr, or use a file name\n\
	-h --hexdump          : output in hex instead\n\n\
	-b --block <bytes>    : block size, how many bytes to read at once, see below for what to put for size, 4096 is default\n\
	-c --count <blocks>   : count blocks, amount of blocks to read, this means total blocks not blocks for each input file, 0 means whole file (default)\n\
	-s --skip <blocks>    : skip first n blocks in input, 0 is default\n\
	-S --seek <blocks>    : skip first n blocks in output, 0 is default\n\n\
	the last 4 arguments must be followed a number, and optionally suffixed by one of these which will multiply it\n\
		K, KiB, KIB, kib: 1024\n\
		M, MiB, MIB, mib: 1024^2, 1048576\n\
		G, GiB, GIB, gib: 1024^3, 1073741824\n\
		T, TiB, TIB, tib: 1024^4, 1099511627776\n\
		P, PiB, PIB, pib: 1024^5, 1125899906842624\n\
		E, EiB, EIB, eib: 1024^6, 1152921504606846976\n\
		KB, kb, k:        1000\n\
		MB, mb, m:        1000^2, 1000000\n\
		GB, gb, g:        1000^3, 1000000000\n\
		TB, tb, t:        1000^4, 1000000000000\n\
		PB, pb, p:        1000^5, 1000000000000000\n\
		EB, eb, e:        1000^6, 1000000000000000000\n\
	e.g 123M = 123*1024*1024 = 123*1048576 = 128974848\n\n\
", argv0);
	return 2;
}
char* preview_bytes(uint64_t bytes) {
	uint64_t e = 0;
	uint64_t p = 0;
	uint64_t t = 0;
	uint64_t g = 0;
	uint64_t m = 0;
	uint64_t k = 0;
	printf("%li\n", bytes);
	while (bytes >= (uint64_t) 1<<60) { ++e; bytes -= (uint64_t) 1<<60; }
	while (bytes >= (uint64_t) 1<<50) { ++p; bytes -= (uint64_t) 1<<50; }
	while (bytes >= (uint64_t) 1<<40) { ++t; bytes -= (uint64_t) 1<<40; }
	while (bytes >= (uint64_t) 1<<30) { ++g; bytes -= (uint64_t) 1<<30; }
	while (bytes >= (uint64_t) 1<<20) { ++m; bytes -= (uint64_t) 1<<20; }
	while (bytes >= (uint64_t) 1<<10) { ++k; bytes -= (uint64_t) 1<<10; }
	char* res = malloc(64);
	res[0] = 0;
	printf("%li\n", e);
	printf("%li\n", p);
	printf("%li\n", t);
	printf("%li\n", g);
	printf("%li\n", m);
	printf("%li\n", k);
	printf("%li\n", bytes);
	if (e     > 0) { char* tmp = malloc(64); sprintf(res, "%liE %s", e,     tmp); free(res); res = tmp; }
	if (p     > 0) { char* tmp = malloc(64); sprintf(res, "%liP %s", p,     tmp); free(res); res = tmp; }
	if (t     > 0) { char* tmp = malloc(64); sprintf(res, "%liT %s", t,     tmp); free(res); res = tmp; }
	if (g     > 0) { char* tmp = malloc(64); sprintf(res, "%liG %s", g,     tmp); free(res); res = tmp; }
	if (m     > 0) { char* tmp = malloc(64); sprintf(res, "%liM %s", m,     tmp); free(res); res = tmp; }
	if (k     > 0) { char* tmp = malloc(64); sprintf(res, "%liK %s", k,     tmp); free(res); res = tmp; }
	if (bytes > 0) { char* tmp = malloc(64); sprintf(res, "%li %s",  bytes, tmp); free(res); res = tmp; }
	res[strlen(res) - 1] = 0;
	return res;
}
bool parse_bytes(bool start, uint64_t* res, char* str) {
	size_t len = strlen(str);
	bool number_yet = 0;
	bool suffix_yet = 0;
	char* number = malloc(len+1);
	char* suffix = malloc(len+1);
	for (size_t i = 0; i < len+1; ++i) number[i] = suffix[i] = 0;
	size_t number_len = 0;
	size_t suffix_len = 0;
#define NUMERIC(x) (x>='0'&&x<='9')
#define ALPHABETIC(x) ((x>='a'&&x<='z')||(x>='A'&&x<='Z'))
	for (size_t i = 0; i < len; ++i) {
		if (NUMERIC(str[i])) {
			if (suffix_yet) return 0;
			if (number_len >= 10) return 0;
			number_yet = 1;
			number[number_len++] = str[i];
		} else if (ALPHABETIC(str[i])) {
			if (!number_yet) return 0;
			if (suffix_len >= 3) return 0;
			suffix_yet = 1;
			suffix[suffix_len++] = str[i];
		} else return 0;
	}
	uint64_t num = atoll(number);
	//TODO: fix this code
	if      (strcmp(suffix, "K")  == 0 || strcmp(suffix, "KiB") == 0 || strcmp(suffix, "KIB") == 0 || strcmp(suffix, "kib") == 0) num *= (uint64_t) 1<<10;
	else if (strcmp(suffix, "M")  == 0 || strcmp(suffix, "MiB") == 0 || strcmp(suffix, "MIB") == 0 || strcmp(suffix, "mib") == 0) num *= (uint64_t) 1<<20;
	else if (strcmp(suffix, "G")  == 0 || strcmp(suffix, "GiB") == 0 || strcmp(suffix, "GIB") == 0 || strcmp(suffix, "gib") == 0) num *= (uint64_t) 1<<30;
	else if (strcmp(suffix, "T")  == 0 || strcmp(suffix, "TiB") == 0 || strcmp(suffix, "TIB") == 0 || strcmp(suffix, "tib") == 0) num *= (uint64_t) 1<<40;
	else if (strcmp(suffix, "P")  == 0 || strcmp(suffix, "PiB") == 0 || strcmp(suffix, "PIB") == 0 || strcmp(suffix, "pib") == 0) num *= (uint64_t) 1<<50;
	else if (strcmp(suffix, "E")  == 0 || strcmp(suffix, "EiB") == 0 || strcmp(suffix, "EIB") == 0 || strcmp(suffix, "eib") == 0) num *= (uint64_t) 1<<60;
	else if (strcmp(suffix, "KB") == 0 || strcmp(suffix, "kb")  == 0 || strcmp(suffix, "k")   == 0) num *= 1e3;
	else if (strcmp(suffix, "MB") == 0 || strcmp(suffix, "mb")  == 0 || strcmp(suffix, "m")   == 0) num *= 1e6;
	else if (strcmp(suffix, "GB") == 0 || strcmp(suffix, "gb")  == 0 || strcmp(suffix, "g")   == 0) num *= 1e9;
	else if (strcmp(suffix, "TB") == 0 || strcmp(suffix, "tb")  == 0 || strcmp(suffix, "t")   == 0) num *= 1e12;
	else if (strcmp(suffix, "PB") == 0 || strcmp(suffix, "pb")  == 0 || strcmp(suffix, "p")   == 0) num *= 1e15;
	else if (strcmp(suffix, "EB") == 0 || strcmp(suffix, "eb")  == 0 || strcmp(suffix, "e")   == 0) num *= 1e18;
	else return 0;
	*res = start;
	return 1;
}
int main(int argc, char* argv[]) {
#define INVALID { return usage(argv[0]); }
	bool hexdump_flag = 0;
	bool verbose_flag = 0;
	char* input[argc];
	size_t input_len = 0;
	char* output[argc];
	size_t output_len = 0;
	uint64_t block = 4096;
	uint64_t count = 0;
	uint64_t skip = 0;
	uint64_t seek = 0;
	{
		bool hexdump_done = 0;
		bool verbose_done = 0;
		bool out_flag = 0;
		bool flag_done = 0;
		bool block_flag = 0;
		bool count_flag = 0;
		bool skip_flag = 0;
		bool seek_flag = 0;
		bool block_done = 0;
		bool count_done = 0;
		bool skip_done = 0;
		bool seek_done = 0;
		for (int i = 1; i < argc; ++i) {
			if (block_flag) {
				block_flag = 0;
				if (!parse_bytes(0, &block, argv[i])) INVALID
			} else if (count_flag) {
				count_flag = 0;
				if (!parse_bytes(0, &count, argv[i])) INVALID
			} else if (skip_flag) {
				skip_flag = 0;
				if (!parse_bytes(1, &skip, argv[i])) INVALID
			} else if (seek_flag) {
				seek_flag = 0;
				if (!parse_bytes(1, &seek, argv[i])) INVALID
			} else if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
				if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1;
				else {
					if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--out") == 0) {
						if (out_flag) INVALID
						out_flag = 1;
					} else if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) { INVALID
					} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--hexdump") == 0) {
						if (hexdump_done) INVALID
						hexdump_flag = hexdump_done = 1;
					} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
						if (verbose_done) INVALID
						verbose_flag = verbose_done = 1;
					} else if (i >= argc - 1) { INVALID
					} else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--block") == 0) {
						if (block_done) INVALID
						block_flag = block_done = 1;
					} else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0) {
						if (count_done) INVALID
						count_flag = count_done = 1;
					} else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--skip") == 0) {
						if (skip_done) INVALID
						skip_flag = skip_done = 1;
					} else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--seek") == 0) {
						if (seek_done) INVALID
						seek_flag = seek_done = 1;
					}
					else INVALID
				}
			} else if (out_flag) {
				output[output_len++] = argv[i];
			} else {
				input[input_len++] = argv[i];
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
			fprintf(stderr, "Block Size: %s\n", preview_bytes(block));
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
