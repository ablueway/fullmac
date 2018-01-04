/*
* Purpose: APIs for decompress BMP image
*
* Author: Tristan Yang
*
* Date: 2009/08/14
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 2.00
*/

#include "gplib_bmp_decode.h"

#if GPLIB_BMP_EN == 1

// static global variables for BMP header parser only
static INT8U *bmp_parse_buf;
static INT8U *bmp_parse_end;
static INT32U bmp_parsed_count;
static PARSE_MAIN_STATE_ENUM bmp_main_state;
static PARSE_BMP_HEADER_STATE_ENUM bmp_header_state;
static PARSE_DIB_STATE_ENUM bmp_dib_state;

static INT32U bmp_data_offset;
static INT32U bmp_dib_header_size;			// 40=Windows V3, 12=OS/2 V1, 64=OS/2 V2, 108=Windows V4, 124=Windows V5
static INT16U bmp_image_width;
static INT16U bmp_image_height;
static INT8U bmp_top_to_bottom;				// 0=Bottom line to top line, 1=Top line to bottom line
static INT16U bmp_bits_per_pixel;			// 1, 4, 8, 16, 24 or 32
static INT32U bmp_compression_method;		// 0=None, 1=RLE(Run-Length-Encode) 8-bit, 2=RLE 4-bit, 3=Bit field, 4=JPEG, 5=PNG
static INT32U bmp_palette_color_number;
static INT32U bmp_palette_load_number;
static INT32U bmp_bitfield_mask_red;
static INT32U bmp_bitfield_mask_green;
static INT32U bmp_bitfield_mask_blue;
static INT16U bmp_palette[256];

static INT32U bmp_decode_status;
static INT8U bmp_leading_byte_offset;		// 0x0 ~ 0x3
static INT8U *bmp_current_in_addr;
static INT32U bmp_current_in_length;
static INT32U bmp_out_fifo_line_num;
static INT16U bmp_current_fifo_ab;			// 0=FIFO A, 1=FIFO B
static INT16U bmp_current_fifo_line;		// 16/32/64/128/256
static INT16U *bmp_out_start_addr;
static INT16U *bmp_current_out_addr;
static INT16U bmp_current_row;
static INT16U bmp_current_column;
static INT16U bmp_current_byte_index;		// 0=Blur, 1=Green, 2=Red
static INT8U bmp_padding_bytes;
static INT8U bmp_current_padding_index;

INT32S bmp_parse_bmp_header(void);
INT32S bmp_parse_dib_format(void);
INT32S bmp_parse_dib_header(void);
INT32S bmp_parse_color_palette(void);
INT32S bmp_decode_24bit_data(void);
INT32S bmp_decode_3216841bit_data(INT16U pixel);
INT32S bmp_decode_16bit_bitfield(void);
INT32S bmp_decode_32bit_bitfield(void);


void bmp_decode_init(void)
{
	// Initiate state machine
	bmp_parsed_count = 0;
	bmp_main_state = STATE_BMP_HEADER;
	bmp_header_state = STATE_MIGIC_NUMBER;
	bmp_dib_state = STATE_BMP_WIDTH;

	bmp_data_offset = 0;
	bmp_dib_header_size = 0;
	bmp_image_width = 0;
	bmp_image_height = 0;
	bmp_top_to_bottom = 0;
	bmp_bits_per_pixel = 0;
	bmp_compression_method = 0;
	bmp_palette_color_number = 0;

	bmp_current_in_addr = NULL;
	bmp_current_in_length = 0;

	bmp_decode_status = C_BMP_STATUS_STOP;
}

