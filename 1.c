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
	-c --count <blocks>   : count blocks, amount of blocks to read, this means total blocks not blocks for each input file, 0 means whole file (default)\n"
/*	-s --skip <blocks>    : skip first n blocks in input, 0 is default\n\
	-S --seek <blocks>    : skip first n blocks in output, 0 is default\n\n*/"\
	the last 2 flags must be followed a number, and optionally suffixed by one of these which will multiply it\n\
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
char* display_bytes(uint64_t bytes) {
	if (bytes == 0) return "0";
	uint64_t e = 0;
	uint64_t p = 0;
	uint64_t t = 0;
	uint64_t g = 0;
	uint64_t m = 0;
	uint64_t k = 0;
	while (bytes >= (uint64_t) 1<<60) { ++e; bytes -= (uint64_t) 1<<60; } // subtracts 1 exbi until 0, adds 1 to e each time
	while (bytes >= (uint64_t) 1<<50) { ++p; bytes -= (uint64_t) 1<<50; } // subtracts 1 pebi until 0, adds 1 to p each time
	while (bytes >= (uint64_t) 1<<40) { ++t; bytes -= (uint64_t) 1<<40; } // subtracts 1 tebi until 0, adds 1 to t each time
	while (bytes >= (uint64_t) 1<<30) { ++g; bytes -= (uint64_t) 1<<30; } // subtracts 1 gibi until 0, adds 1 to g each time
	while (bytes >= (uint64_t) 1<<20) { ++m; bytes -= (uint64_t) 1<<20; } // subtracts 1 mebi until 0, adds 1 to m each time
	while (bytes >= (uint64_t) 1<<10) { ++k; bytes -= (uint64_t) 1<<10; } // subtracts 1 kibi until 0, adds 1 to k each time
	char* res = malloc(64);
	res[0] = 0;
	// horrible code, i'm sorry
	if (bytes > 0) { char* tmp = malloc(64); sprintf(tmp, "%li %s",  bytes, res); free(res); res = tmp; } // adds bytes to start of string
	if (k     > 0) { char* tmp = malloc(64); sprintf(tmp, "%liK %s", k,     res); free(res); res = tmp; } // adds k to start of string
	if (m     > 0) { char* tmp = malloc(64); sprintf(tmp, "%liM %s", m,     res); free(res); res = tmp; } // adds m to start of string
	if (g     > 0) { char* tmp = malloc(64); sprintf(tmp, "%liG %s", g,     res); free(res); res = tmp; } // adds g to start of string
	if (t     > 0) { char* tmp = malloc(64); sprintf(tmp, "%liT %s", t,     res); free(res); res = tmp; } // adds t to start of string
	if (p     > 0) { char* tmp = malloc(64); sprintf(tmp, "%liP %s", p,     res); free(res); res = tmp; } // adds p to start of string
	if (e     > 0) { char* tmp = malloc(64); sprintf(tmp, "%liE %s", e,     res); free(res); res = tmp; } // adds e to start of string
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
			// make sure there are no letters beforehand
			if (suffix_yet) return 0;
			// make sure length of numbers is less than or equal to 10
			if (number_len >= 10) return 0;
			number_yet = 1;
			// set the char 
			number[number_len++] = str[i];
		} else if (ALPHABETIC(str[i])) {
			// make sure there has been numbers beforehand
			if (!number_yet) return 0;
			// make sure length of suffix is less than or equal to 3
			if (suffix_len >= 3) return 0;
			suffix_yet = 1;
			// set the char
			suffix[suffix_len++] = str[i];
		} else return 0;
	}
	uint64_t num = atoll(number); // convert the number
	if (start == 1 && num == 0) return 0; // don't allow 0 if start is 1
	if (suffix_len == 0); // allow no suffix
	else if ((suffix_len == 1 && strncmp(suffix, "K", 1) == 0) || (suffix_len == 3 && (strncmp(suffix, "KiB", 3) == 0 || strncmp(suffix, "KIB", 3) == 0 || strncmp(suffix, "kib", 3) == 0))) num *= (uint64_t) 1<<10; // K, KiB, KIB, kib
	else if ((suffix_len == 1 && strncmp(suffix, "M", 1) == 0) || (suffix_len == 3 && (strncmp(suffix, "MiB", 3) == 0 || strncmp(suffix, "MIB", 3) == 0 || strncmp(suffix, "mib", 3) == 0))) num *= (uint64_t) 1<<20; // M, MiB, MIB, mib
	else if ((suffix_len == 1 && strncmp(suffix, "G", 1) == 0) || (suffix_len == 3 && (strncmp(suffix, "GiB", 3) == 0 || strncmp(suffix, "GIB", 3) == 0 || strncmp(suffix, "gib", 3) == 0))) num *= (uint64_t) 1<<30; // G, GiB, GIB, gib
	else if ((suffix_len == 1 && strncmp(suffix, "T", 1) == 0) || (suffix_len == 3 && (strncmp(suffix, "TiB", 3) == 0 || strncmp(suffix, "TIB", 3) == 0 || strncmp(suffix, "tib", 3) == 0))) num *= (uint64_t) 1<<40; // T, TiB, TIB, tib
	else if ((suffix_len == 1 && strncmp(suffix, "P", 1) == 0) || (suffix_len == 3 && (strncmp(suffix, "PiB", 3) == 0 || strncmp(suffix, "PIB", 3) == 0 || strncmp(suffix, "pib", 3) == 0))) num *= (uint64_t) 1<<50; // P, PiB, PIB, pib
	else if ((suffix_len == 1 && strncmp(suffix, "E", 1) == 0) || (suffix_len == 3 && (strncmp(suffix, "EiB", 3) == 0 || strncmp(suffix, "EIB", 3) == 0 || strncmp(suffix, "eib", 3) == 0))) num *= (uint64_t) 1<<60; // E, EiB, EIB, eib
	else if ((suffix_len == 2 && (strncmp(suffix, "KB", 2) == 0 || strncmp(suffix, "kb", 2) == 0)) || (suffix_len == 1 && strncmp(suffix, "k", 1) == 0)) num *= 1e3;  // KB, kb, k
	else if ((suffix_len == 2 && (strncmp(suffix, "MB", 2) == 0 || strncmp(suffix, "mb", 2) == 0)) || (suffix_len == 1 && strncmp(suffix, "m", 1) == 0)) num *= 1e6;  // MB, mb, m
	else if ((suffix_len == 2 && (strncmp(suffix, "GB", 2) == 0 || strncmp(suffix, "gb", 2) == 0)) || (suffix_len == 1 && strncmp(suffix, "g", 1) == 0)) num *= 1e9;  // GB, gb, g
	else if ((suffix_len == 2 && (strncmp(suffix, "TB", 2) == 0 || strncmp(suffix, "tb", 2) == 0)) || (suffix_len == 1 && strncmp(suffix, "t", 1) == 0)) num *= 1e12; // TB, tb, t
	else if ((suffix_len == 2 && (strncmp(suffix, "PB", 2) == 0 || strncmp(suffix, "pb", 2) == 0)) || (suffix_len == 1 && strncmp(suffix, "p", 1) == 0)) num *= 1e15; // PB, pb, p
	else if ((suffix_len == 2 && (strncmp(suffix, "EB", 2) == 0 || strncmp(suffix, "eb", 2) == 0)) || (suffix_len == 1 && strncmp(suffix, "e", 1) == 0)) num *= 1e18; // EB, eb, e
	else return 0;
	*res = num;
	return 1;
}
int main(int argc, char* argv[]) {
#define INVALID { return usage(argv[0]); }
	bool hexdump = 0;
	bool verbose = 0;
	char* input[argc];
	size_t input_len = 0;
	char* output[argc];
	size_t output_len = 0;
	// defaults
	uint64_t block = 4096;
	uint64_t count = 0;
//	uint64_t skip = 0;
//	uint64_t seek = 0;
	{
		// command handling
		bool hexdump_done = 0;
		bool verbose_done = 0;
		bool out_flag = 0;
		bool flag_done = 0;
		bool block_flag = 0;
		bool count_flag = 0;
//		bool skip_flag = 0;
//		bool seek_flag = 0;
		bool block_done = 0;
		bool count_done = 0;
//		bool skip_done = 0;
//		bool seek_done = 0;
		for (int i = 1; i < argc; ++i) {
			if (block_flag) {
				block_flag = 0;
				if (!parse_bytes(1, &block, argv[i])) INVALID
			} else if (count_flag) {
				count_flag = 0;
				if (!parse_bytes(0, &count, argv[i])) INVALID
//			} else if (skip_flag) {
//				skip_flag = 0;
//				if (!parse_bytes(0, &skip, argv[i])) INVALID
//			} else if (seek_flag) {
//				seek_flag = 0;
//				if (!parse_bytes(0, &seek, argv[i])) INVALID
			} else if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
				if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; // -- denotes end of flags (and -o)
				else {
					if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--out") == 0) {
						if (out_flag) INVALID
						out_flag = 1;
					} else if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) { INVALID
					} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--hexdump") == 0) {
						if (hexdump_done) INVALID
						hexdump = hexdump_done = 1;
					} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
						if (verbose_done) INVALID
						verbose = verbose_done = 1;
					} else if (i >= argc - 1) { INVALID
					} else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--block") == 0) {
						if (block_done) INVALID
						block_flag = block_done = 1;
					} else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0) {
						if (count_done) INVALID
						count_flag = count_done = 1;
//					} else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--skip") == 0) {
//						if (skip_done) INVALID
//						skip_flag = skip_done = 1;
//					} else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--seek") == 0) {
//						if (seek_done) INVALID
//						seek_flag = seek_done = 1;
					}
					else INVALID
				}
			} else if (out_flag) {
				output[output_len++] = argv[i]; // add to output
			} else {
				input[input_len++] = argv[i]; // add to input
			}
		}
		if (input_len  == 0) { ++input_len;  input[0]  = "-"; } // add stdin if no arguments
		if (output_len == 0) { ++output_len; output[0] = "-"; } // add stdout if no arguments
	}
	FILE* input_streams[input_len];
	FILE* output_streams[output_len];
