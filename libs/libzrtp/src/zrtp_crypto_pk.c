/*
 * libZRTP SDK library, implements the ZRTP secure VoIP protocol.
 * Copyright (c) 2006-2009 Philip R. Zimmermann.  All rights reserved.
 * Contact: http://philzimmermann.com
 * For licensing and other legal details, see the file zrtp_legal.c.
 * 
 * Viktor Krykun <v.krikun at zfoneproject.com> 
 */

#include "zrtp.h"

#define _ZTU_ "zrtp dh"

/*============================================================================*/
/*    Global DH Functions													  */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
static zrtp_status_t zrtp_dh_init(void *s)
{
	struct BigNum* p = NULL;
	struct BigNum* p_1 = NULL;
	uint8_t* p_data = NULL;
	unsigned int p_data_length = 0;
	zrtp_pk_scheme_t *self = (zrtp_pk_scheme_t *) s;
	
	switch (self->base.id) {
		case ZRTP_PKTYPE_DH2048:
			p = &self->base.zrtp->P_2048;
			p_1 = &self->base.zrtp->P_2048_1;
			p_data = self->base.zrtp->P_2048_data;
			p_data_length = sizeof(self->base.zrtp->P_2048_data);
			break;
		case ZRTP_PKTYPE_DH3072:
			p = &self->base.zrtp->P_3072;
			p_1 = &self->base.zrtp->P_3072_1;
			p_data = self->base.zrtp->P_3072_data;
			p_data_length = sizeof(self->base.zrtp->P_3072_data);
			break;
		default:
			return zrtp_status_bad_param;
	}
	
    bnBegin(p);
    bnInsertBigBytes(p, (const unsigned char *)p_data, 0, p_data_length);
		
    bnBegin(p_1);
    bnCopy(p_1, p);
    bnSub(p_1, &self->base.zrtp->one);
    
    return zrtp_status_ok;
}

/*----------------------------------------------------------------------------*/
static zrtp_status_t zrtp_dh_free(void *s)
{
	zrtp_pk_scheme_t *self = (zrtp_pk_scheme_t *) s;
	switch (self->base.id) {
		case ZRTP_PKTYPE_DH2048:
			bnEnd(&self->base.zrtp->P_2048);
			bnEnd(&self->base.zrtp->P_2048_1);
			break;
		case ZRTP_PKTYPE_DH3072:
			bnEnd(&self->base.zrtp->P_3072);
			bnEnd(&self->base.zrtp->P_3072_1);
			break;
		default:
			return zrtp_status_bad_param;
	}
	
    return zrtp_status_ok;
}

/*----------------------------------------------------------------------------*/
static struct BigNum* _zrtp_get_p(zrtp_pk_scheme_t *self)
{
	struct BigNum* p = NULL;
	switch (self->base.id) {
		case ZRTP_PKTYPE_DH2048:
			p = &self->base.zrtp->P_2048;
			break;
		case ZRTP_PKTYPE_DH3072:
			p = &self->base.zrtp->P_3072;
			break;
		default:
			break;
	}
	
	return p;
}

static zrtp_status_t zrtp_dh_initialize( zrtp_pk_scheme_t *self,
										 zrtp_dh_crypto_context_t *dh_cc)
{
	unsigned char* buffer = zrtp_sys_alloc(sizeof(zrtp_uchar128_t));
	struct BigNum* p = _zrtp_get_p(self);
	zrtp_time_t start_ts = zrtp_time_now();
	
	ZRTP_LOG(1,(_ZTU_,"\tDH TEST: %.4s zrtp_dh_initialize() START. now=%llums.\n", self->base.type, start_ts));
	
	if (!buffer) {
		return zrtp_status_alloc_fail;
	}
	if (!p) {
		zrtp_sys_free(buffer);
		return zrtp_status_bad_param;
	}
	
	if (64 != zrtp_randstr(self->base.zrtp, buffer, 64)) {
		zrtp_sys_free(buffer);
		return zrtp_status_rng_fail;
	}

	bnBegin(&dh_cc->sv);
	bnInsertBigBytes(&dh_cc->sv, (const unsigned char *)buffer, 0, self->sv_length);
	bnBegin(&dh_cc->pv);
	bnExpMod(&dh_cc->pv, &self->base.zrtp->G, &dh_cc->sv, p);
	
	zrtp_sys_free(buffer);
	
	ZRTP_LOG(1,(_ZTU_,"\tDH TEST: zrtp_dh_initialize() for %.4s was executed ts=%llums d=%llums.\n", self->base.type, zrtp_time_now(), zrtp_time_now()-start_ts));
	return zrtp_status_ok;
}

