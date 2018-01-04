/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <log.h>
#include "os.h"
#include "porting.h"
#include <ssv_lib.h>
#include <hdr80211.h>
#include "smartConf.h"
#include "arc4.h"

#if (ENABLE_SMART_CONFIG==1)

//#define SLOG_PRINTF LOG_PRINTF
#define SLOG_PRINTF(fmt, ...)		((void)0)

//constant data
U8 magicNum[11] = "iot";//strt cmd will be check magic number
U8 rc4Key[4] = "Key";//rc4 key
U8 poly = 0x1D;//crc8 poly

//static struct etimer timer;
//struct data
smartComm smart;
static smart_control recePhone;
struct arc4_ctx smart_rc4;
//extern data
//extern struct sk_pbuff rx_skb; //Ian 20151225
//extern TAG_CABRIO_CONFIGURATION gCabrioConf; //Ian 20151225

enum t_SMARK_LINK_TYPE
{
    NOSMART	= 0,
    ICOMM,
    WECHAT,
    USER,
} SMARK_LINK_TYPE;

enum DATA_STATUS
{
    NOTYPE = -1,
    AP_MODE = 0,
    PHONE_MODE = 1,
    BOTH = 2,
} DATA_STATUS;

enum CMD_NUM
{
    START = 0,
    INFO,
    SSID_PASS_DATA,
    AP_LIST_CHECK,
    DONE,
    CHECK = 255,
} CMD_NUM;

IEEE80211STATUS gwifistatus;

static inline void  setSmartConfData(u8 *rx_data,u32 rx_len, u8 type);
static inline void checkBaseLen(U8 value, U8 type) ;
static inline void checkSeqSetPacket(U8 cmdValue, U8 type) ;
static inline U8 checkCrcWithSaveData(u8 *rx_data, u8 cmdValue, u8 type);
static inline U8 decryption(U8 value, U8 num) ;
static inline U8 decryptionSeq(U8 value) ;

void getPhoneMAC(U8 *mac, U8 len)
{
    memcpy(mac, smart.phoneMAC, len);
}

void getPhoneData(U8 *data, U8 len)
{
    memcpy(data, smart.buf, len);
}

void getSmartPhoneIP(U8 *ip, U8 len)
{
    memcpy(ip, smart.phoneIP, len);
}

void setPhoneMAC(U8 *mac, U8 len)
{
    memcpy(smart.phoneMAC, mac, len);
}

void setPhoneData(U8 *data, U8 len)
{
    smart.bufLen = len;
    memcpy(smart.buf, data, smart.bufLen);
}

void setSmartPhoneIP(U8 *ip, U8 len)
{
    memcpy(smart.phoneIP, ip, len);
}

void setSmartCmdStatus(u8 cmd, u8 type)
{
    if(type == BOTH) {
        gwifistatus.smart_state[0] = cmd;
    } else {
        gwifistatus.smart_state[type] = cmd;
    }
}

u8 getSmartCmdStatus(u8 type)
{
    if(type == BOTH) {
        return gwifistatus.smart_state[0];
    } else {
        return gwifistatus.smart_state[type];
    }
}

static U8
compData(U8 *a, U8 *b, U8 size)
{
    U8 i = 0;

    for(i = 0; i < size; i++)
        if(*(a + i) != *(b + i))
            return FALSE;

    return TRUE;
}