// State machine that parses BMP header
INT32S bmp_decode_parse_header(INT8U *buf_start, INT32U len)
{
	INT32S status;

	if (!len) {
		DBG_PRINT("bmp_decode_parse_header(): parameter len is 0!\r\n");

		return BMP_PARSE_FAIL;
	}

	bmp_parse_buf = buf_start;
	bmp_parse_end = bmp_parse_buf + len;

	switch (bmp_main_state) {
	case STATE_BMP_HEADER:
		status = bmp_parse_bmp_header();
		if (status != BMP_PARSE_OK) {
			return status;
		}

		bmp_main_state = STATE_DIB_FORMAT;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_DIB_FORMAT:
		status = bmp_parse_dib_format();
		if (status != BMP_PARSE_OK) {
			return status;
		}

		if (bmp_dib_header_size!=40 && bmp_dib_header_size!=12 && bmp_dib_header_size!=240) {
			DBG_PRINT("Unsupported DIB header format!\r\n");

			return BMP_PARSE_FAIL;
		}
		bmp_main_state = STATE_DIB_HEADER;
		bmp_dib_state = STATE_BMP_WIDTH;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_DIB_HEADER:
		status = bmp_parse_dib_header();
		if (status != BMP_PARSE_OK) {
			return status;
		}

		if (bmp_bits_per_pixel<16 || bmp_compression_method==3) {
			bmp_main_state = STATE_COLOR_PALETTE;
			bmp_palette_load_number = 0;
			bmp_bitfield_mask_red = 0;
			bmp_bitfield_mask_green = 0;
			bmp_bitfield_mask_blue = 0;
		} else {
			bmp_main_state = STATE_BITMAP_DATA;
		}
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_COLOR_PALETTE:
		if (bmp_main_state == STATE_COLOR_PALETTE) {
			status = bmp_parse_color_palette();
			if (status != BMP_PARSE_OK) {
				return status;
			}

			bmp_main_state = STATE_BITMAP_DATA;
			bmp_parsed_count = 0;
			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
	case STATE_BITMAP_DATA:
		bmp_current_in_addr = bmp_parse_buf;
		bmp_leading_byte_offset = ((INT32U) bmp_current_in_addr) & 0x3;		// 0x0 ~ 0x3

		return BMP_PARSE_OK;

	default:
		DBG_PRINT("bmp_decode_parse_header(): Unknown state!\r\n");
	}

	return BMP_PARSE_FAIL;
}

INT32S bmp_parse_bmp_header(void)
{
	switch (bmp_header_state) {
	case STATE_MIGIC_NUMBER:
		if (!bmp_parsed_count) {
			if (*bmp_parse_buf++ != 'B') {
				DBG_PRINT("bmp_parse_bmp_header(): First byte is not 'B'!\r\n");

				return BMP_PARSE_FAIL;
			}
			bmp_parsed_count++;
			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		if (*bmp_parse_buf++ != 'M') {
			DBG_PRINT("bmp_parse_bmp_header(): Second byte is not 'M'!\r\n");

			return BMP_PARSE_FAIL;
		}

		bmp_header_state = STATE_FILE_SIZE;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_FILE_SIZE:
		while (bmp_parsed_count < 4) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		bmp_header_state = STATE_RESERVE;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_RESERVE:
		while (bmp_parsed_count < 4) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		bmp_header_state = STATE_BITMAP_DATA_OFFSET;
		bmp_parsed_count = 0;
		bmp_data_offset = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_BITMAP_DATA_OFFSET:
		while (bmp_parsed_count < 4) {
			bmp_data_offset |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		break;

	default:
		DBG_PRINT("bmp_parse_bmp_header(): Unknown state!\r\n");

		return BMP_PARSE_FAIL;
	}

	return BMP_PARSE_OK;
}

INT32S bmp_parse_dib_format(void)
{
	while (bmp_parsed_count < 4) {
		bmp_dib_header_size |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
		bmp_parse_buf++;
		bmp_parsed_count++;

		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	}

	return BMP_PARSE_OK;
}

INT32S bmp_parse_dib_header(void)
{
	switch (bmp_dib_state) {
	case STATE_BMP_WIDTH:
		while (bmp_parsed_count < 2) {
			bmp_image_width |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		if (bmp_dib_header_size == 40) {
			while (bmp_parsed_count < 4) {
				if (*bmp_parse_buf) {
					DBG_PRINT("bmp_parse_dib_header(): width exceeds maximum!\r\n");

					return BMP_PARSE_FAIL;
				}
				bmp_parse_buf++;
				bmp_parsed_count++;

				if (bmp_parse_buf >= bmp_parse_end) {
					return BMP_PARSE_NOT_DONE;
				}
			}
		}

		bmp_dib_state = STATE_BMP_HEIGHT;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_BMP_HEIGHT:
		while (bmp_parsed_count < 2) {
			bmp_image_height |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		if (bmp_dib_header_size == 40) {
			while (bmp_parsed_count < 4) {
				if (bmp_parsed_count==3 && *bmp_parse_buf) {
					bmp_top_to_bottom = 1;
					bmp_image_height = (INT16U) (-((INT16S) bmp_image_height));
				}
				bmp_parse_buf++;
				bmp_parsed_count++;

				if (bmp_parse_buf >= bmp_parse_end) {
					return BMP_PARSE_NOT_DONE;
				}
			}
		}

		bmp_dib_state = STATE_COLOR_PLANE_NUMBER;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_COLOR_PLANE_NUMBER:
		while (bmp_parsed_count < 2) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		bmp_dib_state = STATE_BITS_PER_PIXEL;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_BITS_PER_PIXEL:
		while (bmp_parsed_count < 2) {
			bmp_bits_per_pixel |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		if (bmp_bits_per_pixel != 24 &&
			bmp_bits_per_pixel != 32 &&
			bmp_bits_per_pixel != 16 &&
			bmp_bits_per_pixel != 8 &&
			bmp_bits_per_pixel != 4 &&
			bmp_bits_per_pixel != 1) {
			DBG_PRINT("bmp_parse_dib_header(): Unsupported bits per pixel number!\r\n");

			return BMP_PARSE_FAIL;
		}

		if (bmp_dib_header_size != 40) {
			if (bmp_bits_per_pixel < 16) {
				bmp_palette_color_number = 1 << bmp_bits_per_pixel;
			}
			break;					// OS/2 V1 DIB header ends here
		}

		bmp_dib_state = STATE_COMPRESSION_METHOD;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_COMPRESSION_METHOD:
		while (bmp_parsed_count < 4) {
			bmp_compression_method |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		if (bmp_compression_method!=0 && bmp_compression_method!=3) {
			DBG_PRINT("bmp_parse_dib_header(): Unsupported compression method!\r\n");

			return BMP_PARSE_FAIL;
		}

		bmp_dib_state = STATE_IMAGE_SIZE;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_IMAGE_SIZE:
		while (bmp_parsed_count < 4) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		bmp_dib_state = STATE_H_RESOLUTION;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_H_RESOLUTION:
		while (bmp_parsed_count < 4) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		bmp_dib_state = STATE_V_RESOLUTION;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_V_RESOLUTION:
		while (bmp_parsed_count < 4) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		bmp_dib_state = STATE_PALETTE_COLOR_NUMBER;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_PALETTE_COLOR_NUMBER:			// 0=2^(bits per pixel)
		while (bmp_parsed_count < 4) {
			bmp_palette_color_number |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		if (bmp_bits_per_pixel<16 && bmp_palette_color_number==0) {
			bmp_palette_color_number = 1 << bmp_bits_per_pixel;
		}
		if (bmp_palette_color_number > 256) {
			DBG_PRINT("parse_dib_os2_v1_header(): palette color number exceeds maximum!\r\n");

			return BMP_PARSE_FAIL;
		}

		bmp_dib_state = STATE_IMPORTANT_COLOR;
		bmp_parsed_count = 0;
		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
	case STATE_IMPORTANT_COLOR:
		while (bmp_parsed_count < 4) {
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}

		if (bmp_parse_buf >= bmp_parse_end) {
			return BMP_PARSE_NOT_DONE;
		}
		break;

	default:
		DBG_PRINT("parse_dib_os2_v1_header(): Unknown state!\r\n");

		return BMP_PARSE_FAIL;
	}

	return BMP_PARSE_OK;
}

INT32S bmp_parse_color_palette(void)
{
	if (bmp_compression_method == 3) {			// BitField
		while (bmp_parsed_count < 4) {
			bmp_bitfield_mask_red |= ((INT32U) *bmp_parse_buf) << (bmp_parsed_count<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		while (bmp_parsed_count < 8) {
			bmp_bitfield_mask_green |= ((INT32U) *bmp_parse_buf) << ((bmp_parsed_count-4)<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
		while (bmp_parsed_count < 12) {
			bmp_bitfield_mask_blue |= ((INT32U) *bmp_parse_buf) << ((bmp_parsed_count-8)<<3);
			bmp_parse_buf++;
			bmp_parsed_count++;

			if (bmp_parse_buf >= bmp_parse_end) {
				return BMP_PARSE_NOT_DONE;
			}
		}
	} else {
		while (bmp_palette_load_number < bmp_palette_color_number) {
			if (!bmp_parsed_count) {				// Blue
				bmp_palette[bmp_palette_load_number] = ((INT16U) (*bmp_parse_buf++) & 0x00F8) >> 3;
				bmp_parsed_count++;

				if (bmp_parse_buf >= bmp_parse_end) {
					return BMP_PARSE_NOT_DONE;
				}
			} else if (bmp_parsed_count == 1) {		// Green
				bmp_palette[bmp_palette_load_number] |= ((INT16U) (*bmp_parse_buf++) & 0x00FC) << 3;
				bmp_parsed_count++;

				if (bmp_parse_buf >= bmp_parse_end) {
					return BMP_PARSE_NOT_DONE;
				}
			} else if (bmp_parsed_count == 2) {		// Red
				bmp_palette[bmp_palette_load_number] |= ((INT16U) (*bmp_parse_buf++) & 0x00F8) << 8;
				if (bmp_dib_header_size == 40) {
					bmp_parsed_count = 3;			// Windows V3 has one padding byte
				} else {
					bmp_parsed_count = 0;
					bmp_palette_load_number++;
				}

				if (bmp_parse_buf >= bmp_parse_end) {
					return BMP_PARSE_NOT_DONE;
				}
			} else {
				bmp_palette_load_number++;

				bmp_parsed_count = 0;
				bmp_parse_buf++;
				if (bmp_parse_buf >= bmp_parse_end) {
					return BMP_PARSE_NOT_DONE;
				}
			}
		}
	}

	return BMP_PARSE_OK;
}

INT16U bmp_decode_image_width_get(void)
{
	return bmp_image_width;
}

INT16U bmp_decode_image_height_get(void)
{
	return bmp_image_height;
}

INT8U bmp_decode_line_direction_get(void)
{
	return bmp_top_to_bottom;
}

INT8U * bmp_decode_data_addr_get(void)
{
	return bmp_current_in_addr;
}

INT32S bmp_decode_output_set(INT32U *output_addr, INT32U fifo_line_num)
{
	bmp_out_start_addr = (INT16U *) output_addr;
	bmp_out_fifo_line_num = fifo_line_num;

	return 0;
}

INT32S bmp_decode_on_the_fly_start(INT8U *input_addr, INT32U len)
{
	if (!input_addr || !len) {
		return -1;
	}

	bmp_current_in_addr = input_addr;
	bmp_current_in_length = len;

	if (bmp_decode_status == C_BMP_STATUS_STOP) {
		INT32U total_pixels;

		if (!bmp_image_width || !bmp_image_height || !bmp_out_start_addr || bmp_out_fifo_line_num>256) {
			bmp_decode_status = C_BMP_STATUS_INIT_ERR;

			return -1;
		}

		bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;
		bmp_current_fifo_ab = 0;
		bmp_current_fifo_line = 0;
		bmp_current_out_addr = bmp_out_start_addr;
		bmp_current_row = 0;
		bmp_current_column = 0;
		bmp_current_byte_index = 0;

		// Counts padding bytes per line
		total_pixels = ((INT32U) bmp_image_width) * ((INT32U) bmp_bits_per_pixel);
		bmp_padding_bytes = (((total_pixels + 0x1F) >> 5) << 2) - ((total_pixels + 0x7) >> 3);
		bmp_current_padding_index = 0;
	}

	if (bmp_bits_per_pixel == 24) {
		return bmp_decode_24bit_data();
	} else if (bmp_compression_method == 3) {		// BitField
		if (bmp_bits_per_pixel == 16) {
			return bmp_decode_16bit_bitfield();
		} else if (bmp_bits_per_pixel == 32) {
			return bmp_decode_32bit_bitfield();
		}
	} else {
		return bmp_decode_3216841bit_data(bmp_bits_per_pixel);
	}
	bmp_decode_status = C_BMP_STATUS_INIT_ERR;

	return -1;
}

// B0 G0 R0 B1 G1 R1 B2 G2 R2 B3 G3 R3 ==> [G0 B0] [R1 G1 B1 R0] [B3 R2 G2 B2] [R3 G3]
INT32S bmp_decode_24bit_data(void)
{
	INT16U max_line_bytes, need_line_bytes;

	if (bmp_image_width<5 || (bmp_leading_byte_offset&0x3)!=0x2) {
		// Read input data one-by-one
		while (bmp_current_row < bmp_image_height) {
			while (bmp_current_column < bmp_image_width) {
				if (bmp_current_byte_index == 0) {			// Blue
					*bmp_current_out_addr = ((INT16U) (*bmp_current_in_addr++) & 0x00F8) >> 3;
					bmp_current_byte_index++;
				} else if (bmp_current_byte_index == 1) {	// Green
					*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr++) & 0x00FC) << 3;
					bmp_current_byte_index++;
				} else {									// Red
					*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr++) & 0x00F8) << 8;
					bmp_current_out_addr++;
					bmp_current_column++;

					bmp_current_byte_index = 0;
				}

				bmp_current_in_length--;
				if (!bmp_current_in_length) {
					bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

					return 0;
				}
			}

			while (bmp_current_padding_index < bmp_padding_bytes) {
				bmp_current_padding_index++;

				bmp_current_in_addr++;
				bmp_current_in_length--;
				if (!bmp_current_in_length) {
					bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

					return 0;
				}
			}
			bmp_current_padding_index = 0;

			if (((INT32U) bmp_current_out_addr) & 0x3) {
				*bmp_current_out_addr = *(bmp_current_out_addr-1);
				bmp_current_out_addr++;
			}

			bmp_current_column = 0;
			bmp_current_row++;

			bmp_current_fifo_line++;
			if (bmp_current_fifo_line == bmp_out_fifo_line_num) {
				bmp_current_fifo_line = 0;

				if (bmp_current_row >= bmp_image_height) {
					bmp_decode_status = C_BMP_STATUS_DECODE_DONE;
				} else {
					bmp_decode_status = C_BMP_STATUS_OUTPUT_FULL;

					if (bmp_current_fifo_ab) {
						bmp_current_fifo_ab = 0;		// Start output to FIFO A
						bmp_current_out_addr = bmp_out_start_addr;
					} else {
						bmp_current_fifo_ab++;			// Start output to FIFO B
					}
				}

				return 0;
			}
		}
		bmp_decode_status = C_BMP_STATUS_DECODE_DONE;

		return 0;
	}


	max_line_bytes = (bmp_image_width<<1) + bmp_image_width;
	need_line_bytes = bmp_image_width - bmp_current_column;
	need_line_bytes += (need_line_bytes<<1);

	while (bmp_current_byte_index) {
		if (bmp_current_byte_index==3 || bmp_current_byte_index==6 || bmp_current_byte_index==9) {
			// Blue
			*bmp_current_out_addr = ((INT16U) (*bmp_current_in_addr) & 0x00F8) >> 3;
		} else if (bmp_current_byte_index==1 || bmp_current_byte_index==4 || bmp_current_byte_index==7 || bmp_current_byte_index==10) {
			// Green
			*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr) & 0x00FC) << 3;
		} else {
			// Red
			*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr) & 0x00F8) << 8;
			bmp_current_out_addr++;
			bmp_current_column++;
			need_line_bytes -= 3;
		}
		bmp_current_byte_index++;
		if (bmp_current_byte_index>=12 || !need_line_bytes) {
			bmp_current_byte_index = 0;
		}

		bmp_current_in_addr++;
		bmp_current_in_length--;
		if (!bmp_current_in_length) {
			bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

			return 0;
		}
	}

	while (bmp_current_row < bmp_image_height) {
		INT32U cnt;

		if (bmp_current_in_length >= need_line_bytes) {
			cnt = need_line_bytes;
		} else {
			cnt = bmp_current_in_length;
		}
		while (cnt >= 12) {
			INT32U in_16bit_data, in_32bit_data;

			// B0 G0 R0 B1 G1 R1 B2 G2 R2 B3 G3 R3 ==> [G0 B0] [R1 G1 B1 R0] [B3 R2 G2 B2] [R3 G3]
			in_16bit_data = *((INT16U *) bmp_current_in_addr);
			in_32bit_data = *((INT32U *) (bmp_current_in_addr+2));
			*((INT32U *) bmp_current_out_addr) = ((in_16bit_data>>3)&0x1F) | ((in_16bit_data>>5)&0x7E0) | ((in_32bit_data<<8)&0xF800) | ((in_32bit_data<<5)&0x1F0000) | ((in_32bit_data<<3)&0x7E00000) | (in_32bit_data&0xF8000000);

			in_32bit_data = *((INT32U *) (bmp_current_in_addr+6));
			in_16bit_data = *((INT16U *) (bmp_current_in_addr+10));
			*((INT32U *) (bmp_current_out_addr+2)) = ((in_32bit_data>>3)&0x1F) | ((in_32bit_data>>5)&0x7E0) | ((in_32bit_data>>8)&0xF800) | ((in_32bit_data>>11)&0x1F0000) | ((in_16bit_data<<19)&0x7E00000) | ((in_16bit_data<<16)&0xF8000000);

			bmp_current_in_addr += 12;
			cnt -= 12;

			bmp_current_out_addr += 4;
			bmp_current_column += 4;
		}
		if (bmp_current_in_length >= need_line_bytes) {
			bmp_current_in_length -= (need_line_bytes - cnt);
			need_line_bytes = cnt;
		} else {
			if (!cnt) {
				bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

				return 0;
			}
			need_line_bytes -= (bmp_current_in_length - cnt);
			bmp_current_in_length = cnt;
		}

		while (cnt) {
			if (bmp_current_byte_index==0 || bmp_current_byte_index==3 || bmp_current_byte_index==6 || bmp_current_byte_index==9) {
				// Blue
				*bmp_current_out_addr = ((INT16U) (*bmp_current_in_addr) & 0x00F8) >> 3;
			} else if (bmp_current_byte_index==1 || bmp_current_byte_index==4 || bmp_current_byte_index==7 || bmp_current_byte_index==10) {
				// Green
				*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr) & 0x00FC) << 3;
			} else {
				// Red
				*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr) & 0x00F8) << 8;
				bmp_current_out_addr++;
				bmp_current_column++;
			}
			bmp_current_byte_index++;

			bmp_current_in_addr++;
			cnt--;
		}
		if (bmp_current_in_length >= need_line_bytes) {
			bmp_current_in_length -= need_line_bytes;
			bmp_current_byte_index = 0;
		} else {
			bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

			return 0;
		}

		cnt = bmp_padding_bytes - bmp_current_padding_index;
		if (bmp_current_in_length < cnt) {
			bmp_current_padding_index += bmp_current_in_length;

			bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

			return 0;
		} else {
			bmp_current_in_addr += cnt;
			bmp_current_in_length -= cnt;
		}
		if (((INT32U) bmp_current_out_addr) & 0x3) {
			*bmp_current_out_addr = *(bmp_current_out_addr-1);
			bmp_current_out_addr++;
		}

		need_line_bytes = max_line_bytes;
		bmp_current_byte_index = 0;
		bmp_current_padding_index = 0;

		bmp_current_column = 0;
		bmp_current_row++;

		bmp_current_fifo_line++;
		if (bmp_current_fifo_line == bmp_out_fifo_line_num) {
			bmp_current_fifo_line = 0;

			if (bmp_current_row >= bmp_image_height) {
				bmp_decode_status = C_BMP_STATUS_DECODE_DONE;
			} else {
				bmp_decode_status = C_BMP_STATUS_OUTPUT_FULL;

				if (bmp_current_fifo_ab) {
					bmp_current_fifo_ab = 0;		// Start output to FIFO A
					bmp_current_out_addr = bmp_out_start_addr;
				} else {
					bmp_current_fifo_ab++;			// Start output to FIFO B
				}
			}

			return 0;
		}
	}
	bmp_decode_status = C_BMP_STATUS_DECODE_DONE;

	return 0;
}

