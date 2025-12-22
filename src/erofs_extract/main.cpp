// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 skkk
 */

#include <getopt.h>
#include <erofs/io.h>
#include <erofs/config.h>
#include <erofs/print.h>
#include <iostream>
#include <sys/time.h>

#include "../lib/compressor.h"
#include "../lib/liberofs_compress.h"

#include "ExtractState.h"
#include "ExtractOperation.h"
#include "Logging.h"

#if defined(__CYGWIN__) || defined(_WIN32)
#include "CaseSensitiveInfo.h"
#endif

using namespace skkk;

static inline void get_available_compressors(string &ret) {
	int i = 0;
	bool comma = false;
	const struct erofs_algorithm *s;

	while ((s = z_erofs_list_available_compressors(&i)) != nullptr) {
		if (comma)
			ret.append(", ");
		ret.append(s->name);
		comma = true;
	}
}

static inline void usage() {
	char buf[1536] = {0};
	snprintf(buf, 1536,
			 BROWN "usage: [options]" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-h, --help" COLOR_NONE "              " BROWN "Display this help and exit" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-i, --image=[FILE]" COLOR_NONE "      " BROWN "Image file" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "--offset=#" COLOR_NONE "              " BROWN "skip # bytes at the beginning of IMAGE" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-p" COLOR_NONE "                      " BROWN "Print all entrys" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-P, --print=X" COLOR_NONE "           " BROWN "Print the target of path X" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-x" COLOR_NONE "                      " BROWN "Extract all items" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-X, --extract=X" COLOR_NONE "         " BROWN "Extract the target of path X" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-c, --config=[FILE]" COLOR_NONE "     " BROWN "Target of config" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-r" COLOR_NONE "                      " BROWN "When using config, recurse directories" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-s" COLOR_NONE "                      " BROWN "Silent mode, Don't show progress" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-f, --overwrite" COLOR_NONE "         " BROWN "[" GREEN2_BOLD "default: skip" COLOR_NONE BROWN "] overwrite files that already exist" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-T#" COLOR_NONE "                     " BROWN "[" GREEN2_BOLD "1-%u" COLOR_NONE BROWN "] Use # threads, default: -T0, is " GREEN2_BOLD "%u" COLOR_NONE COLOR_NONE "\n"
			 "  " GREEN2_BOLD "--only-cfg" COLOR_NONE "              " BROWN "Only extract fs_config|file_contexts|fs_options" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-o, --outdir=X" COLOR_NONE "          " BROWN "Output dir" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-V, --version" COLOR_NONE "           " BROWN "Print the version info" COLOR_NONE "\n",
			 eo->limitHardwareConcurrency,
			 eo->hardwareConcurrency
	);
	std::cerr << buf << std::endl;
}

#ifndef EXTRACT_BUILD_TIME
#define EXTRACT_BUILD_TIME "-0"
#endif

static inline void print_version() {
	string compressors;
	get_available_compressors(compressors);
	printf("  " BROWN "erofs-utils:" COLOR_NONE "            " RED2_BOLD "%s" COLOR_NONE "\n", cfg.c_version);
	printf("  " BROWN "extract.erofs:" COLOR_NONE "          " RED2_BOLD "1.0.7" EXTRACT_BUILD_TIME COLOR_NONE "\n");
	printf("  " BROWN "Available compressors:" COLOR_NONE "  " RED2_BOLD "%s" COLOR_NONE "\n", compressors.c_str());
	printf("  " BROWN "extract author:" COLOR_NONE "         " RED2_BOLD "skkk" COLOR_NONE "\n");
}

static struct option arg_options[] = {
	{"help",      no_argument,       nullptr, 'h'},
	{"version",   no_argument,       nullptr, 'V'},
	{"image",     required_argument, nullptr, 'i'},
	{"offset",    required_argument, nullptr, 2},
	{"outdir",    required_argument, nullptr, 'o'},
	{"print",     required_argument, nullptr, 'P'},
	{"overwrite", no_argument,       nullptr, 'f'},
	{"extract",   required_argument, nullptr, 'X'},
	{"config",    required_argument, nullptr, 'c'},
	{"only-cfg",  no_argument,       nullptr, 1},
	{nullptr,     no_argument,       nullptr, 0},
};

