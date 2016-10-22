/***************************************************************************//**
    \file          obd-dtc.c
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0.01
    \date          2016-10-10
    \description   obd code, dtc
*******************************************************************************/

/*******************************************************************************
    Include Files
*******************************************************************************/
#include "obd_dtc.h"
#include "obd_dtc_private.h"
/*******************************************************************************
    Type declaration
*******************************************************************************/

/*******************************************************************************
    Global Varaibles
*******************************************************************************/
static obd_dtc_para_t obd_dtc_para[OBD_DTC_CNT]=
{
    0x04621A,
    0x04631B,
    0xF00316,
    0xF00317,
    0xC07388,
    0xC10001,
    0xC10101,
    0xC12101,
    0xC12701,
    0xC12801,
    0xC12901,
    0xC13101,
    0xC14000,
    0xC15101,
    0xC16300,
    0xC16900,
    0xC18101,
    0xC23601,
    0xC16400,
    0xC16700,
    0x953D01,
    0x953E01,
    0x953F01,
    0x954001,
    0x954101,
    0x954401,
    0x954501,
    0x954601,
    0x954701
};

static obd_dtc_data_t obd_dtc_data[OBD_DTC_CNT];


static bool_t dtc_off;
static bool_t dtc_off_ex_bat;
/*******************************************************************************
    Function  Definition
*******************************************************************************/

/**
 * obd_dtc_clear_data - clear a obd dtc data
 *
 * @dtc_n : dtc index
 *
 * returns:
 *     void
 */
static void
obd_dtc_clear_data (uint16_t dtc_n)
{
    if (dtc_n >= OBD_DTC_CNT) return;
#ifdef SNAPSHOT
    obd_dtc_data[dtc_n].has_ff = FALSE;
#endif
    obd_dtc_data[dtc_n].dtc_st.all = 0u;
    obd_dtc_data[dtc_n].dtc_st.bit.test_ncmp_toc = 1;
    obd_dtc_data[dtc_n].dtc_st.bit.test_ncmp_slc = 1;

    obd_dtc_data[dtc_n].fec_cnt = 0;
    obd_dtc_data[dtc_n].afl_cnt = 0;
}


/*******************************************************************************
    Function  Definition - for obd default test
*******************************************************************************/

/**
 * obd_dtc_start_opcycle - start a fault detected operation cycle
 *
 * @dtc_n : dtc index
 *
 * returns:
 *     void
 */
void
obd_dtc_start_opcycle (uint16_t dtc_n)
{
    if (dtc_n >= OBD_DTC_CNT) return;

    obd_dtc_data[dtc_n].dtc_st.bit.test_fail = 0;
    obd_dtc_data[dtc_n].dtc_st.bit.test_fail_toc = 0;
    obd_dtc_data[dtc_n].dtc_st.bit.test_ncmp_toc = 1;

    obd_dtc_data[dtc_n].afl_cnt = 0;
}

/**
 * obd_dtc_update_data - manage the dtc status bits in a fault test operation cycle
 *
 * @dtc_n : dtc index
 * @test_result : obd fault detect result
 *
 * returns:
 *     void
 */
void
obd_dtc_update_data (uint16_t dtc_n, obd_dtc_test_t test_result)
{
    if (dtc_off == TRUE) return;

    if (dtc_off_ex_bat == TRUE && dtc_n != DTC_BATT_VOLTAG_BELOW && dtc_n != DTC_BATT_VOLTAG_ABOVE)
        return;

    if (dtc_n >= OBD_DTC_CNT) return;

    if (test_result == OBD_DTC_TEST_FAILED)
    {

        if (obd_dtc_data[dtc_n].afl_cnt < 127)
        {
            if (obd_dtc_data[dtc_n].afl_cnt < 0)
                obd_dtc_data[dtc_n].afl_cnt = 0;
            else
                obd_dtc_data[dtc_n].afl_cnt+=2;
            if (obd_dtc_data[dtc_n].afl_cnt >= 127)
            {
                obd_dtc_data[dtc_n].dtc_st.bit.test_fail = 1;
                obd_dtc_data[dtc_n].dtc_st.bit.test_fail_toc = 1;
                obd_dtc_data[dtc_n].dtc_st.bit.pending = 1;
                obd_dtc_data[dtc_n].dtc_st.bit.confirmed = 1;
                obd_dtc_data[dtc_n].dtc_st.bit.test_fail_slc = 1;
            }
        }
    }
    else
    {
        if (obd_dtc_data[dtc_n].afl_cnt > -128)
        {
            obd_dtc_data[dtc_n].afl_cnt--;

            if (obd_dtc_data[dtc_n].afl_cnt <= -128)
            {
                obd_dtc_data[dtc_n].dtc_st.bit.test_fail = 0;
                obd_dtc_data[dtc_n].dtc_st.bit.test_ncmp_toc = 0;
                obd_dtc_data[dtc_n].dtc_st.bit.test_ncmp_slc = 0;
            }
        }
    }
}