static U8
crc8_msb(U8 poly, U8* data, U8 size)
{
    U8 crc = 0;
    U8 bit = 0;

    while (size--) {
        crc ^= *data++;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static inline U8
decryption(U8 value, U8 num)
{
    return (value ^ recePhone.sonkey[num]);
}

static inline U8
decryptionSeq(U8 value)
{
    return (value ^ recePhone.sonkey[0]);
}

static inline U8
setCmdLen(U8 type)
{
    U8 cmdNum = getSmartCmdStatus(type);
    if(cmdNum == START) {
        recePhone.CmdNumLen[type] = strlen((char *)magicNum) + CRC8_DATA;
    } else if(cmdNum == INFO) {
        recePhone.CmdNumLen[type] =  SSID_DATA + PASS_DATA + CRC8_DATA + IP_DATA;
    } else if(cmdNum == SSID_PASS_DATA){
        recePhone.CmdNumLen[type] = recePhone.ssidLen[type] + recePhone.passLen[type] + CRC8_DATA;
    } else {
        SLOG_PRINTF("### setCmdLen error: %d type: %d ###\r\n", cmdNum, type);
        return FALSE;
    }

    return TRUE;
}

static inline U8
//checkCrcWithSaveData(PKT_Info *pPktInfo, u8 cmdValue, u8 type)
checkCrcWithSaveData(u8 *rx_data, u8 cmdValue, u8 type)
{
    U8 cmdNum = getSmartCmdStatus(type);
    //PHDR80211_Data ptr = (PHDR80211_Data)((u32)pPktInfo + pPktInfo->hdr_offset);
    PHDR80211_Data ptr = (PHDR80211_Data)rx_data;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};

    if(recePhone.backData[type]  != 0xffff) {
        recePhone.backSeq[type]  = recePhone.CmdNumLen[type] + 1;
    } else {
        recePhone.backData[type] = cmdValue;
    }

    SLOG_PRINTF("type: %d recePhone.backData: %d\r\n", type, recePhone.backData[type]);
    if(recePhone.backSeq[type] == (recePhone.CmdNumLen[type] - CRC8_DATA)) {//seq num equal crc8 data
        recePhone.crcData = crc8_msb(poly,recePhone.packet[type], (recePhone.CmdNumLen[type] - CRC8_DATA));
        if(recePhone.crcData == decryption((U8)recePhone.backData[type], recePhone.backSeq[type]) && cmdNum == START) {
            if(TRUE == compData(recePhone.packet[type], magicNum, strlen((char *)magicNum))) {
                LOG_PRINTF("SmartLink:INFO\r\n");
                setSmartCmdStatus(INFO, type);//change start cmd to ssid_data cmd
                recePhone.backSeq[type] = 0;
                recePhone.backData[type] = 0xffff;
                memset(recePhone.packet[type], 0, CMD_DATA_MAX_LEN);
                //etimer_reset(&timer); //Ian 20151225
                SLOG_PRINTF("[%d]have magic data\r\n", type);
                #if 0 //Ian 20151225
                if(recePhone.stage1Timer == FALSE) {
                    gwifistatus.smart_tmerSec = 20;
                    process_post_synch(&smart_conf_process, PROCESS_EVENT_MSG, NULL);
                    recePhone.stage1Timer =TRUE;
                }
                #endif
            }
        } else if(recePhone.crcData == decryption((U8)recePhone.backData[type], recePhone.backSeq[type]) && cmdNum == INFO) {
            setSmartCmdStatus(SSID_PASS_DATA, type);//change start cmd to ssid_data cmd
            LOG_PRINTF("SmartLink:SSID_PASS_DATA\r\n");
            recePhone.backSeq[type] = 0;
            recePhone.backData[type] = 0xffff;
            recePhone.ssidLen[type] = recePhone.packet[type][0];
            recePhone.passLen[type] = recePhone.packet[type][SSID_DATA];
            memcpy(recePhone.phoneIP[type], &recePhone.packet[type][SSID_DATA + 1], IP_DATA);
            SLOG_PRINTF("[%d] phoneIP: %d.%d.%d.%d\r\n",type, recePhone.phoneIP[type][0],  recePhone.phoneIP[type][1],  recePhone.phoneIP[type][2],  recePhone.phoneIP[type][3]);
            if(type == PHONE_MODE) {
                memcpy(recePhone.phoneMAC[PHONE_MODE], (const void *)&ptr->addr2.addr[0], MAC_DATA);
            } else {
                memcpy(recePhone.phoneMAC[AP_MODE], (const void *)&ptr->addr1.addr[0], MAC_DATA);
            }
            SLOG_PRINTF("[%d] phoneMAC: %x:%x:%x:%x:%x:%x\r\n", type, recePhone.phoneMAC[type][0],  recePhone.phoneMAC[type][1],  recePhone.phoneMAC[type][2],  recePhone.phoneMAC[type][3],  recePhone.phoneMAC[type][4],  recePhone.phoneMAC[type][5]);
            memset(recePhone.packet[type], 0, CMD_DATA_MAX_LEN);
            #if 0 //Ian 20151225
            if(recePhone.stage2Timer == FALSE) {
                gwifistatus.smart_tmerSec = 40;
                process_post_synch(&smart_conf_process, PROCESS_EVENT_MSG, NULL);
                recePhone.stage2Timer =TRUE;
            }
            #endif
        } else if(recePhone.crcData == decryption((U8)recePhone.backData[type], recePhone.backSeq[type]) && cmdNum == SSID_PASS_DATA){
            setSmartCmdStatus(AP_LIST_CHECK, PHONE_MODE);//change ssid_data cmd to check base len
            setSmartCmdStatus(AP_LIST_CHECK, AP_MODE);//change ssid_data cmd to check base len
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("SmartLink:AP_LIST_CHECK\r\n"));
            recePhone.backData[type] = 0xffff;
            recePhone.baseLenBufCount[type] = 0;
            gwifistatus.connAP.ssid_len= recePhone.ssidLen[type];
            //strncpy((char *)gwifistatus.connAP.ssid, (char *)recePhone.packet[type], (unsigned int)recePhone.ssidLen[type]);
            MEMCPY((void *)gwifistatus.connAP.ssid, (void *)recePhone.packet[type], (unsigned int)recePhone.ssidLen[type]);
            //gwifistatus.connAP.ssid[recePhone.ssidLen[type]] = '\0';
            gwifistatus.connAP.key_len= recePhone.passLen[type];
            strncpy((char *)gwifistatus.connAP.key, (char *)(recePhone.packet[type] + recePhone.ssidLen[type]), (unsigned int)recePhone.passLen[type]);
            //Ian 20151222
            //pbkdf2_sha1((unsigned char *)gwifistatus.connAP.key, gwifistatus.connAP.key_len, (unsigned char *)gwifistatus.connAP.ssid, gwifistatus.connAP.ssid_len, 4096, (unsigned char *)gwifistatus.connAP.pmk, 32);

            MEMCPY((void*)ssid_buf,(void*)gwifistatus.connAP.ssid,gwifistatus.connAP.ssid_len);
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("###############SSID: %s LEN: %d###############\r\n", ssid_buf, gwifistatus.connAP.ssid_len));
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("###############PASS: %s LEN: %d##############\r\n", gwifistatus.connAP.key, gwifistatus.connAP.key_len));
            memset(recePhone.packet[PHONE_MODE], 0, CMD_DATA_MAX_LEN);
            memset(recePhone.packet[AP_MODE], 0, CMD_DATA_MAX_LEN);

            //#if 0 //Ian 20151225
            setSmartPhoneIP(&(recePhone.phoneIP[type][0]), IP_DATA);
            setPhoneMAC(&(recePhone.phoneMAC[type][0]), MAC_DATA);
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("[%d] phoneIP: %d.%d.%d.%d\r\n",type, recePhone.phoneIP[type][0],  recePhone.phoneIP[type][1],  recePhone.phoneIP[type][2],  recePhone.phoneIP[type][3]));
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("[%d] phoneMAC: %x:%x:%x:%x:%x:%x\r\n", type, recePhone.phoneMAC[type][0],  recePhone.phoneMAC[type][1],  recePhone.phoneMAC[type][2],  recePhone.phoneMAC[type][3],  recePhone.phoneMAC[type][4],  recePhone.phoneMAC[type][5]));
            //memcpy(recePhone.buf, &(gCabrioConf).local_ip_addr, IP_DATA);
            //memcpy(recePhone.buf + IP_DATA, &(gwifistatus).local_mac, MAC_DATA);
            //setPhoneData(&(recePhone.buf[0]), IP_DATA + MAC_DATA);
            //#else
            setSmartCmdStatus(DONE, type);
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("SmartLink:DONE\r\n"));
            //#endif
            return TRUE;
        } else {
            //printf("do nothing: %d\n", cmdValue);
        }
    }
    return FALSE;
}

