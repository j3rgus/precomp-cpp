#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "opennurbs_3dm.h"
#include "opennurbs_crc.h"

<<<<<<< HEAD
#define TDM_TCODE_SIZE				4

#define TDM_FILE_MAGIC				"3D Geometry File Format "
#define TDM_FILE_MAGIC_SIZE			24
#define TDM_FILE_VERSION_SIZE		8
#define TDM_FILE_HEADER_SIZE		TDM_FILE_MAGIC_SIZE + TDM_FILE_VERSION_SIZE

#define TDM_SUCCESS					0
#define TDM_ERROR_FILE_DESC_INPUT	1
#define TDM_ERROR_INVALID_FILE		2
#define TDM_ERROR_INVALID_TCODE		3
=======
#define TDM_FILE_MAGIC				"3D Geometry File Format "
#define TDM_FILE_MAGIC_SIZE			24
#define TDM_FILE_HEADER_SIZE		TDM_FILE_MAGIC_SIZE + TDM_FILE_VERSION_SIZE
#define TDM_FILE_VERSION_SIZE_OLD	4
#define TDM_FILE_VERSION_SIZE		8

#define TDM_TCODE_SIZE				4
#define TDM_TCODE_CRC_SIZE_OLD		2
#define TDM_TCODE_CRC_SIZE			4
#define TDM_TCODE_CRC_SEED			0x0

#define TDM_SUCCESS					0
#define TDM_ERROR_FILE_FUNC_INPUT	1
#define TDM_ERROR_INVALID_FILE		2
#define TDM_ERROR_INVALID_TCODE		3
#define TDM_ERROR_OUT_OF_MEMORY		4
#define TDM_ERROR_INVALID_CRC		5
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129

#define ERR_STRING_SIZE				128

static char errstring[ERR_STRING_SIZE];
//static int errnum = 0;

<<<<<<< HEAD
=======
typedef struct bitmap_header {
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t bitCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} bitmap_header_t;

>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
void print_hex(char *data, size_t offset, size_t len)
{
	for (size_t i = offset; i < (offset + len); i++) {
		printf("%02x ", (unsigned char) data[i]);
	}
	printf("\n");
}

void skip_n_bytes(FILE *f, size_t n)
{
    while (n > 0) {
        (void) fgetc(f);
        n--;
    }
}

<<<<<<< HEAD
int parse_record(FILE *fr)
{
	return TDM_SUCCESS;
}