/*----------------------------------------------------------------------------*/
static zrtp_status_t zrtp_dh_compute( zrtp_pk_scheme_t *self,					      
									  zrtp_dh_crypto_context_t *dh_cc,
									  struct BigNum *dhresult,
									  struct BigNum *pv)
{
	struct BigNum* p = _zrtp_get_p(self);
	zrtp_time_t start_ts = zrtp_time_now();
	if (!p) {		
		return zrtp_status_bad_param;
	}
	
	ZRTP_LOG(1,(_ZTU_,"\tDH TEST: %.4s zrtp_dh_compute() START. now=%llums.\n", self->base.type, start_ts));
	
    bnExpMod(dhresult, pv, &dh_cc->sv, p);
	ZRTP_LOG(1,(_ZTU_,"\tDH TEST: zrtp_dh_compute() for %.4s was executed ts=%llums d=%llums.\n", self->base.type, zrtp_time_now(), zrtp_time_now()-start_ts));
    return zrtp_status_ok;
}

/*----------------------------------------------------------------------------*/
static zrtp_status_t zrtp_dh_validate(zrtp_pk_scheme_t *self, struct BigNum *pv)
{
	struct BigNum* p = _zrtp_get_p(self);
	if (!p) {		
		return zrtp_status_bad_param;
	}
	
    if (!pv || 0 == bnCmp(pv, &self->base.zrtp->one) || 0 == bnCmp(pv, p)) {
    	return zrtp_status_fail;
    } else {    
		return zrtp_status_ok;
	}
}

/*----------------------------------------------------------------------------*/
static zrtp_status_t zrtp_dh_self_test(zrtp_pk_scheme_t *self)
{
	zrtp_status_t s = zrtp_status_ok;
	zrtp_dh_crypto_context_t alice_cc;
	zrtp_dh_crypto_context_t bob_cc;
	struct BigNum alice_k;
	struct BigNum bob_k;
	zrtp_time_t start_ts = zrtp_time_now();
	
	ZRTP_LOG(3, (_ZTU_, "PKS %.4s testing... ", self->base.type));
	
	bnBegin(&alice_k);
	bnBegin(&bob_k);
	
	do {	
		/* Both sides initalise DH schemes and compute secret and public values. */
		s = self->initialize(self, &alice_cc);
		if (zrtp_status_ok != s) {
			break;
		}
		s = self->initialize(self, &bob_cc);
		if (zrtp_status_ok != s) {
			break;
		}
		
		/* Both sides validate public values. (to provide exact performance estimation) */
		s = self->validate(self, &bob_cc.pv);
		if (zrtp_status_ok != s) {
			break;
		}
		s = self->validate(self, &alice_cc.pv);
		if (zrtp_status_ok != s) {
			break;
		}
		
		/* Compute secret keys and compare them. */
		s = self->compute(self, &alice_cc, &alice_k, &bob_cc.pv);
		if (zrtp_status_ok != s) {
			break;
		}
		s= self->compute(self, &bob_cc, &bob_k, &alice_cc.pv);
		if (zrtp_status_ok != s) {
			break;
		}
				
		s = (0 == bnCmp(&alice_k, &bob_k)) ? zrtp_status_ok : zrtp_status_algo_fail;
	} while (0);

	bnEnd(&alice_k);
	bnEnd(&bob_k);
		
	ZRTP_LOGC(3, ("%s (%llu ms)\n", zrtp_log_status2str(s), (zrtp_time_now()-start_ts)/2));
	
	return s;
}

/*----------------------------------------------------------------------------*/
extern zrtp_status_t zrtp_defaults_ec_pkt(zrtp_global_t* zrtp);