INT32S bmp_decode_3216841bit_data(INT16U pixel)
{
	while (bmp_current_row < bmp_image_height) {
		while (bmp_current_column < bmp_image_width) {
			if (pixel == 32) {
				if (bmp_current_byte_index == 0) {			// Blue
					*bmp_current_out_addr = ((INT16U) (*bmp_current_in_addr++) & 0x00F8) >> 3;
					bmp_current_byte_index++;
				} else if (bmp_current_byte_index == 1) {	// Green
					*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr++) & 0x00FC) << 3;
					bmp_current_byte_index++;
				} else if (bmp_current_byte_index == 2) {	// Red
					*bmp_current_out_addr |= ((INT16U) (*bmp_current_in_addr++) & 0x00F8) << 8;
					bmp_current_out_addr++;
					bmp_current_byte_index++;
					bmp_current_column++;
				} else {
					// Skip one byte in 32-bit pixel mode
					bmp_current_in_addr++;
					bmp_current_byte_index = 0;
				}
			} else if (pixel == 16) {
				INT16U data;

				data = *((INT16U *) bmp_current_in_addr);
				*bmp_current_out_addr++ = ((data<<1) & 0xFFC0) | (data & 0x001F);
				bmp_current_column++;
				bmp_current_in_addr += 2;
				bmp_current_in_length--;
			} else if (pixel == 8) {
				*bmp_current_out_addr++ = bmp_palette[*bmp_current_in_addr++];
				bmp_current_column++;
			} else if (pixel == 4) {
				*bmp_current_out_addr++ = bmp_palette[((*bmp_current_in_addr)>>4) & 0xF];
				bmp_current_column++;

				if (bmp_current_column < bmp_image_width) {
					*bmp_current_out_addr++ = bmp_palette[(*bmp_current_in_addr) & 0xF];
					bmp_current_column++;
				}
				bmp_current_in_addr++;
			} else if (pixel == 1) {
				INT8U i, index;

				index = *bmp_current_in_addr++;
				for (i=0; i<8; i++) {
					*bmp_current_out_addr++ = bmp_palette[(index>>(7-i)) & 0x1];
					bmp_current_column++;
					if (bmp_current_column >= bmp_image_width) {
						break;
					}
				}
			}

			bmp_current_in_length--;
			if (!bmp_current_in_length) {
				bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

				return 0;
			}
		}
		while (bmp_current_padding_index < bmp_padding_bytes) {
			bmp_current_padding_index++;

			bmp_current_in_addr++;
			bmp_current_in_length--;
			if (!bmp_current_in_length) {
				bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

				return 0;
			}
		}
		bmp_current_padding_index = 0;

		if (((INT32U) bmp_current_out_addr) & 0x3) {
			*bmp_current_out_addr = *(bmp_current_out_addr-1);
			bmp_current_out_addr++;
		}

		bmp_current_column = 0;
		bmp_current_row++;

		bmp_current_fifo_line++;
		if (bmp_current_fifo_line == bmp_out_fifo_line_num) {
			bmp_current_fifo_line = 0;

			if (bmp_current_row >= bmp_image_height) {
				bmp_decode_status = C_BMP_STATUS_DECODE_DONE;
			} else {
				bmp_decode_status = C_BMP_STATUS_OUTPUT_FULL;

				if (bmp_current_fifo_ab) {
					bmp_current_fifo_ab = 0;		// Start output to FIFO A
					bmp_current_out_addr = bmp_out_start_addr;
				} else {
					bmp_current_fifo_ab++;			// Start output to FIFO B
				}
			}

			return 0;
		}
	}
	bmp_decode_status = C_BMP_STATUS_DECODE_DONE;

	return 0;
}