/*******************************************************************************
    Function  Definition - for uds service 14 hex & 19 hex
*******************************************************************************/
/**
 * get_dtc_number_by_status_mask - get dtc number by status mask
 *
 * @st_mask : dtc status mask
 *
 * returns:
 *     dtc number
 */
uint16_t
get_dtc_number_by_status_mask (uint8_t st_mask)
{
    uint16_t dtc_n;
    uint8_t dtc_st;
    uint16_t dtc_num;

    dtc_num = 0;
    for (dtc_n = 0; dtc_n < OBD_DTC_CNT; dtc_n++)
    {
        dtc_st = (obd_dtc_data[dtc_n].dtc_st.all & DTC_AVAILABILITY_STATUS_MASK);
        if (0 != (dtc_st & st_mask))
            dtc_num++;
    }

    return dtc_num;
}
/**
 * get_dtc_by_status_mask - get dtc by status mask
 *
 * @dtc_buf : the dest buf to store dtc data
 * @buf_len : the len of the buf
 * @st_mask : dtc status mask
 *
 * returns:
 *     dtc data len
 */
uint16_t
get_dtc_by_status_mask (uint8_t dtc_buf[], uint16_t buf_len, uint8_t st_mask)
{
    uint16_t dtc_n;
    uint8_t dtc_st;
    uint16_t dtc_dlc;

    dtc_dlc = 0;
    for (dtc_n = 0; dtc_n < OBD_DTC_CNT; dtc_n++)
    {
        dtc_st = obd_dtc_data[dtc_n].dtc_st.all;
        if (0 != (dtc_st & st_mask))
        {
            if ((dtc_dlc + 4) <= buf_len)
            {
                dtc_buf[dtc_dlc++] = (uint8_t)(obd_dtc_para[dtc_n].dtc_code >> 16);
                dtc_buf[dtc_dlc++] = (uint8_t)(obd_dtc_para[dtc_n].dtc_code >> 8);
                dtc_buf[dtc_dlc++] = (uint8_t)(obd_dtc_para[dtc_n].dtc_code >> 0);
                dtc_buf[dtc_dlc++] = (dtc_st & DTC_AVAILABILITY_STATUS_MASK);
            }
            else
            {
                break;
            }
        }
    }

    return dtc_dlc;
}

/**
 * get_supported_dtc - get all supported dtc
 *
 * @dtc_buf : the dest buf to store dtc data
 * @buf_len : the len of the buf
 *
 * returns:
 *     dtc data len
 */
uint16_t
get_supported_dtc (uint8_t dtc_buf[], uint16_t buf_len)
{
    uint16_t dtc_n;
    uint8_t dtc_st;
    uint16_t dtc_dlc;

    dtc_dlc = 0;
    for (dtc_n = 0; dtc_n < OBD_DTC_CNT; dtc_n++)
    {
        dtc_st = obd_dtc_data[dtc_n].dtc_st.all;

        if ((dtc_dlc + 4) <= buf_len)
        {
            dtc_buf[dtc_dlc++] = (uint8_t)(obd_dtc_para[dtc_n].dtc_code >> 16);
            dtc_buf[dtc_dlc++] = (uint8_t)(obd_dtc_para[dtc_n].dtc_code >> 8);
            dtc_buf[dtc_dlc++] = (uint8_t)(obd_dtc_para[dtc_n].dtc_code >> 0);
            dtc_buf[dtc_dlc++] = (dtc_st & DTC_AVAILABILITY_STATUS_MASK);
        }
        else
        {
            break;
        }
    }

    return dtc_dlc;
}
/**
 * clear_dtc_by_group - clear dtc data by group
 *
 * @dtc_buf : the dest buf to store dtc data
 * @buf_len : the len of the buf
 *
 * returns:
 *     dtc data len
 */
void
clear_dtc_by_group (uint32_t group)
{
    uint16_t dtc_n;
    switch (group)
    {
    case UDS_DTC_GROUP_EMISSION:
        for (dtc_n = 0; dtc_n < OBD_DTC_CNT; dtc_n++)
        {
            if (obd_dtc_para[dtc_n].dtc_code <= DTCG_EMISSION_END)
                obd_dtc_clear_data (dtc_n);
        }
        break;
    case UDS_DTC_GROUP_NETWORK:
        for (dtc_n = 0; dtc_n < OBD_DTC_CNT; dtc_n++)
        {
            if (obd_dtc_para[dtc_n].dtc_code >= DTCG_NETWORK_START && obd_dtc_para[dtc_n].dtc_code <= DTCG_NETWORK_END)
                obd_dtc_clear_data (dtc_n);
        }
        break;
    case UDS_DTC_GROUP_ALL:
        for (dtc_n = 0; dtc_n < OBD_DTC_CNT; dtc_n++)
        {
            obd_dtc_clear_data (dtc_n);
        }
        break;
    default:
        break;
    }
}
/****************EOF****************/