static int parseAndCheckExtractCfg(int argc, char **argv) {
	int opt;
	int rc = RET_EXTRACT_CONFIG_FAIL;
	bool enterParseOpt = false;
	while ((opt = getopt_long(argc, argv, "hi:psxfrc:P:T:o:X:V", arg_options, nullptr)) != -1) {
		enterParseOpt = true;
		switch (opt) {
			case 'h':
				usage();
				goto exit;
			case 'V':
				print_version();
				goto exit;
			case 'i':
				if (optarg) {
					eo->setImgPath(optarg);
				}
				LOGD("imgPath=%s", eo->getImgPath().c_str());
				break;
			case 'o':
				if (optarg) {
					eo->setOutDir(optarg);
				}
				LOGD("outDir=%s", eo->getOutDir().c_str());
				break;
			case 'p':
				eo->isPrintAllNode = true;
				LOGD("isPrintAllNode=%d", eo->isPrintAllNode);
				break;
			case 'P':
				eo->isPrintTarget = true;
				if (optarg) eo->targetPath = optarg;
				LOGD("isPrintTarget=%d targetPath=%s", eo->isPrintTarget, eo->targetPath.c_str());
				break;
			case 'f':
				eo->overwrite = true;
				LOGD("overwrite=%d", eo->overwrite);
				break;
			case 'x':
				eo->check_decomp = true;
				eo->isExtractAllNode = true;
				LOGD("isExtractAllNode=%d check_decomp=%d", eo->isExtractAllNode, eo->check_decomp);
				break;
			case 'X':
				eo->check_decomp = true;
				eo->isExtractTarget = true;
				if (optarg) eo->targetPath = optarg;
				LOGD("isExtractTarget=%d targetPath=%s", eo->isExtractTarget, eo->targetPath.c_str());
				break;
			case 'c':
				eo->isExtractTargetConfig = true;
				if (optarg) eo->targetConfigPath = optarg;
				LOGD("targetConfig=%s", eo->targetConfigPath.c_str());
				break;
			case 's':
				eo->isSilent = true;
				LOGD("isSilent=%d", eo->isSilent);
				break;
			case 'r':
				eo->targetConfigRecurse = true;
				LOGD("targetConfigRecurse=%d", eo->targetConfigRecurse);
				break;
			case 'T':
				if (optarg) {
					char *endPtr;
					uint64_t n = strtoull(optarg, &endPtr, 0);
					if (*endPtr == '\0') {
						eo->threadNum = n;
					}
				}
				break;
			case 1:
				eo->extractOnlyConfAndSeLabel = true;
				LOGD("extractOnlyConfAndSeLabel=%d", eo->extractOnlyConfAndSeLabel);
				break;
			case 2:
				if (optarg) {
					char *endPtr;
					uint64_t n = strtoull(optarg, &endPtr, 0);
					if (*endPtr == '\0') {
						g_sbi.bdev.offset = n;
						LOGD("offset=%lu", g_sbi.bdev.offset);
					}
				}
				break;
			default:
				usage();
				print_version();
				goto exit;
		}
	}

	if (enterParseOpt) {
		bool err;
		// check needed arg
		err = !eo->getImgPath().empty() && fileExists(eo->getImgPath());
		if (!err) {
			LOGE("img file '%s' does not exist", eo->getImgPath().c_str());
			goto exit;
		}
		rc = !eo->initOutDir();
		if (!rc) {
			goto exit;
		}
		LOGD("outDir=%s confDir=%s", eo->getOutDir().c_str(), eo->getConfDir().c_str());

		if (eo->threadNum > eo->limitHardwareConcurrency) {
			rc = RET_EXTRACT_THREAD_NUM_ERROR;
			LOGE("Threads min: 1 , max: %u", eo->limitHardwareConcurrency);
			goto exit;
		} else if (eo->threadNum == 0) {
			eo->threadNum = eo->hardwareConcurrency;
		}
		LOGD("Threads num=%u", eo->threadNum);

		rc = RET_EXTRACT_CONFIG_DONE;
	} else {
		usage();
	}

exit:
	return rc;
}

static inline void printOperationTime(struct timeval *start, struct timeval *end) {
	LOGI("The operation took: %3f second(s)\n",
		  (end->tv_sec - start->tv_sec) + static_cast<float>(end->tv_usec - start->tv_usec) / 1000000
	);
}

