/*
 * Coded EBCDIC Character Set OSD_EBCDIC_DF04_1
 * as used on Fujitsu-Siemens mainframes
 * running the BS2000 operating system
 * (See http://www.fujitsu-siemens.com/rl/products/bs2000/)
 *
 * This EBCDIC character set defines a 1:1 (bijective) mapping
 * with ISO-8859-1
 *
 * Author: Martin Kraemer  <martin.at.apache.org>
 */
#define ICONV_INTERNAL
#include <iconv.h>

static const iconv_ccs_convtable_8bit to_ucs = { {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0085, 0x0009, 0x0086, 0x007f,
	0x0087, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x008f, 0x000a, 0x0008, 0x0097,
	0x0018, 0x0019, 0x009c, 0x009d, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0092, 0x0017, 0x001b,
	0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007,
	0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004,
	0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a,
	0x0020, 0x00a0, 0x00e2, 0x00e4, 0x00e0, 0x00e1, 0x00e3, 0x00e5,
	0x00e7, 0x00f1, 0x0060, 0x002e, 0x003c, 0x0028, 0x002b, 0x007c,
	0x0026, 0x00e9, 0x00ea, 0x00eb, 0x00e8, 0x00ed, 0x00ee, 0x00ef,
	0x00ec, 0x00df, 0x0021, 0x0024, 0x002a, 0x0029, 0x003b, 0x009f,
	0x002d, 0x002f, 0x00c2, 0x00c4, 0x00c0, 0x00c1, 0x00c3, 0x00c5,
	0x00c7, 0x00d1, 0x005e, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f,
	0x00f8, 0x00c9, 0x00ca, 0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf,
	0x00cc, 0x00a8, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022,
	0x00d8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x00ab, 0x00bb, 0x00f0, 0x00fd, 0x00fe, 0x00b1,
	0x00b0, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070,
	0x0071, 0x0072, 0x00aa, 0x00ba, 0x00e6, 0x00b8, 0x00c6, 0x00a4,
	0x00b5, 0x00af, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078,
	0x0079, 0x007a, 0x00a1, 0x00bf, 0x00d0, 0x00dd, 0x00de, 0x00ae,
	0x00a2, 0x00a3, 0x00a5, 0x00b7, 0x00a9, 0x00a7, 0x00b6, 0x00bc,
	0x00bd, 0x00be, 0x00ac, 0x005b, 0x005c, 0x005d, 0x00b4, 0x00d7,
	0x00f9, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x00ad, 0x00f4, 0x00f6, 0x00f2, 0x00f3, 0x00f5,
	0x00a6, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050,
	0x0051, 0x0052, 0x00b9, 0x00fb, 0x00fc, 0x00db, 0x00fa, 0x00ff,
	0x00d9, 0x00f7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058,
	0x0059, 0x005a, 0x00b2, 0x00d4, 0x00d6, 0x00d2, 0x00d3, 0x00d5,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x00b3, 0x007b, 0x00dc, 0x007d, 0x00da, 0x007e
} };

static const iconv_ccs_convtable_8bit from_ucs_00 = { {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0037, 0x002d, 0x002e, 0x002f,
	0x0016, 0x0005, 0x0015, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x003c, 0x003d, 0x0032, 0x0026,
	0x0018, 0x0019, 0x003f, 0x0027, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0040, 0x005a, 0x007f, 0x007b, 0x005b, 0x006c, 0x0050, 0x007d,
	0x004d, 0x005d, 0x005c, 0x004e, 0x006b, 0x0060, 0x004b, 0x0061,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
	0x00f8, 0x00f9, 0x007a, 0x005e, 0x004c, 0x007e, 0x006e, 0x006f,
	0x007c, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
	0x00c8, 0x00c9, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6,
	0x00d7, 0x00d8, 0x00d9, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6,
	0x00e7, 0x00e8, 0x00e9, 0x00bb, 0x00bc, 0x00bd, 0x006a, 0x006d,
	0x004a, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
	0x0088, 0x0089, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096,
	0x0097, 0x0098, 0x0099, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6,
	0x00a7, 0x00a8, 0x00a9, 0x00fb, 0x004f, 0x00fd, 0x00ff, 0x0007,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0004, 0x0006, 0x0008,
	0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x0009, 0x000a, 0x0014,
	0x0030, 0x0031, 0x0025, 0x0033, 0x0034, 0x0035, 0x0036, 0x0017,
	0x0038, 0x0039, 0x003a, 0x003b, 0x001a, 0x001b, 0x003e, 0x005f,
	0x0041, 0x00aa, 0x00b0, 0x00b1, 0x009f, 0x00b2, 0x00d0, 0x00b5,
	0x0079, 0x00b4, 0x009a, 0x008a, 0x00ba, 0x00ca, 0x00af, 0x00a1,
	0x0090, 0x008f, 0x00ea, 0x00fa, 0x00be, 0x00a0, 0x00b6, 0x00b3,
	0x009d, 0x00da, 0x009b, 0x008b, 0x00b7, 0x00b8, 0x00b9, 0x00ab,
	0x0064, 0x0065, 0x0062, 0x0066, 0x0063, 0x0067, 0x009e, 0x0068,
	0x0074, 0x0071, 0x0072, 0x0073, 0x0078, 0x0075, 0x0076, 0x0077,
	0x00ac, 0x0069, 0x00ed, 0x00ee, 0x00eb, 0x00ef, 0x00ec, 0x00bf,
	0x0080, 0x00e0, 0x00fe, 0x00dd, 0x00fc, 0x00ad, 0x00ae, 0x0059,
	0x0044, 0x0045, 0x0042, 0x0046, 0x0043, 0x0047, 0x009c, 0x0048,
	0x0054, 0x0051, 0x0052, 0x0053, 0x0058, 0x0055, 0x0056, 0x0057,
	0x008c, 0x0049, 0x00cd, 0x00ce, 0x00cb, 0x00cf, 0x00cc, 0x00e1,
	0x0070, 0x00c0, 0x00de, 0x00db, 0x00dc, 0x008d, 0x008e, 0x00df
} };

static const iconv_ccs_convtable_16bit from_ucs = { {
	&from_ucs_00, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
} };

#define NBITS 8

static ucs2_t convert_from_ucs(ucs2_t ch)
{
	return iconv_ccs_convert_16bit((const iconv_ccs_convtable *)&from_ucs, ch);
}

static ucs2_t convert_to_ucs(ucs2_t ch)
{
	return iconv_ccs_convert_8bit((const iconv_ccs_convtable *)&to_ucs, ch);
}

static const char * const names[] = {
	"osd_ebcdic_df04_1", NULL
};

static const struct iconv_ccs_desc iconv_ccs_desc = {
	names, NBITS,
	(const iconv_ccs_convtable *)&from_ucs,
	(const iconv_ccs_convtable *)&to_ucs,
	convert_from_ucs, convert_to_ucs,
};

struct iconv_module_desc iconv_module = {
	ICMOD_UC_CCS,
	apr_iconv_mod_noevent,
	NULL,
	&iconv_ccs_desc
};