static inline void
checkSeqSetPacket(U8 cmdValue, U8 type)
{
    U8 cmdNum = getSmartCmdStatus(type);
    recePhone.seqData[type] = decryptionSeq(cmdValue);
    recePhone.NumSeq[type] = (recePhone.seqData[type] >> cmdNum);//seq shift cmd num bit

    SLOG_PRINTF("recePhone.seqData:%d cmdValue:%d\r\n", recePhone.seqData[type], cmdValue);
    if(((recePhone.seqData[type] & cmdNum) == cmdNum) && (recePhone.NumSeq[type] <= recePhone.CmdNumLen[type]) ) {
        //printf("[%d]NumSeq: %d backSeq :%d backData%d\n", type,recePhone.NumSeq[type], recePhone.backSeq[type], recePhone.backData[type]);
        if((recePhone.NumSeq[type] - recePhone.backSeq[type]) == 1 && recePhone.backData[type] != 0xffff) {
            recePhone.decrypValue = decryption((U8)recePhone.backData[type], recePhone.backSeq[type]);
            recePhone.packet[type][recePhone.backSeq[type]] = recePhone.decrypValue;
            SLOG_PRINTF("[%d]packet: %s\r\n", type, recePhone.packet[type]);
        } else {
            //printf("may be lost smartConf data: %d\n", cmdValue);
        }
        recePhone.backSeq[type] = recePhone.NumSeq[type];
    } else {
        recePhone.backSeq[type]  = recePhone.CmdNumLen[type] + 1;
       // printf("not smartConf data: %d\n", cmdValue);
    }
    recePhone.backData[type] = 0xffff;

}

