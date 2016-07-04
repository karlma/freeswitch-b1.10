/*
 * @brief   Estimation of amplitude using DESA-2 algorithm.
 *
 * Contributor(s):
 *
 * Eric des Courtis <eric.des.courtis@benbria.com>
 * Piotr Gregor <piotrek.gregor gmail.com>:
 */


#ifndef __AVMD_AMPLITUDE_H__
#define __AVMD_AMPLITUDE_H__


#include "avmd_buffer.h"


double avmd_amplitude(circ_buffer_t *, size_t i, double f) __attribute__ ((nonnull(1)));


#endif /* __AVMD_AMPLITUDE_H__ */