#define ERROR(x) { fprintf(stderr, "%s: %s\n", x, strerror(errno)); return errno; }
	{
		char* input_streams_names[input_len];
		char* output_streams_names[output_len];
		for (size_t i = 0; i < input_len; ++i) {
			if (strcmp(input[i], STDIN) == 0) {
				input_streams[i] = stdin; // set to stdin
				input_streams_names[i] = "stdin";
			} else {
				input_streams[i] = fopen(input[i], "r"); // open file
				input_streams_names[i] = input[i];
			}
			if (!input_streams[i]) ERROR(input[i]);
		}
		for (size_t i = 0; i < output_len; ++i) {
			if (strcmp(output[i], STDOUT) == 0) {
				output_streams[i] = stdout; // set to stdout
				output_streams_names[i] = "stdout";
			} else if (strcmp(output[i], STDERR) == 0) {
				output_streams[i] = stderr; // set to stderr
				output_streams_names[i] = "stderr";
			} else {
				output_streams[i] = fopen(output[i], "w"); // open file
				output_streams_names[i] = output[i];
			}
			if (!output_streams[i]) ERROR(output[i]);
		}
		if (verbose) {
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
			fprintf(stderr, "Block Size: %s\n", display_bytes(block));
			fprintf(stderr, "Count: %s\n",      display_bytes(count));
//			fprintf(stderr, "Skip: %s\n",       display_bytes(skip));
//			fprintf(stderr, "Seek: %s\n\n",     display_bytes(seek));
		}
	}
	uint64_t blocks_read = 0;
	for (size_t i = 0; i < input_len; ++i) {
		char* data = malloc(block);
		uint64_t len = 0;
		while ((len = fread(data, 1, block, input_streams[i])) > 0) {
			for (size_t j = 0; j < output_len; ++j) {
				if (hexdump) {
					for (uint64_t k = 0; k < len; ++k) {
						fprintf(output_streams[j], "%02x", data[k]);
					}
				} else {
					fwrite(data, 1, len, output_streams[j]);
				}
			}
			if (count != 0 && ++blocks_read >= count) {
				if (verbose) fprintf(stderr, "Read count blocks, finished\n");
				return 0;
			}
		}
	}
	if (verbose) fprintf(stderr, "Finished\n");
	return 0;
}