#if defined(_WIN32) || defined(CYGWIN)
static void handleWinTerminal() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin != INVALID_HANDLE_VALUE) {
		DWORD mode;
		if (GetConsoleMode(hStdin, &mode)) {
			mode &= ~ENABLE_QUICK_EDIT_MODE;
			mode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			mode &= ~ENABLE_MOUSE_INPUT;
			SetConsoleMode(hStdin, mode);
		}
	}
}

static void enableWinTerminalColor(DWORD handle) {
	HANDLE nHandle = GetStdHandle(handle);
	if (nHandle != INVALID_HANDLE_VALUE) {
		DWORD mode;
		if (GetConsoleMode(nHandle, &mode)) {
			mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(nHandle, mode);
		}
	}
}
#endif

int main(int argc, char **argv) {
	int ret = RET_EXTRACT_DONE, err;
	struct timeval start = {}, end = {};

#if defined(_WIN32) || defined(CYGWIN)
	handleWinTerminal();
	enableWinTerminalColor(STD_OUTPUT_HANDLE);
	enableWinTerminalColor(STD_ERROR_HANDLE);
#endif

	setbuf(stdout, nullptr);
	setbuf(stderr, nullptr);

	// Start time
	gettimeofday(&start, nullptr);

	// Initialize erofs config
	erofs_init_configure();
	cfg.c_dbg_lvl = EROFS_ERR;

	// Initialize extract config
	err = parseAndCheckExtractCfg(argc, argv);
	if (err != RET_EXTRACT_CONFIG_DONE) {
		ret = err;
		goto exit;
	}

	err = erofs_dev_open(&g_sbi, eo->getImgPath().c_str(), O_RDONLY);
	if (err) {
		ret = RET_EXTRACT_INIT_FAIL;
		LOGE("failed to open '%s'", eo->getImgPath().c_str());
		goto exit;
	}

	err = erofs_read_superblock(&g_sbi);
	if (err) {
		ret = RET_EXTRACT_INIT_FAIL;
		LOGE("failed to read superblock");
		goto exit_dev_close;
	}

	if (eo->isPrintTarget || eo->isExtractTarget || eo->isExtractTargetConfig)
		err = eo->initErofsNodeByTarget();
	else if (eo->isPrintAllNode || eo->isExtractAllNode)
		err = eo->initAllErofsNode();
	if (err) {
		ret = RET_EXTRACT_INIT_NODE_FAIL;
		goto exit_dev_close;
	}

	if (eo->isPrintTarget || eo->isPrintAllNode) {
		ExtractOperation::printInitializedNode();
		goto exit_dev_close;
	}

	LOGI("Starting... \n");

	if ((eo->isExtractTarget || eo->isExtractAllNode) && eo->extractOnlyConfAndSeLabel) {
		err = eo->createExtractConfigDir();
		if (err) {
			ret = RET_EXTRACT_CREATE_DIR_FAIL;
			goto exit_dev_close;
		}
		eo->extractFsConfigAndSelinuxLabelAndFsOptions();
		goto end;
	}

	if (eo->isExtractTarget || eo->isExtractAllNode) {
		err = eo->createExtractConfigDir() & eo->createExtractOutDir();
		if (err) {
			ret = RET_EXTRACT_CREATE_DIR_FAIL;
			goto exit_dev_close;
		}
#if defined(__CYGWIN__) || defined(_WIN32)
		// Dir must exist and empty.
		if (EnsureCaseSensitive(eo->getOutDir().c_str()) == 0)
			LOGI("Success change case sensitive.");
		else
			LOGW("Failed change case sensitive.");
#endif
		eo->extractFsConfigAndSelinuxLabelAndFsOptions();
		if (eo->threadNum == 1) {
			eo->extractErofsNode(eo->isSilent);
		} else {
			eo->extractErofsNodeMultiThread(eo->isSilent);
		}
		goto end;
	}

end:
	// End time
	gettimeofday(&end, nullptr);
	printOperationTime(&start, &end);

exit_dev_close:
	erofs_dev_close(&g_sbi);
	LOGD("ErofsNode size=%lu", eo->getErofsNodes().size());
	LOGD("main exit ret=%d", ret);

exit:
	erofs_blob_closeall(&g_sbi);
	erofs_exit_configure();
	ExtractOperation::erofsOperationExit();
	return ret;
}
