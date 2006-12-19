#define ICONV_INTERNAL
#include <iconv.h>

static const iconv_ccs_convtable_8bit to_ucs = { {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0x0020, 0x00A0, 0x00E2, 0x007B, 0x00E0, 0x00E1, 0x00E3, 0x00E5,
	0x00E7, 0x00F1, 0x00C4, 0x002E, 0x003C, 0x0028, 0x002B, 0x0021,
	0x0026, 0x00E9, 0x00EA, 0x00EB, 0x00E8, 0x00ED, 0x00EE, 0x00EF,
	0x00EC, 0x007E, 0x00DC, 0x0024, 0x002A, 0x0029, 0x003B, 0x005E,
	0x002D, 0x002F, 0x00C2, 0x005B, 0x00C0, 0x00C1, 0x00C3, 0x00C5,
	0x00C7, 0x00D1, 0x00F6, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F,
	0x00F8, 0x00C9, 0x00CA, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF,
	0x00CC, 0x0060, 0x003A, 0x0023, 0x00A7, 0x0027, 0x003D, 0x0022,
	0x00D8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x00AB, 0x00BB, 0x00F0, 0x00FD, 0x00FE, 0x00B1,
	0x00B0, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070,
	0x0071, 0x0072, 0x00AA, 0x00BA, 0x00E6, 0x00B8, 0x00C6, 0x00A4,
	0x00B5, 0x00DF, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078,
	0x0079, 0x007A, 0x00A1, 0x00BF, 0x00D0, 0x00DD, 0x00DE, 0x00AE,
	0x00A2, 0x00A3, 0x00A5, 0x00B7, 0x00A9, 0x0040, 0x00B6, 0x00BC,
	0x00BD, 0x00BE, 0x00AC, 0x007C, 0x203E, 0x00A8, 0x00B4, 0x00D7,
	0x00E4, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x00AD, 0x00F4, 0x00A6, 0x00F2, 0x00F3, 0x00F5,
	0x00FC, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050,
	0x0051, 0x0052, 0x00B9, 0x00FB, 0x007D, 0x00F9, 0x00FA, 0x00FF,
	0x00D6, 0x00F7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058,
	0x0059, 0x005A, 0x00B2, 0x00D4, 0x005C, 0x00D2, 0x00D3, 0x00D5,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x00B3, 0x00DB, 0x005D, 0x00D9, 0x00DA, 0x007F
} };

static const iconv_ccs_convtable_8bit from_ucs_00 = { {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
	0x0040, 0x004F, 0x007F, 0x007B, 0x005B, 0x006C, 0x0050, 0x007D,
	0x004D, 0x005D, 0x005C, 0x004E, 0x006B, 0x0060, 0x004B, 0x0061,
	0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
	0x00F8, 0x00F9, 0x007A, 0x005E, 0x004C, 0x007E, 0x006E, 0x006F,
	0x00B5, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
	0x00C8, 0x00C9, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6,
	0x00D7, 0x00D8, 0x00D9, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6,
	0x00E7, 0x00E8, 0x00E9, 0x0063, 0x00EC, 0x00FC, 0x005F, 0x006D,
	0x0079, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
	0x0088, 0x0089, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096,
	0x0097, 0x0098, 0x0099, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6,
	0x00A7, 0x00A8, 0x00A9, 0x0043, 0x00BB, 0x00DC, 0x0059, 0x00FF,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0x0041, 0x00AA, 0x00B0, 0x00B1, 0x009F, 0x00B2, 0x00CC, 0x007C,
	0x00BD, 0x00B4, 0x009A, 0x008A, 0x00BA, 0x00CA, 0x00AF, 0xFFFE,
	0x0090, 0x008F, 0x00EA, 0x00FA, 0x00BE, 0x00A0, 0x00B6, 0x00B3,
	0x009D, 0x00DA, 0x009B, 0x008B, 0x00B7, 0x00B8, 0x00B9, 0x00AB,
	0x0064, 0x0065, 0x0062, 0x0066, 0x004A, 0x0067, 0x009E, 0x0068,
	0x0074, 0x0071, 0x0072, 0x0073, 0x0078, 0x0075, 0x0076, 0x0077,
	0x00AC, 0x0069, 0x00ED, 0x00EE, 0x00EB, 0x00EF, 0x00E0, 0x00BF,
	0x0080, 0x00FD, 0x00FE, 0x00FB, 0x005A, 0x00AD, 0x00AE, 0x00A1,
	0x0044, 0x0045, 0x0042, 0x0046, 0x00C0, 0x0047, 0x009C, 0x0048,
	0x0054, 0x0051, 0x0052, 0x0053, 0x0058, 0x0055, 0x0056, 0x0057,
	0x008C, 0x0049, 0x00CD, 0x00CE, 0x00CB, 0x00CF, 0x006A, 0x00E1,
	0x0070, 0x00DD, 0x00DE, 0x00DB, 0x00D0, 0x008D, 0x008E, 0x00DF
} };

static const iconv_ccs_convtable_8bit from_ucs_20 = { {
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0x00BC, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE
} };

static const iconv_ccs_convtable_16bit from_ucs = { {
	&from_ucs_00, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	&from_ucs_20, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
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
	"cp273", NULL
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
