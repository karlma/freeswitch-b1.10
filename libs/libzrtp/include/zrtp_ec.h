/*
 * zrtp_crypto_ec.c - Elliptic Curve Low Level Crypto functions for ZRTP.
 * NSA Suite B Elliptic Curves from NIST SP 800-56A and FIPS 186-3.
 *
 * Copyright (c) 2006-2009 Philip R. Zimmermann. All rights reserved.
 * This is NOT licensed under the GPL or any other open source license.
 * For licensing terms or other information,
 * contact:  Philip Zimmermann <prz@mit.edu>.
 * For more contact information, see http://philzimmermann.com
 */
 
#ifndef __ZRTP_CRYPTO_EC_H__
#define __ZRTP_CRYPTO_EC_H__

#include "bn.h"

#include "zrtp_config.h"
#include "zrtp_types.h"
#include "zrtp_error.h"

#if (defined(ZRTP_ENABLE_EC) && (ZRTP_ENABLE_EC == 1))

#define ZRTP_MAXECBITS	521
#define ZRTP_MAXECWORDS	((ZRTP_MAXECBITS+7)/8)

typedef struct zrtp_ec_params
{
	unsigned		ec_bits;						/* # EC bits: 256, 384, 521 */
	uint8_t			P_data[ZRTP_MAXECWORDS];		/* curve field prime */
	uint8_t			n_data[ZRTP_MAXECWORDS];		/* curve order (# points) */
	uint8_t			b_data[ZRTP_MAXECWORDS];		/* curve param, y^3 = x^2 -3x + b */
	uint8_t			Gx_data[ZRTP_MAXECWORDS];		/* curve point, x coordinate */
	uint8_t			Gy_data[ZRTP_MAXECWORDS];		/* curve point, y coordinate */
} zrtp_ec_params_t;

#if defined(__cplusplus)
extern "C"
{
#endif 
 
/*============================================================================*/
/* 	  Elliptic Curve library                 		      					  */
/*============================================================================*/

int zrtp_ecAdd ( struct BigNum *rsltx,
				 struct BigNum *rslty,
				 struct BigNum *p1x,
				 struct BigNum *p1y,
				 struct BigNum *p2x,
				 struct BigNum *p2y,
				 struct BigNum *mod);

int zrtp_ecMul ( struct BigNum *rsltx,
				 struct BigNum *rslty,
				 struct BigNum *mult,
				 struct BigNum *basex,
				 struct BigNum *basey,
				 struct BigNum *mod);

zrtp_status_t zrtp_ec_random_point( zrtp_global_t *zrtp,
									struct BigNum *P,
									struct BigNum *n,
									struct BigNum *Gx,
									struct BigNum *Gy,
									struct BigNum *pkx,
									struct BigNum *pky,
									struct BigNum *sv,
									uint8_t *test_sv_data,
									size_t test_sv_data_len);

extern zrtp_status_t zrtp_ec_init_params(struct zrtp_ec_params *params, uint32_t bits );


/* Useful bignum utility functions not defined in bignum library */
int bnAddMod_ (struct BigNum *rslt, struct BigNum *n1, struct BigNum *mod);
int bnAddQMod_ (struct BigNum *rslt, unsigned n1, struct BigNum *mod);
int bnSubMod_ (struct BigNum *rslt, struct BigNum *n1, struct BigNum *mod);
int bnSubQMod_ (struct BigNum *rslt, unsigned n1, struct BigNum *mod);
int bnMulMod_ (struct BigNum *rslt, struct BigNum *n1, struct BigNum *n2, struct BigNum *mod);
int bnMulQMod_ (struct BigNum *rslt, struct BigNum *n1, unsigned n2, struct BigNum *mod);
int bnSquareMod_ (struct BigNum *rslt, struct BigNum *n1, struct BigNum *mod);

#if defined(__cplusplus)
}
#endif

#endif /*ZRTP_ENABLE_EC*/

#endif /* __ZRTP_CRYPTO_EC_H__ */
