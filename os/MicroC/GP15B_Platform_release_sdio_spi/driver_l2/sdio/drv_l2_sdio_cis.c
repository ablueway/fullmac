/*
* Purpose: Driver layer2 for controlling SDIO cards CIS
*
* Author: Dunker Chen
*
* Date: 2013/10/31
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.0
*/

#include <string.h>
#include "gplib_mm_gplus.h"
#include "drv_l1_sdc.h"
#include "driver_l2_cfg.h"
#include "drv_l2_sdio.h"


#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
**************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
**************************************************************************/

typedef int (tpl_parse_t)( gpSDIOInfo_t *sdio, struct sdio_func *,
			   const INT8U *, INT32U);

struct cis_tpl {
	INT8U code;
	INT8U min_size;
	tpl_parse_t *parse;
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

extern void print_string(CHAR *fmt, ...);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
**************************************************************************/

static INT32S cistpl_vers_1(gpSDIOInfo_t *sdio, struct sdio_func *func, const INT8U *buf, INT32U size);
static INT32S cistpl_manfid(gpSDIOInfo_t *sdio, struct sdio_func *func, const INT8U *buf, INT32U size);
static INT32S cistpl_funce(gpSDIOInfo_t *sdio, struct sdio_func *func, const INT8U *buf, INT32U size);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
**************************************************************************/

static const struct cis_tpl cis_tpl_list[4] = {
	{	0x15,	3,	cistpl_vers_1		},
	{	0x20,	4,	cistpl_manfid		},
	{	0x21,	2,	/* cistpl_funcid */	},
	{	0x22,	0,	cistpl_funce		},
};

static const INT8U speed_val[16] =
	{ 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };

static const INT32U speed_unit[8] =
	{ 10000, 100000, 1000000, 10000000, 0, 0, 0, 0 };

/* FUNCE tuples with these types get passed to SDIO drivers */
static const INT8U funce_type_whitelist[1] = {
	4 /* CISTPL_FUNCE_LAN_NODE_ID used in Broadcom cards */
};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/

/**
* @brief 	Parse level 1 version/production-information.
* @param 	sdio[in]: SDIO handle.
* @param 	func[in]: SDIO fuction struct pointer.
* @param 	buf[in]: Buffer pointer.
* @param 	size[in]: Buffer size.
* @return 	0: success, -1: fail.
*/
static INT32S cistpl_vers_1(
	gpSDIOInfo_t *sdio,
	struct sdio_func *func,
	const INT8U *buf,
	INT32U size)
{
	unsigned i, nr_strings;
	char **buffer, *string;

	/* Find all null-terminated (including zero length) strings in
	   the TPLLV1_INFO field. Trailing garbage is ignored. */
	buf += 2;
	size -= 2;

	nr_strings = 0;
	for (i = 0; i < size; i++)
	{
		if (buf[i] == 0xff)
			break;
		if (buf[i] == 0)
			nr_strings++;
	}
	if (nr_strings == 0)
		return 0;

	size = i;

	buffer = (char **)gp_malloc_align(sizeof(char*) * nr_strings + size, 4);
	if (!buffer)
		return -1;
	memset( buffer, 0, sizeof(char*) * nr_strings + size );

	string = (char*)(buffer + nr_strings);

	for (i = 0; i < nr_strings; i++)
	{
		buffer[i] = string;
		strcpy(string, (const char *) buf);
		string += strlen(string) + 1;
		buf += strlen((const char *)buf) + 1;
	}

	if (func)
	{
		func->num_info = nr_strings;
		func->info = (const char**)buffer;
	}
	else
	{
		sdio->num_info = nr_strings;
		sdio->info = (const char**)buffer;
	}

	return 0;
}

/**
* @brief 	Parse manufacturer identification.
* @param 	sdio[in]: SDIO handle.
* @param 	func[in]: SDIO fuction struct pointer.
* @param 	buf[in]: Buffer pointer.
* @param 	size[in]: Buffer size.
* @return 	0: success, -1: fail.
*/
static INT32S cistpl_manfid(
	gpSDIOInfo_t *sdio,
	struct sdio_func *func,
	const INT8U *buf,
	INT32U size)
{
	unsigned int vendor, device;

	/* TPLMID_MANF */
	vendor = buf[0] | (buf[1] << 8);

	/* TPLMID_CARD */
	device = buf[2] | (buf[3] << 8);

	if (func) {
		func->vendor = vendor;
		func->device = device;
	} else {
		sdio->cis.vendor = vendor;
		sdio->cis.device = device;
	}

	return 0;
}

/**
* @brief 	Check function extension is in white list or not.
* @param 	type[in]: Tuple type.
* @return 	1: in white lsit, 0: not in white list.
*/
static INT32S cistpl_funce_whitelisted(
	INT8U type)
{
	int i;

	for (i = 0; i < sizeof(funce_type_whitelist); i++) {
		if (funce_type_whitelist[i] == type)
			return 1;
	}
	return 0;
}

/**
* @brief 	Parse common function extensions tuple.
* @param 	sdio[in]: SDIO handle.
* @param 	buf[in]: Buffer pointer.
* @param 	size[in]: Buffer size.
* @return 	0: success, -1: fail.
*/
static INT32S cistpl_funce_common(
	gpSDIOInfo_t *sdio,
	const INT8U *buf,
	INT32U size)
{
	if (size < 0x04 || buf[0] != 0)
		return -1;

	/* TPLFE_FN0_BLK_SIZE */
	sdio->cis.blksize = buf[1] | (buf[2] << 8);

	/* TPLFE_MAX_TRAN_SPEED */
	sdio->cis.max_dtr = speed_val[(buf[3] >> 3) & 15] * speed_unit[buf[3] & 7];

	return 0;
}

/**
* @brief 	Parse function extensions tuple.
* @param 	sdio[in]: SDIO handle.
* @param 	buf[in]: Buffer pointer.
* @param 	size[in]: Buffer size.
* @return 	0: success, -1: fail.
*/
static INT32S cistpl_funce_func(
	struct sdio_func *func,
	const INT8U *buf,
	INT32U size)
{
	unsigned vsn;
	unsigned min_size;

	/* let SDIO drivers take care of whitelisted FUNCE tuples */
	if (cistpl_funce_whitelisted(buf[0]))
		return -2;

	vsn = func->sdio->cccr.sdio_vsn;
	min_size = (vsn == SDIO_SDIO_REV_1_00) ? 28 : 42;

	if (size < min_size || buf[0] != 1)
		return -1;

	/* TPLFE_MAX_BLK_SIZE */
	func->max_blksize = buf[12] | (buf[13] << 8);

	/* TPLFE_ENABLE_TIMEOUT_VAL, present in ver 1.1 and above */
	if (vsn > SDIO_SDIO_REV_1_00)
		func->enable_timeout = (buf[28] | (buf[29] << 8)) * 10;
	else
		func->enable_timeout = 1000;//jiffies_to_msecs(HZ);

	return 0;
}

/**
* @brief 	Parse function extensions tuple.
* @param 	sdio[in]: SDIO handle.
* @param 	func[in]: SDIO fuction struct pointer.
* @param 	buf[in]: Buffer pointer.
* @param 	size[in]: Buffer size.
* @return 	0: success, -1: fail.
*/
static INT32S cistpl_funce(
	gpSDIOInfo_t *sdio,
	struct sdio_func *func,
	const INT8U *buf,
	INT32U size)
{
	int ret;

	/*
	 * There should be two versions of the CISTPL_FUNCE tuple,
	 * one for the common CIS (function 0) and a version used by
	 * the individual function's CIS (1-7). Yet, the later has a
	 * different length depending on the SDIO spec version.
	 */
	if (func)
		ret = cistpl_funce_func(func, buf, size);
	else
		ret = cistpl_funce_common(sdio, buf, size);

	if (ret && ret != -2)
	{
		DBG_PRINT("Bad CISTPL_FUNCE size %u, type %u\n", size, buf[0]);
	}

	return ret;
}

/**
* @brief 	Parse card information structure.
* @param 	sdio[in]: SDIO handle.
* @param 	func[in]: SDIO fuction struct pointer.
* @return 	0: success, -1: fail.
*/
static INT32S drvl2_sdio_read_cis(
	gpSDIOInfo_t *sdio,
	struct sdio_func *func)
{
	int ret;
	struct sdio_func_tuple *this, **prev;
	unsigned i, ptr = 0;