static inline void
checkBaseLen(U8 value, U8 type)
{
    U8 i = 0, count = 0, lastBufVal = 0;
    //printf("[%d]recePhone.baseLenBufCount: %d value: %d \n", type ,recePhone.baseLenBufCount[type], value);
    if(recePhone.checkBaseLenBuf[type][0] >= value) {
        memset(recePhone.checkBaseLenBuf[type], 0, 5);
        recePhone.baseLenBufCount[type] = 0;
    }
    count = recePhone.baseLenBufCount[type];
    lastBufVal = recePhone.checkBaseLenBuf[type][count -1];
    if(count > 0 && (lastBufVal== value)) {
        return;
    } else if(count > 0 && ((value - lastBufVal) != 1)) {
        memset(recePhone.checkBaseLenBuf[type], 0, 5);
        recePhone.baseLenBufCount[type] = 0;
        return;
    }
    recePhone.checkBaseLenBuf[type][count] = value;
    recePhone.baseLenBufCount[type] = (count + 1) % MAX_BASE_LEN_BUF;
#if 1
    SLOG_PRINTF("status[%d] - ", type);
    for( i = 0; i < MAX_BASE_LEN_BUF; i++ ) {
        SLOG_PRINTF("%d ", recePhone.checkBaseLenBuf[type][i]);
    }
    SLOG_PRINTF("\r\n",__func__);
#endif
    for(i = 0; i < 3; i++) {
        if(recePhone.checkBaseLenBuf[type][i+1] - recePhone.checkBaseLenBuf[type][i] != 1) {
            if(count >= 4)
                memset(recePhone.checkBaseLenBuf[type], 0, 5);
            return;
        }
    }
    recePhone.baseLenBufCount[type] = MAX_BASE_LEN_BUF + 1;
    setSmartCmdStatus(START, type);
    #if 0 //Ian 20151225
    if(recePhone.stopScan == FALSE) {
        stopscanchannel();
        recePhone.stopScan = TRUE;
        gwifistatus.checkbeacon = FALSE;
        process_start(&smart_conf_process, NULL);
        LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL, ("set channel: %d\n", gwifistatus.connAP.channel));
    }
    #endif
}

static inline void
//setSmartConfData(PKT_Info *pPktInfo, u8 type)
setSmartConfData(u8 *rx_data,u32 rx_len, u8 type)
{
    U32 size = 0;
    U8 status = 0;
    u32 value = rx_len;

    if(recePhone.baseLenBufCount[type] <= MAX_BASE_LEN_BUF) {
    	checkBaseLen(value, type);
    } else {
        if(value < recePhone.checkBaseLenBuf[type][0] )
            return;

        size  =  value - recePhone.checkBaseLenBuf[type][0] ;//send data offset 1 byte
        status = setCmdLen(type);
        //printf("value: %d size: %d\n", value, size);
        if(FALSE == status) {
            SLOG_PRINTF("getCmdLen can not get normal cmd number\r\n",__func__);
        }

        if(size >= SEQ_BASE_CMD) {// seq data
            checkSeqSetPacket(size - SEQ_BASE_CMD, type);
        } else {// ssid & password data
            if(TRUE == checkCrcWithSaveData(rx_data,size, type)) {
                 //SET_REASON_TRAP0(0x0000);
            } else {
                //printf("setSmart do nothing: %d\n", cmdValue);
            }
        }
    }
}
/*---------------------------------------------------------------------------*/
void clearSmartLinkBuf()
{
    if(getSmartCmdStatus(PHONE_MODE) == CHECK) {
        memset(recePhone.checkBaseLenBuf[PHONE_MODE], 0, 5);
        recePhone.CmdNumLen[PHONE_MODE] = 0;
        recePhone.baseLenBufCount[PHONE_MODE]= 0;
    } else if(getSmartCmdStatus(AP_MODE) == CHECK) {
        memset(recePhone.checkBaseLenBuf[AP_MODE], 0, 5);
        recePhone.CmdNumLen[AP_MODE] = 0;
        recePhone.baseLenBufCount[AP_MODE]= 0;
    }
    //clearRxBuffer(); //Ian 20151225
}

