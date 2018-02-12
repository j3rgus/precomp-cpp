/* Copyright 2018 Jergus Lysy

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "3dm.h"
#include "opennurbs_3dm.h"
#include "opennurbs_crc.h"

static int lasterr = TDM_SUCCESS;
static char *errstring[] = {"Success.",
							"Invalid input given to a function",
							"Invalid or corrupted file.",
							"Invalid typecode.",
							"Out of memory.",
							"Invalid CRC.",
							"Invalid file version." };

static int check_table_typecode(tcode_t *tcode)
{
	uint32_t value = TCODE_TABLE;
	int ret = TDM_FUNC_ERROR;

	if (tcode == NULL) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	for (size_t i = 0x0010; i <= 0x0026; i++) {
		if ((value | i) == tcode->value) {
			ret = TDM_FUNC_SUCCESS;
		}
	}

	return ret;
}

int check_property_record(tcode_t *tcode)
{
	int ret = TDM_FUNC_ERROR;

	if (tcode == NULL) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	switch (tcode->value) {
		case TCODE_PROPERTIES_REVISIONHISTORY:
		case TCODE_PROPERTIES_NOTES:
		case TCODE_PROPERTIES_PREVIEWIMAGE:
		case TCODE_PROPERTIES_APPLICATION:
		case TCODE_PROPERTIES_COMPRESSED_PREVIEWIMAGE:
		case TCODE_PROPERTIES_OPENNURBS_VERSION:
		case TCODE_PROPERTIES_AS_FILE_NAME:
			ret = TDM_FUNC_SUCCESS;
			break;
	}

	return ret;
}

static int get_version_size(version_t *ver)
{
	return (ver->v_maj > 4) ? TDM_FILE_VERSION_SIZE : TDM_FILE_VERSION_SIZE_OLD;
}

static int get_crc_size(version_t *ver)
{
	return (ver->v_maj > 1) ? TDM_TCODE_CRC_SIZE : TDM_TCODE_CRC_SIZE_OLD;
}

static int read_tdm_file(FILE *fd_tdm, void *mem, size_t num_bytes)
{
	char *buf = (char *) mem;
	size_t readlen = 0;

	if ((fd_tdm == NULL) || (mem == NULL) || (num_bytes == 0)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	readlen = fread(buf, 1, num_bytes, fd_tdm);
	if (readlen < num_bytes) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	return TDM_FUNC_SUCCESS;
}

static int skip_chunk_fd(FILE *fd_tdm, version_t *ver)
{
	tcode_t tcode;
	size_t sz_data = 0;
	uint64_t size = 0;

	if ((fd_tdm == NULL) || (ver == NULL)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	if (read_tcode_fd(fd_tdm, &tcode, TDM_FILE_READ) != TDM_FUNC_SUCCESS) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	sz_data = get_version_size(ver);

	if (tcode.format) {
		/* If the chunk is short just skip the size data */
		fseek(fd_tdm, sz_data, SEEK_CUR);
	} else {
		/* Need to read size and skip all data */
		if (read_tdm_file(fd_tdm, &size, sz_data) != TDM_FUNC_SUCCESS) {
			return TDM_FUNC_ERROR;
		}
		fseek(fd_tdm, size, SEEK_CUR);
	}

	return TDM_FUNC_SUCCESS;
}

void print_tdm_error(char *str)
{
	fprintf(stderr, "%s: %s\n", str, errstring[lasterr]);
}

void clear_tdm_error(void)
{
	lasterr = 0;
}

void free_bchunk(chunk_big_t *b_chunk)
{
	if ((b_chunk != NULL) && (b_chunk->data != NULL)) {
		free(b_chunk->data);
		b_chunk->size = 0;
		b_chunk->data = NULL;
	}
}