	/*
	 * Note that this works for the common CIS (function number 0) as
	 * well as a function's CIS * since SDIO_CCCR_CIS and SDIO_FBR_CIS
	 * have the same offset.
	 */
	for (i = 0; i < 3; i++)
	{
		INT8U x, fn;

		if (func)
			fn = func->num;
		else
			fn = 0;

		ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_FBR_BASE(fn) + SDIO_FBR_CIS + i, 0, &x);
		if (ret)
			return ret;
		ptr |= x << (i * 8);
	}

	if (func)
		prev = &func->tuples;
	else
		prev = &sdio->tuples;

	do {
		INT8U tpl_code, tpl_link;

		ret = drvl2_sdio_rw_direct( sdio, 0, 0, ptr++, 0, &tpl_code );
		if (ret)
			break;

		/* 0xff means we're done */
		if (tpl_code == 0xff)
			break;

		/* null entries have no link field or data */
		if (tpl_code == 0x00)
			continue;

		ret = drvl2_sdio_rw_direct( sdio, 0, 0, ptr++, 0, &tpl_link);
		if (ret)
			break;

		/* a size of 0xff also means we're done */
		if (tpl_link == 0xff)
			break;

		this = (struct sdio_func_tuple *)gp_malloc_align(sizeof(*this) + tpl_link, 4);
		if (!this)
			return -1;

		for (i = 0; i < tpl_link; i++)
		{
			ret = drvl2_sdio_rw_direct( sdio, 0, 0, ptr + i, 0, &this->data[i]);
			if (ret)
				break;
		}
		if (ret)
		{
			gp_free(this);
			break;
		}

		for (i = 0; i < sizeof(cis_tpl_list); i++)
			if (cis_tpl_list[i].code == tpl_code)
				break;
		if (i < sizeof(cis_tpl_list) )
		{
			const struct cis_tpl *tpl = cis_tpl_list + i;
			if (tpl_link < tpl->min_size)
			{
				DBG_PRINT("Bad CIS tuple 0x%02x(length = %u, expected >= %u)\n", tpl_code, tpl_link, tpl->min_size);
				ret = -1;
			}
			else if (tpl->parse)
			{
				ret = tpl->parse(sdio, func, this->data, tpl_link);
			}
			/*
			 * We don't need the tuple anymore if it was
			 * successfully parsed by the SDIO core or if it is
			 * not going to be parsed by SDIO drivers.
			 */
			if (!ret || ret != -2)
				gp_free(this);
		} else {
			/* unknown tuple */
			ret = -2;
		}

		if (ret == -2) {
			/* this tuple is unknown to the core or whitelisted */
			this->next = NULL;
			this->code = tpl_code;
			this->size = tpl_link;
			*prev = this;
			prev = &this->next;
			DBG_PRINT("Queuing CIS tuple 0x%02x length %u\n", tpl_code, tpl_link);
			/* keep on analyzing tuples */
			ret = 0;
		}
		ptr += tpl_link;
	} while (!ret);

