#include <stdio.h>
#include "3dm.h"

int main(int argc, char **argv)
{
	if ((argc < 2) || (argv[1] == NULL)) {
		fprintf(stderr, "Argument needed.\n");
		return 1;
	}

	if (parse_tdm_file(argv[1]) != TDM_FUNC_SUCCESS) {
		print_tdm_error("parse_tdm_file");
		return 1;
	}

	return 0;
}