int read_tcode_fd(FILE *fd_tdm, tcode_t *tcode, int peek)
{
	uint32_t tc = 0;

	if ((fd_tdm == NULL) || (tcode == NULL)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	if (read_tdm_file(fd_tdm, &tc, TDM_TCODE_SIZE) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	tcode->format = (tc & TCODE_SHORT) >> 31;
	tcode->category = (tc & 0x7fff0000) >> 16;
	tcode->crc_ctrl = (tc & TCODE_CRC) >> 15;
	tcode->spec_code = (tc & 0x7ff8) >> 3;
	tcode->stuff = (tc & 4) >> 2;
	tcode->reserved = (tc & 3);
	tcode->value = tc;

	if (peek == TDM_FILE_PEEK) {
		fseek(fd_tdm, -TDM_TCODE_SIZE, SEEK_CUR);
	}

	return TDM_FUNC_SUCCESS;
}

int read_schunk_fd(FILE *fd_tdm, version_t *ver, chunk_short_t *s_chunk, int peek)
{
	int data_size = 0;

	if ((fd_tdm == NULL) || (s_chunk == NULL) || (ver == NULL)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	if (read_tcode_fd(fd_tdm, &s_chunk->typecode, TDM_FILE_READ) != TDM_FUNC_SUCCESS) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	if (!s_chunk->typecode.format) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	data_size = get_version_size(ver);

	s_chunk->ver.v_maj = ver->v_maj;
	s_chunk->ver.v_min = ver->v_min;

	memset(s_chunk->data, 0, TDM_FILE_VERSION_SIZE); // zero the data memory
	if (read_tdm_file(fd_tdm, s_chunk->data, data_size) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	if (peek == TDM_FILE_PEEK) {
		fseek(fd_tdm, -(TDM_TCODE_SIZE + data_size), SEEK_CUR);
	}

	return TDM_FUNC_SUCCESS;
}

int read_bchunk_fd(FILE *fd_tdm, version_t *ver, chunk_big_t *b_chunk, int peek)
{
	int sz_size = 0;
	int sz_crc = 0;

	if ((fd_tdm == NULL) || (b_chunk == NULL) || (ver == NULL)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	/* Read and set typecode */
	if (read_tcode_fd(fd_tdm, &b_chunk->typecode, TDM_FILE_READ) != TDM_FUNC_SUCCESS) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	if (b_chunk->typecode.format) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	/* Decide size of size data and CRC size */
	sz_size = get_version_size(ver);
	if (b_chunk->typecode.crc_ctrl) {
		sz_crc = get_crc_size(ver);
	}

	/* Set the version */
	b_chunk->ver.v_maj = ver->v_maj;
	b_chunk->ver.v_min = ver->v_min;

	/* Read data length */
	b_chunk->size = 0;
	if (read_tdm_file(fd_tdm, &b_chunk->size, sz_size) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	/* Omit CRC data from the size */
	b_chunk->size -= sz_crc;

	/* Read and store data */
	b_chunk->data = malloc(b_chunk->size);
	if (b_chunk->data == NULL) {
		lasterr = TDM_ERROR_OUT_OF_MEMORY;
		return TDM_FUNC_ERROR;
	}

	if (read_tdm_file(fd_tdm, b_chunk->data, b_chunk->size) != TDM_FUNC_SUCCESS) {
		free(b_chunk->data);
		return TDM_FUNC_ERROR;
	}

	/* If available then read and store CRC */
	if (b_chunk->typecode.crc_ctrl) {
		b_chunk->crc = 0;
		if (read_tdm_file(fd_tdm, &b_chunk->crc, sz_crc) != TDM_FUNC_SUCCESS) {
			free(b_chunk->data);
			return TDM_FUNC_ERROR;
		}
	}

	if (peek == TDM_FILE_PEEK) {
		fseek(fd_tdm, -(TDM_TCODE_SIZE + sz_size + b_chunk->size + sz_crc), SEEK_CUR);
	}

	return TDM_FUNC_SUCCESS;
}

static int parse_header(FILE *fd_tdm, version_t *ver)
{
	char buffer[TDM_FILE_HEADER_SIZE];
	int v_major = -1;
	int v_minor = -1;

	if ((fd_tdm == NULL) || (ver == NULL)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	if (read_tdm_file(fd_tdm, buffer, TDM_FILE_HEADER_SIZE) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	/* Check magic. */
	if (strncmp(buffer, TDM_FILE_MAGIC, TDM_FILE_MAGIC_SIZE)) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	/* Check alignment and version. */
	for (size_t i = TDM_FILE_MAGIC_SIZE; i < (TDM_FILE_HEADER_SIZE - 2); i++) {
		if (buffer[i] != ' ') {
			lasterr = TDM_ERROR_INVALID_FILE;
			return TDM_FUNC_ERROR;
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

	/* Check version of file */
	if ((v_major == -1) && (v_minor == -1)) {
		lasterr = TDM_INVALID_FILE_VER;
		return TDM_FUNC_ERROR;
	}

	ver->v_maj = v_major;
	ver->v_min = v_minor;

	return TDM_FUNC_SUCCESS;
}

static int parse_property_table(FILE *fd_tdm, version_t *ver)
{
	tcode_t tcode;
	int sz_size = 0;
	uint64_t size = 0;

	if ((fd_tdm == NULL) || (ver == NULL)) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	sz_size = get_version_size(ver);

	/* Read typecode */
	if (read_tcode_fd(fd_tdm, &tcode, TDM_FILE_READ) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	/* Check if typecode is property table */
	if (tcode.value != TCODE_PROPERTIES_TABLE) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	/* Read size of the table */
	if (read_tdm_file(fd_tdm, &size, sz_size) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	/* Parse property records. Consider at most 7 records */
	for (size_t i = 0; i < 7; i++) {
		/* Peek into typecode */
		////fprintf(stderr, "%04x\n", tcode.value);

		if (read_tcode_fd(fd_tdm, &tcode, TDM_FILE_PEEK) != TDM_FUNC_SUCCESS) {
			lasterr = TDM_ERROR_INVALID_TCODE;
			return TDM_FUNC_ERROR;
		}

		if (tcode.value == TCODE_ENDOFTABLE) {
			if (skip_chunk_fd(fd_tdm, ver) != TDM_FUNC_SUCCESS) {
				return TDM_FUNC_ERROR;
			}
			break;
		}

		/* If the typecode is not a table record, then quit */
		if (check_property_record(&tcode) != TDM_FUNC_SUCCESS) {
			lasterr = TDM_ERROR_INVALID_TCODE;
			return TDM_FUNC_ERROR;
		}

		if (tcode.value != TCODE_PROPERTIES_COMPRESSED_PREVIEWIMAGE) {
			/* Skip other chunk than the one containing compressed data */
			if (skip_chunk_fd(fd_tdm, ver) != TDM_FUNC_SUCCESS) {
				return TDM_FUNC_ERROR;
			}
		} else {
			/* Dump compressed data of image */
			bitmap_header_t bmhdr;
			chunk_big_t b_chunk;
			char *data = NULL;
			uint32_t tc_anon;
			uint64_t c_image_size = 0;
			uint32_t crc32 = 0;
			uint32_t calc_crc32 = 0;

			//fprintf(stderr, "Compressed preview image detected.\n");
			if (read_bchunk_fd(fd_tdm, ver, &b_chunk, TDM_FILE_READ) != TDM_FUNC_SUCCESS) {
				return TDM_FUNC_ERROR;
			}

			/* Store bitmap header data of image into structure */
			data = b_chunk.data;
			memcpy(&bmhdr, data, sizeof(bitmap_header_t));
			data += sizeof(bitmap_header_t) + 9;
			memcpy(&tc_anon, data, TDM_TCODE_SIZE);
			if (tc_anon != TCODE_ANONYMOUS_CHUNK) {
				lasterr = TDM_ERROR_INVALID_TCODE;
				free_bchunk(&b_chunk);
				return TDM_FUNC_ERROR;
			}
			data += TDM_TCODE_SIZE;
			memcpy(&c_image_size, data, sz_size);
			data += sz_size;
			c_image_size -= 4; // CRC at the end of data
			memcpy(&crc32, &data[c_image_size], 4);

			calc_crc32 = ON_CRC32(0, c_image_size, data);

			if (calc_crc32 != crc32) {
				lasterr = TDM_ERROR_INVALID_CRC;
				free_bchunk(&b_chunk);
				return TDM_FUNC_ERROR;
			}

			/* Don't dump for testing purposes
			FILE *fw = fopen("dump.bin", "wb");
			fwrite(data, 1, c_image_size, fw);
			fclose(fw); */
			free_bchunk(&b_chunk);
		}

	}

	return TDM_FUNC_SUCCESS;
}

int parse_tdm_file(const char *filename)
{
	FILE *fd = NULL;
	version_t ver;
	tcode_t tcode;
	chunk_big_t last_chunk;
	int ret = TDM_FUNC_SUCCESS;
	int do_quit = 0;
	long file_size = 0;
	int64_t read_file_size = 0;

	if (filename == NULL) {
		lasterr = TDM_ERROR_FUNC_INPUT;
		return TDM_FUNC_ERROR;
	}

	fd = fopen(filename, "rb");
	if (fd == NULL) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	/* Get file size for later usage */
	if (fseek(fd, 0, SEEK_END) == -1) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	file_size = ftell(fd);
	if (file_size == -1) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	if (fseek(fd, 0, SEEK_SET) == -1) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	/* Check header and parse version */
	if (parse_header(fd, &ver) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	//fprintf(stderr, "Header parsed.\n");

	/* The file must contain TCODE_COMMENTBLOCK as the first chunk */
	/* All values are stored as little endian */
	if (read_tcode_fd(fd, &tcode, TDM_FILE_PEEK) != TDM_FUNC_SUCCESS) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	if (tcode.value != TCODE_COMMENTBLOCK) {
		lasterr = TDM_ERROR_INVALID_TCODE;
		return TDM_FUNC_ERROR;
	}

	//fprintf(stderr, "Comment chunk parsed.\n");

	/* Skip the comment chunk */
	if (skip_chunk_fd(fd, &ver) != TDM_FUNC_SUCCESS) {
		return TDM_FUNC_ERROR;
	}

	//fprintf(stderr, "Comment chunk skipped.\n");

	/* Read tables. Restrict possible number of tables to 100 */
	for (size_t i = 0; i < 100; i++) {
		if (do_quit) {
			break;
		}

		/* Peek into typecode */
		if (read_tcode_fd(fd, &tcode, TDM_FILE_PEEK) != TDM_FUNC_SUCCESS) {
			lasterr = TDM_ERROR_INVALID_TCODE;
			return TDM_FUNC_ERROR;
		}

		//fprintf(stderr, "Peeked typecode %04x\n", tcode.value);

		/* Search only following tables */
		switch (tcode.value) {
			case TCODE_PROPERTIES_TABLE:
				/* In property table interested in compressed preview image */
				//fprintf(stderr, "Property table found.\n");
				ret = parse_property_table(fd, &ver);
				break;
			case TCODE_BITMAP_TABLE:
				/* In bitmap table interested in compressed image stored */
				// NOT IMPLEMENTED
				if (skip_chunk_fd(fd, &ver) != TDM_FUNC_SUCCESS) {
					return TDM_FUNC_ERROR;
				}
				//fprintf(stderr, "Bitmap table found.\n");
				break;
			case TCODE_ENDOFFILE:
				ret = read_bchunk_fd(fd, &ver, &last_chunk, TDM_FILE_READ);
				//fprintf(stderr, "End of table found.\n");
				do_quit = 1;
				break;
			default:
				/* Not interested in other tables. */
				/* This means that some files with legacy typecodes will not be parsed correctly */
				/* If the typecode is not a table then quit */
				if (check_table_typecode(&tcode) != TDM_FUNC_SUCCESS) {
					lasterr = TDM_ERROR_INVALID_TCODE;
					ret = TDM_FUNC_ERROR;
				} else {
					ret = skip_chunk_fd(fd, &ver);
				}
				break;
		}

		if (ret != TDM_FUNC_SUCCESS) {
			return TDM_FUNC_ERROR;
		}

		if (do_quit) {
			break;
		}
	}

	if (do_quit) {
		if (last_chunk.size <= 8) {
			memcpy(&read_file_size, last_chunk.data, last_chunk.size);
		}
		free_bchunk(&last_chunk);
	} else {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	/* Check if file size and last chunk file size match */
	if (file_size != read_file_size) {
		lasterr = TDM_ERROR_INVALID_FILE;
		return TDM_FUNC_ERROR;
	}

	fclose(fd);

	return TDM_FUNC_SUCCESS;
}