	/*
	 * Link in all unknown tuples found in the common CIS so that
	 * drivers don't have to go digging in two places.
	 */
	if (func)
		*prev = sdio->tuples;

	return ret;
}

/**
* @brief 	Read common CIS tuple.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_read_common_cis(
	gpSDIOInfo_t *sdio)
{
	return drvl2_sdio_read_cis(sdio, NULL);
}

/**
* @brief 	Free common CIS tuple.
* @param 	sdio[in]: SDIO handle.
* @return 	None.
*/
void drvl2_sdio_free_common_cis(
	gpSDIOInfo_t *sdio)
{
	struct sdio_func_tuple *tuple, *victim;

	tuple = sdio->tuples;

	while (tuple)
	{
		victim = tuple;
		tuple = tuple->next;
		gp_free(victim);
	}

	sdio->tuples = NULL;
}

/**
* @brief 	Read function CIS tuple.
* @param 	func[in]:  SDIO fuction struct pointer.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_read_func_cis(
	struct sdio_func *func)
{
	int ret;

	ret = drvl2_sdio_read_cis(func->sdio, func);
	if (ret)
		return ret;
	/*
	 * Vendor/device id is optional for function CIS, so
	 * copy it from the card structure as needed.
	 */
	if (func->vendor == 0) {
		func->vendor = func->sdio->cis.vendor;
		func->device = func->sdio->cis.device;
	}

	return 0;
}

/**
* @brief 	Free function CIS tuple.
* @param 	func[in]:  SDIO fuction struct pointer.
* @return 	None.
*/
void drvl2_sdio_free_func_cis(
	struct sdio_func *func)
{
	struct sdio_func_tuple *tuple, *victim;

	tuple = func->tuples;

	while (tuple && tuple != func->sdio->tuples)
	{
		victim = tuple;
		tuple = tuple->next;
		gp_free(victim);
	}

	func->tuples = NULL;
}

#endif
