#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifdef X
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>
#endif
#define RANDOM_FILE "/dev/urandom"
#define STDIN "-"
#define STDOUT "-"
#define STDERR "!"
#define FILE_PREFIX "f:"
#define APPEND_PREFIX "a:"
int usage(char *argv0) {
	fprintf(stderr, "\
Usage: %s <in files> -o <out files>\n\n\
	-? --help             : return this help menu\n\
	-v --verbose          : report what it's doing to stderr\n\
	<in files>            : use "STDIN" for stdin (default), or use a file name\n\
	-o --out <out files>  : use "STDOUT" for stdout (default), use "STDERR" for stderr, or use a file name\n\
	                        for in files and out files, prefix it with "FILE_PREFIX" to ensure it's treated as a file\n\
	                        for out files, prefixing with "APPEND_PREFIX" has the same effect as "FILE_PREFIX" but it also appends instead of overwrites\n\
	                        if an out file and in file are the same, the file's contents will be wiped, if the out file is set to append, the file contents will be doubled\n\
	-h --hexdump          : output in hex instead\n\
	-b --block <bytes>    : block size, how many bytes to read at once, see below for what to put for size, 4096 is default\n\
	-c --count <blocks>   : count blocks, amount of blocks to read, this means total blocks not blocks for each input file, 0 means whole file (default)\n\
	                        this won't be accurate if we are reading less than the block size\n"
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
	uint64_t e = 0, p = 0, t = 0, g = 0, m = 0, k = 0;
#define ADD_NUM(val, num) { while (bytes >= val) { ++num; bytes -= val; } }
	ADD_NUM(1ull<<60, e);
	ADD_NUM(1ull<<50, p);
	ADD_NUM(1ull<<40, t);
	ADD_NUM(1ull<<30, g);
	ADD_NUM(1ull<<20, m);
	ADD_NUM(1ull<<10, k);
	char* res = malloc(256);
	char* tmp = malloc(256);
	res[0] = 0;
#define ADD_STR(suf, num) { if (num > 0) { sprintf(tmp, "%li"suf" %s", num, res); strcpy(res, tmp); } }
	ADD_STR("",  bytes)
	ADD_STR("K", k)
	ADD_STR("M", m)
	ADD_STR("G", g)
	ADD_STR("T", t)
	ADD_STR("P", p)
	ADD_STR("E", e)
#undef ADD_NUM
#undef ADD_STR
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
#define SUF1024(s0, s1, s2, s3, m) else if \
	((suffix_len == 1 && strncmp(suffix, s0, 1) == 0) || (suffix_len == 3 && (strncmp(suffix, s1, 3) == 0 || strncmp(suffix, s2, 3) == 0 || strncmp(suffix, s3, 3) == 0))) \
	num *= m;
#define SUF1000(s0, s1, s2, m) else if \
	((suffix_len == 2 && (strncmp(suffix, s0, 2) == 0 || strncmp(suffix, s1, 2) == 0)) || (suffix_len == 1 && strncmp(suffix, s2, 1) == 0)) \
	num *= m;
	if (suffix_len == 0); // allow no suffix
	SUF1024("K", "KiB", "KIB", "kib", 1ull<<10)
	SUF1024("M", "MiB", "MIB", "mib", 1ull<<20)
	SUF1024("G", "GiB", "GIB", "gib", 1ull<<30)
	SUF1024("T", "TiB", "TIB", "tib", 1ull<<40)
	SUF1024("P", "PiB", "PIB", "pib", 1ull<<50)
	SUF1024("E", "EiB", "EIB", "eib", 1ull<<60)
	SUF1000("KB", "kb", "k", 1000ull)
	SUF1000("MB", "mb", "m", 1000000ull)
	SUF1000("GB", "gb", "g", 1000000000ull)
	SUF1000("TB", "tb", "t", 1000000000000ull)
	SUF1000("PB", "pb", "p", 1000000000000000ull)
	SUF1000("EB", "eb", "e", 1000000000000000000ull)
	else return 0;
#undef SUF1024
#undef SUF1000
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
	char* input_streams_names[input_len];
	FILE* output_streams[output_len];
	char* output_streams_names[output_len];
#define ERROR(y, x) { fprintf(stderr, "%s %s: %s\n", y, x, strerror(errno)); return errno; }
	{
		size_t file_prefix_len = strlen(FILE_PREFIX);
		size_t append_prefix_len = strlen(FILE_PREFIX);
		for (size_t i = 0; i < input_len; ++i) {
			if (strncmp(input[i], FILE_PREFIX, file_prefix_len) == 0) {
				char* a = input[i] + file_prefix_len;
				input_streams[i] = fopen(a, "rb"); // open file, read, binary
				if (!input_streams[i]) ERROR("Input", input[i]);
				input_streams_names[i] = a;
			} else if (strcmp(input[i], STDIN) == 0) {
				input_streams[i] = stdin; // set to stdin
				input_streams_names[i] = "stdin";
			} else {
				input_streams[i] = fopen(input[i], "rb"); // open file, read, binary
				if (!input_streams[i]) ERROR("Input", input[i]);
				input_streams_names[i] = input[i];
			}
		}
		for (size_t i = 0; i < output_len; ++i) {
			if (strncmp(output[i], APPEND_PREFIX, append_prefix_len) == 0) {
				char* a = output[i] + append_prefix_len;
				output_streams[i] = fopen(a, "ab"); // open file, append, binary
				if (!output_streams[i]) ERROR("Output (append)", a);
				char* b = malloc(strlen(a));
				sprintf(b, "%s (append)", a);
				output_streams_names[i] = b;
			} else if (strncmp(output[i], FILE_PREFIX, file_prefix_len) == 0) {
				char* a = output[i] + file_prefix_len;
				output_streams[i] = fopen(a, "wb"); // open file, write, binary
				if (!output_streams[i]) ERROR("Output", a);
				output_streams_names[i] = a;
			} else if (strcmp(output[i], STDOUT) == 0) {
				output_streams[i] = stdout; // set to stdout
				output_streams_names[i] = "stdout";
			} else if (strcmp(output[i], STDERR) == 0) {
				output_streams[i] = stderr; // set to stderr
				output_streams_names[i] = "stderr";
			} else {
				output_streams[i] = fopen(output[i], "wb"); // open file, write, binary
				if (!output_streams[i]) ERROR("Output", output[i]);
				output_streams_names[i] = output[i];
			}
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
		uint8_t* data = malloc(block);
		uint64_t len = 0;
		while (1) {
			if (!input_streams[i]) break;
			if (feof(input_streams[i])) {
				if (verbose) fprintf(stderr, "Input %s: EOF\n", input_streams_names[i]);
				input_streams[i] = NULL;
				break;
			}
			if (ferror(input_streams[i])) {
				if (verbose) fprintf(stderr, "Input %s: Error\n", input_streams_names[i]);
				input_streams[i] = NULL;
				break;
			}
			len = fread(data, 1, block, input_streams[i]);
			for (size_t j = 0; j < output_len; ++j) {
				if (!output_streams[j]) continue;
				if (feof(output_streams[j])) {
					if (verbose) fprintf(stderr, "Output %s: EOF\n", output_streams_names[j]);
					output_streams[j] = NULL;
					continue;
				}
				if (ferror(output_streams[j])) {
					if (verbose) fprintf(stderr, "Output %s: Error\n", output_streams_names[j]);
					output_streams[j] = NULL;
					continue;
				}
				if (hexdump) {
					for (uint64_t k = 0; k < len; ++k) {
						fprintf(output_streams[j], "%02x", data[k]);
					}
				} else {
					fwrite(data, 1, len, output_streams[j]);
				}
			}
			if (count != 0) {
				if (++blocks_read >= count) {
					if (verbose) fprintf(stderr, "Read count blocks, finished\n");
					return 0;
				}
			}
		}
	}
	if (verbose) fprintf(stderr, "Finished\n");
	return 0;
}
