#include "utils.h"

static const uint8_t CRC8_TABLE[] =
{
		0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

/**
 * @brief  Compute a Dallas Semiconductor 8 bit CRC
 * @param  Addres to data, length of data
 * @return CRC8
 */
uint8_t CRC8(const void *data, size_t length)
{
	/* init start hash-sum value */
	uint8_t m_crc = 0;
	const uint8_t *m_data = (uint8_t *)data;

	while (length--)
	{
		//--------------------------------------------------------------------------
		#if defined (CRC8_FAST_CALCULATION) /* Compute with table (fast) */
		//--------------------------------------------------------------------------
			m_crc = CRC8_TABLE[m_crc ^ *m_data++];
		//--------------------------------------------------------------------------
		#elif defined (CRC8_SLOW_CALCULATION) /* Compute directly (slow) */
		//--------------------------------------------------------------------------
			uint8_t inbyte = *m_data++;

			for (size_t i = 0; i < 8; ++i)
			{
				uint8_t mix = (m_crc ^ inbyte) & 0x01;

				m_crc >>= 1;

				if (mix)
					m_crc ^= 0x8C; // 0x8C = 0x80|(CRC8_POLYNOM>>1), CRC8_POLYNOM = X^8+X^5+X^4+X^0 = 0x18

				inbyte >>= 1;
			}
		//--------------------------------------------------------------------------
		#endif /* end of type calculation */
		//--------------------------------------------------------------------------
	}

	/* return final hash-sum value */
	return m_crc;
}

char *parse_args(int argc, char **argv, char *optstring)
{
	int opt;
	char *interface = NULL;

	while ((opt = getopt(argc, argv, optstring)) != -1)
	{
		switch (opt)
		{
		case 'i':
			interface = interface;
			break;

		default:
			fprintf(stderr, "Usage: %s -i <interface>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	return interface;
}

void print_packet(kermit_frame_t *m) {
    if (!m)
        return;

    printf("Starter mark: %x\n", m->starter_mark);
    printf("Size: %d\n", m->size);
    printf("Sequence number: %d\n", m->seq_num);
    printf("Type: %d\n", m->type);
    printf("Data: %s\n", m->data);
    printf("CRC: %x\n", m->crc);
}