INT32S bmp_decode_16bit_bitfield(void)
{
//	if (bmp_bitfield_mask_red!=0xF800 || bmp_bitfield_mask_green!=0x07E0 || bmp_bitfield_mask_blue!=0x001F) {
//		return -1;
//	}
	while (bmp_current_row < bmp_image_height) {
		while (bmp_current_column < bmp_image_width) {
			*bmp_current_out_addr++ =*((INT16U *) bmp_current_in_addr);

			bmp_current_column++;
			bmp_current_in_addr += 2;
			bmp_current_in_length -= 2;
			if (!bmp_current_in_length) {
				bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

				return 0;
			}
		}
		while (bmp_current_padding_index < bmp_padding_bytes) {
			bmp_current_padding_index++;

			bmp_current_in_addr++;
			bmp_current_in_length--;
			if (!bmp_current_in_length) {
				bmp_decode_status = C_BMP_STATUS_INPUT_EMPTY;

				return 0;
			}
		}
		bmp_current_padding_index = 0;

		if (((INT32U) bmp_current_out_addr) & 0x3) {
			*bmp_current_out_addr = *(bmp_current_out_addr-1);
			bmp_current_out_addr++;
		}

		bmp_current_column = 0;
		bmp_current_row++;

		bmp_current_fifo_line++;
		if (bmp_current_fifo_line == bmp_out_fifo_line_num) {
			bmp_current_fifo_line = 0;

			if (bmp_current_row >= bmp_image_height) {
				bmp_decode_status = C_BMP_STATUS_DECODE_DONE;
			} else {
				bmp_decode_status = C_BMP_STATUS_OUTPUT_FULL;

				if (bmp_current_fifo_ab) {
					bmp_current_fifo_ab = 0;		// Start output to FIFO A
					bmp_current_out_addr = bmp_out_start_addr;
				} else {
					bmp_current_fifo_ab++;			// Start output to FIFO B
				}
			}

			return 0;
		}
	}
	bmp_decode_status = C_BMP_STATUS_DECODE_DONE;

	return 0;
}

INT32S bmp_decode_32bit_bitfield(void)
{
	// Never seen this format, treat it as normal 32-bit BMP
	return bmp_decode_3216841bit_data(32);
}

INT32S bmp_decode_output_restart(void)
{
	return bmp_decode_on_the_fly_start(bmp_current_in_addr, bmp_current_in_length);
}

void bmp_decode_stop(void)
{
	bmp_decode_status = C_BMP_STATUS_STOP;
	bmp_image_width = 0;
	bmp_image_height = 0;
}

// Return the current status of BMP engine
INT32S bmp_decode_status_query(void)
{
	return bmp_decode_status;
}

#endif		// GPLIB_BMP_EN