int parse_table(FILE *fr)
{
=======
int parse_bitmap_hdr(FILE *fr, bitmap_header_t *bhdr)
{
	if (fr == NULL || bhdr == NULL) {
		strncpy(errstring, "Invalid inpupt given to function.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_FILE_FUNC_INPUT;
	}

	fread(&bhdr->biSize, 1, sizeof(bhdr->biSize), fr);
	if (bhdr->biSize != sizeof(bitmap_header_t)) {
		strncpy(errstring, "Invalid format of bitmap header.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	fread(&bhdr->biWidth, 1, sizeof(bhdr->biWidth), fr);
	fread(&bhdr->biHeight, 1, sizeof(bhdr->biHeight), fr);

	fread(&bhdr->biPlanes, 1, sizeof(bhdr->biPlanes), fr);
	if (bhdr->biPlanes != 1) {
		strncpy(errstring, "Invalid format of bitmap header.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	fread(&bhdr->biBitCount, 1, sizeof(bhdr->biBitCount), fr);
	if ((bhdr->biBitCount != 0) && (bhdr->biBitCount != 1) && (bhdr->biBitCount != 4) && (bhdr->biBitCount != 8)
		&& (bhdr->biBitCount != 16) && (bhdr->biBitCount != 24) && (bhdr->biBitCount != 32))
	{
		strncpy(errstring, "Invalid format of bitmap header.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	fread(&bhdr->bitCompression, 1, sizeof(bhdr->bitCompression), fr);
	if (bhdr->bitCompression > 4) {
		strncpy(errstring, "Invalid format of bitmap header.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	fread(&bhdr->biSizeImage, 1, sizeof(bhdr->biSizeImage), fr);
	fread(&bhdr->biXPelsPerMeter, 1, sizeof(bhdr->biXPelsPerMeter), fr);
	fread(&bhdr->biClrUsed, 1, sizeof(bhdr->biClrUsed), fr);
	fread(&bhdr->biClrImportant, 1, sizeof(bhdr->biClrImportant), fr);

	return TDM_SUCCESS;
}

int parse_table(FILE *fr, int v_maj, int v_min)
{
	size_t readlen = 0;
	uint32_t tcode = 0;
	uint64_t size = 0;
	int sz_crc = 0;
	int sz_size = 0;

	if (fr == NULL) {
		strncpy(errstring, "Invalid file descriptor.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_FILE_FUNC_INPUT;
	}

	for (;;) {
		/* Read and parse records. */
		readlen = fread(&tcode, 1, TDM_TCODE_SIZE, fr);
		if (readlen < TDM_TCODE_SIZE) {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}

		printf("  TCODE: %04x\n", tcode);

		if (tcode == TCODE_ENDOFTABLE) {
			break;
		}

		if (!(tcode & TCODE_TABLEREC)) {
			strncpy(errstring, "Table does not contain a record typecode.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_TCODE;
		}

		if (v_maj < 5 || (v_maj == 5 && v_min == -1)) {
			sz_crc = TDM_TCODE_CRC_SIZE_OLD;
			sz_size = TDM_FILE_VERSION_SIZE_OLD;
		} else if (v_maj >= 5 && v_min >= 0) {
			sz_crc = TDM_TCODE_CRC_SIZE;
			sz_size = TDM_FILE_VERSION_SIZE;
		} else {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}

		readlen = fread(&size, 1, sz_size, fr);
		if (readlen < sz_size) {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}

		if (tcode == TCODE_OBJECT_TABLE) {
			//parse_object_table();
		} else if (!(tcode & TCODE_SHORT)) {
			/* Check if CRC is needed, skip if not */
			if (!(tcode & TCODE_CRC)) {
				skip_n_bytes(fr, size);
			} else {
				char *mem = NULL;

				mem = malloc(size);
				if (mem == NULL) {
					strncpy(errstring, "Not enough memory to store data", ERR_STRING_SIZE - 1);
					return TDM_ERROR_OUT_OF_MEMORY;
				}

				if (tcode == TCODE_PROPERTIES_COMPRESSED_PREVIEWIMAGE) {
					bitmap_header_t bhdr;
					int err = parse_bitmap_hdr(fr, &bhdr);

					if (err == TDM_SUCCESS) {
						printf("%04x\n", bhdr.biSize);
						readlen = fread(mem, 1, size - bhdr.biSize, fr);
						if (readlen < (size - bhdr.biSize)) {
							strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
							return TDM_ERROR_INVALID_FILE;
						}

						FILE *fw = fopen("dump.bin", "wr");
						fwrite(mem, 1, size - bhdr.biSize, fw);
						fclose(fw);
					} else {
						return err;
					}
				} else {
					readlen = fread(mem, 1, size, fr);
					if (readlen < size) {
						strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
						return TDM_ERROR_INVALID_FILE;
					}
				}

				/* Check the CRC */
				if (sz_crc == TDM_TCODE_CRC_SIZE_OLD) {
					uint16_t calc_crc16 = ON_CRC16(TDM_TCODE_CRC_SEED, size - sz_crc, mem);
					uint16_t crc16 = *(uint16_t *) &mem[size - sz_crc];
					if (crc16 != calc_crc16) {
						strncpy(errstring, "Found invalid CRC for chunk.", ERR_STRING_SIZE - 1);
						return TDM_ERROR_INVALID_CRC;
					}
				} else if (sz_crc == TDM_TCODE_CRC_SIZE) {
					uint32_t calc_crc32 = ON_CRC32(TDM_TCODE_CRC_SEED, size - sz_crc, mem);
					uint32_t crc32 = *(uint32_t *) &mem[size - sz_crc];
					if (crc32 != calc_crc32) {
						strncpy(errstring, "Found invalid CRC for chunk.", ERR_STRING_SIZE - 1);
						return TDM_ERROR_INVALID_CRC;
					}
				} else {
						strncpy(errstring, "Invalid size of CRC detected.", ERR_STRING_SIZE - 1);
						return TDM_ERROR_INVALID_FILE;
				}
				free(mem);
			}
		}
	}

>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
	return TDM_SUCCESS;
}

int parse_3dm(FILE *fr)
{
	char buffer[64];
	size_t readlen = 0;
	int v_major = -1;
	int v_minor = -1;
	int short_chunk = 0;
	int do_crc = 0;
	uint32_t tcode = 0;
	uint32_t size_v1 = 0;
	uint64_t size_v2 = 0;

	if (fr == NULL) {
		strncpy(errstring, "Invalid file descriptor.", ERR_STRING_SIZE - 1);
<<<<<<< HEAD
		return TDM_ERROR_FILE_DESC_INPUT;
=======
		return TDM_ERROR_FILE_FUNC_INPUT;
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
	}

	/*Read the MAGIC string from the file.*/
	readlen = fread(buffer, 1, TDM_FILE_HEADER_SIZE, fr);
	if (readlen < TDM_FILE_HEADER_SIZE) {
		strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	/* Check magic. */
	if (strncmp(buffer, TDM_FILE_MAGIC, TDM_FILE_MAGIC_SIZE)) {
		strncpy(errstring, "Invalid header of 3dm file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	/* Check alignment and version. */
	for (size_t i = TDM_FILE_MAGIC_SIZE; i < (TDM_FILE_HEADER_SIZE - 2); i++) {
		if (buffer[i] != ' ') {
			strncpy(errstring, "File version is not properly aligned.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}
	}

	if (isdigit(buffer[TDM_FILE_HEADER_SIZE - 1])) {
		if (isdigit(buffer[TDM_FILE_HEADER_SIZE - 2])) {
			v_major = buffer[TDM_FILE_HEADER_SIZE - 2] - '0';
			v_minor = buffer[TDM_FILE_HEADER_SIZE - 1] - '0';
		} else {
			v_major = buffer[TDM_FILE_HEADER_SIZE - 1] - '0';
		}
	}

	if ((v_major > 6) || (v_minor > 0) || (v_major < 5 && v_minor != -1)) {
		strncpy(errstring, "Invalid version of file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	/* The file must contain TCODE_COMMENTBLOCK as the first chunk */
	/* All values are stored as little endian */
	readlen = fread(&tcode, 1, TDM_TCODE_SIZE, fr);
	if (readlen < TDM_TCODE_SIZE) {
		strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	if (tcode ^ TCODE_COMMENTBLOCK) {
		strncpy(errstring, "First chunk must be comment block.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

<<<<<<< HEAD
	if (v_major < 5 && v_major > 0) {
		readlen = fread(&size_v1, 1, 4, fr);
		if (readlen < 4) {
=======
	if (v_major < 5) {
		readlen = fread(&size_v1, 1, TDM_FILE_VERSION_SIZE_OLD, fr);
		if (readlen < TDM_FILE_VERSION_SIZE_OLD) {
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}
		skip_n_bytes(fr, size_v1);
	} else if (v_major >= 5) {
<<<<<<< HEAD
		readlen = fread(&size_v2, 1, 8, fr);
		if (readlen < 8) {
=======
		readlen = fread(&size_v2, 1, TDM_FILE_VERSION_SIZE, fr);
		if (readlen < TDM_FILE_VERSION_SIZE) {
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}
		skip_n_bytes(fr, size_v2);
	}

	/* Read out all chunks from the file */
	for (;;) {
		readlen = fread(&tcode, 1, TDM_TCODE_SIZE, fr);
		if (readlen < TDM_TCODE_SIZE) {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}

		printf("TCODE: %04x\n", tcode);

		if (tcode & 0x040c8000) {
			strncpy(errstring, "Invalid typecode in 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_TCODE;
		}

		if (tcode & TCODE_SHORT) {
			short_chunk = 1;
		}

		if (tcode & TCODE_CRC) {
			do_crc = 1;
		}

<<<<<<< HEAD
		if (v_major < 5 && v_major > 0) {
=======
		if (v_major < 5) {
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
			readlen = fread(&size_v1, 1, 4, fr);
			if (readlen < 4) {
				strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
				return TDM_ERROR_INVALID_FILE;
			}
<<<<<<< HEAD
			skip_n_bytes(fr, size_v1);
=======
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
		} else if (v_major >= 5) {
			readlen = fread(&size_v2, 1, 8, fr);
			if (readlen < 8) {
				strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
				return TDM_ERROR_INVALID_FILE;
			}
<<<<<<< HEAD
			skip_n_bytes(fr, size_v2);
=======
		} else {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}

		if (tcode & TCODE_TABLE) {
			int err = parse_table(fr, v_major, v_minor);
			if (err != TDM_SUCCESS) {
				return err;
			}
>>>>>>> 9c344386d557f8f9dfa1d19583f224a67f2b5129
		}

		if (tcode == TCODE_ENDOFFILE) {
			break;
		}
	}

	return TDM_SUCCESS;
}

int main(int argc, char **argv)
{
	FILE *fr = NULL;
	int err;

	if (argc < 2)
		return 1;

	fr = fopen(argv[1], "rb");
	if (fr == NULL) {
		perror("fopen:");
		return 1;
	}
	err = parse_3dm(fr);
	if (err > 0) {
		fprintf(stderr, "parse_3dm returned %d: %s\n", err, errstring);
		return 1;
	}

	fclose(fr);

	return 0;
}