void clearAllSmartLinkBuf()
{
    memset(recePhone.checkBaseLenBuf[PHONE_MODE], 0, 5);
    recePhone.CmdNumLen[PHONE_MODE] = 0;
    recePhone.baseLenBufCount[PHONE_MODE]= 0;
    memset(recePhone.checkBaseLenBuf[AP_MODE], 0, 5);
    recePhone.CmdNumLen[AP_MODE] = 0;
    recePhone.baseLenBufCount[AP_MODE]= 0;
    //clearRxBuffer(); //Ian 20151225
}

void resetSmartLink()
{
    clearAllSmartLinkBuf();
    setSmartCmdStatus(CHECK, PHONE_MODE);
    setSmartCmdStatus(CHECK, AP_MODE);
    //gwifistatus.smart_tmerSec = 10; //Ian 20151225
    recePhone.stopScan = FALSE;
    recePhone.stage1Timer = FALSE;
    recePhone.stage2Timer = FALSE;
    //startscanchannel(200 * CLOCK_MINI_SECOND); //Ian 20151225
}

void initSmartLink()
{
    //U8 freemac[6] = {0, 0, 0, 0, 0, 0};
    U8 tmpSonkey[CMD_DATA_MAX_LEN];
    memset(tmpSonkey, 0, sizeof(tmpSonkey));
    memset(&gwifistatus, 0, sizeof(IEEE80211STATUS));
    #if 0 //Ian 20151225
    _DBG_AT_SET_PEERMAC(freemac, 0);
    _DBG_AT_SET_BSSID(freemac);
    enableSmartHwFilter();
    #endif
    //gwifistatus.smart_tmerSec = 10; //20151225
    memset(&recePhone, 0, sizeof(recePhone));
    recePhone.backData[PHONE_MODE] = 0xffff;
    recePhone.backData[AP_MODE] = 0xffff;
    recePhone.stopScan = FALSE;
    recePhone.stage1Timer = FALSE;
    recePhone.stage2Timer = FALSE;
    setSmartCmdStatus(CHECK, PHONE_MODE);
    setSmartCmdStatus(CHECK, AP_MODE);
    #if 1 //Ian 20151225
    arc4_setkey(&smart_rc4, rc4Key, strlen((char *)rc4Key));
    arc4_xor(&smart_rc4, tmpSonkey, recePhone.sonkey, CMD_DATA_MAX_LEN);
    #endif
}

//void rx_process_smartConf(PKT_Info *pPktInfo, int unicast_ptk)
void rx_process_smartConf(u8 *rx_data, u32 rx_len,bool fromDS)
{
    U8 cmdNum = 0;
    U8 type = NOTYPE;

    if (!fromDS) {
        LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("ToDS:ADDR2:%2x:%2x:%2x:%2x:%2x:%2x\r\n",rx_data[10],rx_data[11],rx_data[12],rx_data[13],rx_data[14],rx_data[15]));
        type = PHONE_MODE;
    } else {
        LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("FromDS:ADDR2:%2x:%2x:%2x:%2x:%2x:%2x\r\n",rx_data[10],rx_data[11],rx_data[12],rx_data[13],rx_data[14],rx_data[15]));
        type = AP_MODE;
    }

    cmdNum = getSmartCmdStatus(type);
    if(cmdNum != DONE) {
        if(cmdNum != AP_LIST_CHECK) {
            setSmartConfData(rx_data, rx_len, type);
        } else {
            setSmartCmdStatus(AP_LIST_CHECK, type);
            //process_post_synch(&smart_conf_process, PROCESS_EVENT_CONTINUE, NULL); //Ian 20151225
        }
    } else {
        //dump_data ("other data", &rx_skb.data[0], rx_skb.len);
    }
}

SLINK_STATUS getSmartLinkStatus(void)
{
    if((START==getSmartCmdStatus(PHONE_MODE))||(START==getSmartCmdStatus(AP_MODE))){
        return SLINK_STATUS_CHANNEL_LOCKED;
    }

    if((DONE==getSmartCmdStatus(PHONE_MODE))||(DONE==getSmartCmdStatus(AP_MODE))){
        return SLINK_STATUS_COMPLETE;
    }

    return SLINK_STATUS_CONTINUE;
}

int getSmartLinkResult(AP_DETAIL_INFO *result)
{
    if(NULL==result){
        return -1;
    }

    if(SLINK_STATUS_COMPLETE==getSmartLinkStatus()){
        memcpy((void *)result,(const void *)&gwifistatus.connAP,sizeof(AP_DETAIL_INFO));
        return 0;
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
#endif