zrtp_status_t zrtp_defaults_pkt(zrtp_global_t* zrtp)
{
	zrtp_pk_scheme_t* presh  = zrtp_sys_alloc(sizeof(zrtp_pk_scheme_t));
	zrtp_pk_scheme_t* dh2048 = zrtp_sys_alloc(sizeof(zrtp_pk_scheme_t));
    zrtp_pk_scheme_t* dh3072 = zrtp_sys_alloc(sizeof(zrtp_pk_scheme_t));
	zrtp_pk_scheme_t* multi  = zrtp_sys_alloc(sizeof(zrtp_pk_scheme_t));
    
	uint8_t P_2048_data[] =
	{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
	0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
	0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
	0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
	0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
	0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
	0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
	0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D, 0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
	0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
	0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
	0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
	0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
	0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
	0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
	0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
	0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
	
    uint8_t P_3072_data[] =
    {		
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
	0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
	0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
	0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
	0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
	0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
	0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
	0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D, 0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
	0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
	0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
	0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
	0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
	0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
	0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
	0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
	0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33, 0x17, 0x0D, 0x04, 0x50, 0x7A, 0x33,
	0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64, 0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB, 0xEF, 0x0A,
	0x8A, 0xEA, 0x71, 0x57, 0x5D, 0x06, 0x0C, 0x7D, 0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7,
	0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0, 0x4A, 0x25, 0x61, 0x9D,
	0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B, 0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64,
	0xD8, 0x76, 0x02, 0x73, 0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F, 0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C,
	0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C, 0x77, 0x09, 0x88, 0xC0, 0xBA, 0xD9, 0x46, 0xE2,
	0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31, 0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E,
	0x4B, 0x82, 0xD1, 0x20, 0xA9, 0x3A, 0xD2, 0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

	if (!dh2048 || !dh3072 || !presh || !multi)	{
		if (presh) {
			zrtp_sys_free(presh);
		}
		if (dh2048) {
			zrtp_sys_free(dh2048);
		}
		if (dh3072) {
			zrtp_sys_free(dh3072);
		}
		if (multi) {
			zrtp_sys_free(multi);
		}
		return zrtp_status_alloc_fail;
	}
	    
    zrtp_memset(dh3072, 0, sizeof(zrtp_pk_scheme_t));
    zrtp_memcpy(dh3072->base.type, ZRTP_DH3K, ZRTP_COMP_TYPE_SIZE);
	dh3072->base.id		= ZRTP_PKTYPE_DH3072;
    dh3072->base.zrtp	= zrtp;
    dh3072->sv_length	= 256/8;
    dh3072->pv_length	= 384;
    dh3072->base.init 	= zrtp_dh_init;
    dh3072->base.free	= zrtp_dh_free;
    dh3072->initialize	= zrtp_dh_initialize;
    dh3072->compute		= zrtp_dh_compute;
    dh3072->validate	= zrtp_dh_validate;
	dh3072->self_test	= zrtp_dh_self_test;
	zrtp_memcpy(zrtp->P_3072_data, P_3072_data, sizeof(P_3072_data));
	zrtp_comp_register(ZRTP_CC_PKT, dh3072, zrtp);
		
	zrtp_memset(dh2048, 0, sizeof(zrtp_pk_scheme_t));
	zrtp_memcpy(dh2048->base.type, ZRTP_DH2K, ZRTP_COMP_TYPE_SIZE);
	dh2048->base.id		= ZRTP_PKTYPE_DH2048;
	dh2048->base.zrtp	= zrtp;
	dh2048->sv_length	= 256/8;
	dh2048->pv_length	= 256;
	dh2048->base.init 	= zrtp_dh_init;
	dh2048->base.free	= zrtp_dh_free;
	dh2048->initialize	= zrtp_dh_initialize;
	dh2048->compute		= zrtp_dh_compute;
	dh2048->validate	= zrtp_dh_validate;
	dh2048->self_test	= zrtp_dh_self_test;
	zrtp_memcpy(zrtp->P_2048_data, P_2048_data, sizeof(P_2048_data));
	zrtp_comp_register(ZRTP_CC_PKT, dh2048, zrtp);

	zrtp_memset(multi, 0, sizeof(zrtp_pk_scheme_t));
	zrtp_memcpy(multi->base.type, ZRTP_MULT, ZRTP_COMP_TYPE_SIZE);
	multi->base.id				= ZRTP_PKTYPE_MULT;
	zrtp_comp_register(ZRTP_CC_PKT, multi,  zrtp);

	zrtp_memset(presh, 0, sizeof(zrtp_pk_scheme_t));
	zrtp_memcpy(presh->base.type, ZRTP_PRESHARED, ZRTP_COMP_TYPE_SIZE);
	presh->base.id				= ZRTP_PKTYPE_PRESH;
	zrtp_comp_register(ZRTP_CC_PKT, presh,  zrtp);	
    
		return zrtp_defaults_ec_pkt(zrtp);
}

/*----------------------------------------------------------------------------*/
zrtp_status_t zrtp_prepare_pkt(zrtp_global_t* zrtp)
{
    bnInit();
    bnBegin(&zrtp->one);
    bnSetQ(&zrtp->one, 1);
    bnBegin(&zrtp->G);
    bnSetQ(&zrtp->G, 2);

    return zrtp_status_ok;
}

zrtp_status_t zrtp_done_pkt(zrtp_global_t* zrtp)
{
    bnEnd(&zrtp->one);
    bnEnd(&zrtp->G);
    
    return zrtp_status_ok;
}
