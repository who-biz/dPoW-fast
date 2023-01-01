
/******************************************************************************
 * Copyright © 2014-2018 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
//
//  LP_nativeDEX.c
//  marketmaker
//
// portfolio to set prices from historical
// portfolio value based on ask?
// else claim path
//
// WONTFIX:
// dPoW security -> 4: KMD notarized, 5: BTC notarized, after next notary elections
// bigendian architectures need to use little endian for sighash calcs
// improve critical section detection when parallel trades
// use electrum in case of addr change in swap
// locktime claiming on sporadic assetchains
// there is an issue about waiting for notarization for a swap that never starts (expiration ok)

#include <stdio.h>
#ifndef MM_VERSION
#define MM_VERSION "UNKNOWN"
#endif

long LP_cjson_allocated,LP_cjson_total,LP_cjson_count;

struct LP_millistats
{
    double lastmilli,millisum,threshold;
    uint32_t count;
    char name[64];
} LP_psockloop_stats,LP_reserved_msgs_stats,utxosQ_loop_stats,command_rpcloop_stats,queue_loop_stats,prices_loop_stats,LP_coinsloop_stats,LP_coinsloopBTC_stats,LP_coinsloopKMD_stats,LP_pubkeysloop_stats,LP_swapsloop_stats,LP_gcloop_stats,LP_tradesloop_stats;
extern int32_t IAMLP;
char LP_methodstr[64];

void LP_millistats_update(struct LP_millistats *mp)
{
    double elapsed,millis;
    if ( mp == 0 )
    {
        if ( IAMLP != 0 )
        {
            mp = &LP_psockloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        }
        mp = &LP_reserved_msgs_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &utxosQ_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &command_rpcloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &queue_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &prices_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloopBTC_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloopKMD_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_pubkeysloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_tradesloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_swapsloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_gcloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
    }
    else
    {
        if ( mp->lastmilli == 0. )
            mp->lastmilli = OS_milliseconds();
        else
        {
            mp->count++;
            millis = OS_milliseconds();
            elapsed = (millis - mp->lastmilli);
            mp->millisum += elapsed;
            if ( mp->threshold != 0. && elapsed > mp->threshold )
            {
                //if ( IAMLP == 0 )
                    printf("%32s elapsed %10.2f millis > threshold %10.2f, ave %10.2f millis, count.%u %s\n",mp->name,elapsed,mp->threshold,mp->millisum/mp->count,mp->count,LP_methodstr);
            }
            mp->lastmilli = millis;
        }
    }
}

#include "LP_include.h"

#ifndef NOTETOMIC
#include "LP_etomic.h"
#endif

portable_mutex_t LP_peermutex,LP_UTXOmutex,LP_utxomutex,LP_commandmutex,LP_cachemutex,LP_swaplistmutex,LP_forwardmutex,LP_pubkeymutex,LP_networkmutex,LP_psockmutex,LP_coinmutex,LP_messagemutex,LP_portfoliomutex,LP_electrummutex,LP_butxomutex,LP_reservedmutex,LP_nanorecvsmutex,LP_tradebotsmutex,LP_gcmutex,LP_inusemutex,LP_cJSONmutex,LP_logmutex,LP_statslogmutex,LP_tradesmutex,LP_commandQmutex,LP_blockinit_mutex,LP_pendswap_mutex,LP_listmutex,LP_gtcmutex;
int32_t LP_canbind;
char *Broadcaststr,*Reserved_msgs[2][1000];
int32_t num_Reserved_msgs[2],max_Reserved_msgs[2];
struct LP_peerinfo  *LP_peerinfos,*LP_mypeer;
struct LP_forwardinfo *LP_forwardinfos;
struct iguana_info *LP_coins;
struct LP_pubkey_info *LP_pubkeyinfos;
struct rpcrequest_info *LP_garbage_collector;
struct LP_address_utxo *LP_garbage_collector2;
struct LP_trade *LP_trades,*LP_tradesQ;

//uint32_t LP_deadman_switch;
uint16_t LP_fixed_pairport;//,LP_publicport;
uint32_t LP_lastnonce,LP_swap_endcritical,LP_swap_critical,LP_RTcount,LP_swapscount;
int32_t LP_STOP_RECEIVED,LP_numactive_LP;//,LP_mybussock = -1;
int32_t LP_mypubsock = -1,IPC_ENDPOINT = -1;
int32_t LP_cmdcount,LP_mypullsock = -1;
int32_t LP_numfinished,LP_showwif,IAMLP = 0;
double LP_profitratio = 1.;

struct LP_privkey { bits256 privkey; uint8_t rmd160[20]; };

struct LP_globals
{
    //struct LP_utxoinfo  *LP_utxoinfos[2],*LP_utxoinfos2[2];
    bits256 LP_mypub25519,LP_privkey,LP_mypriv25519,LP_passhash;
    uint64_t LP_skipstatus[10000], LP_required_etomic_balance;
    uint16_t netid;
    uint8_t LP_myrmd160[20],LP_pubsecp[33];
    uint32_t LP_sessionid,counter,mpnet;
    int32_t LP_IAMLP,LP_pendingswaps,USERPASS_COUNTER,LP_numprivkeys,initializing,waiting,LP_numskips;
    char seednode[64],USERPASS[65],USERPASS_WIFSTR[64],LP_myrmd160str[41],gui[65],LP_NXTaddr[64];
    struct LP_privkey LP_privkeys[100];
} G;

uint32_t LP_rand()
{
    uint32_t retval;
    retval = rand();
    retval = (retval << 7) ^ (retval >> 17) ^ rand();
    retval = (retval << 13) ^ (retval >> 13) ^ rand();
    retval = (retval << 17) ^ (retval >> 7) ^ rand();
    return(retval);
}

#include "LP_network.c"

char *activecoins[] = { "BTC", "KMD" };
char GLOBAL_DBDIR[] = { "DB" };
char LP_myipaddr[64],USERHOME[512] = { "/root" };
char LP_gui[65] = { "cli" };

char *default_LPnodes[] = { "5.9.253.195", "173.212.225.176", "136.243.45.140", "23.254.202.142", "45.32.19.196"
    //"24.54.206.138", "107.72.162.127", "72.50.16.86", "51.15.202.191", "173.228.198.88",
    //"51.15.203.171", "51.15.86.136", "51.15.94.249", "51.15.80.18", "51.15.91.40", "51.15.54.2", "51.15.86.31", "51.15.82.29", "51.15.89.155", "173.212.225.176", "136.243.45.140"
};

// stubs

void tradebot_swap_balancingtrade(struct basilisk_swap *swap,int32_t iambob)
{
    
}

void tradebot_pendingadd(cJSON *tradejson,char *base,double basevolume,char *rel,double relvolume)
{
    // add to trades
}

char *LP_getdatadir()
{
    return(USERHOME);
}

char *blocktrail_listtransactions(char *symbol,char *coinaddr,int32_t num,int32_t skip)
{
    return(0);
}

#include "LP_mmjson.c"
#include "LP_socket.c"
#include "LP_secp.c"
#include "LP_bitcoin.c"
#include "LP_coins.c"
#include "LP_rpc.c"
#include "LP_mpnet.c"
#include "LP_NXT.c"
#include "LP_cache.c"
#include "LP_RTmetrics.c"
#include "LP_utxo.c"
#include "LP_prices.c"
#include "LP_scan.c"
#include "LP_transaction.c"
#include "LP_stats.c"
#include "LP_remember.c"
#include "LP_instantdex.c"
#include "LP_swap.c"
#include "LP_peers.c"
#include "LP_privkey.c"
#include "LP_forwarding.c"
#include "LP_signatures.c"
#include "LP_ordermatch.c"
#include "LP_tradebots.c"
#include "LP_portfolio.c"
#include "LP_messages.c"
#include "LP_commands.c"

char *LP_command_process(void *ctx,char *myipaddr,int32_t pubsock,cJSON *argjson,uint8_t *data,int32_t datalen,int32_t stats_JSONonly)
{
    char *retstr=0; cJSON *retjson; bits256 zero;
    if ( jobj(argjson,"result") != 0 || jobj(argjson,"error") != 0 )
        return(0);
    if ( stats_JSONonly != 0 || LP_tradecommand(0,ctx,myipaddr,pubsock,argjson,data,datalen) <= 0 )
    {
        if ( (retstr= stats_JSON(ctx,0,myipaddr,pubsock,argjson,"127.0.0.1",stats_JSONonly)) != 0 )
        {
            //printf("%s PULL.[%d]-> (%s)\n",myipaddr != 0 ? myipaddr : "127.0.0.1",datalen,retstr);
            //if ( pubsock >= 0 ) //strncmp("{\"error\":",retstr,strlen("{\"error\":")) != 0 &&
                //LP_send(pubsock,retstr,(int32_t)strlen(retstr)+1,0);
        }
    }
    else if ( LP_statslog_parse() > 0 && 0 )
    {
        memset(zero.bytes,0,sizeof(zero));
        if ( (retjson= LP_statslog_disp(2000000000,2000000000,"",zero,0,0))) // pending swaps
            free_json(retjson);
    }
    return(retstr);
}

char *LP_decrypt(uint8_t decoded[LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES],uint8_t *ptr,int32_t *recvlenp)
{
    uint8_t *nonce,*cipher; int32_t recvlen,cipherlen; char *jsonstr = 0;
    recvlen = *recvlenp;
    nonce = &ptr[2];
    cipher = &ptr[2 + crypto_box_NONCEBYTES];
    cipherlen = recvlen - (2 + crypto_box_NONCEBYTES);
    if ( cipherlen > 0 && cipherlen <= LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES )
    {
        if ( (jsonstr= (char *)_SuperNET_decipher(nonce,cipher,decoded,cipherlen,GENESIS_PUBKEY,G.LP_mypriv25519)) != 0 )
        {
            recvlen = (cipherlen - crypto_box_ZEROBYTES);
            if ( strlen(jsonstr)+1 != recvlen )
            {
                printf("unexpected len %d vs recvlen.%d\n",(int32_t)strlen(jsonstr)+1,recvlen);
                jsonstr = 0;
            } //else printf("decrypted (%s)\n",jsonstr);
        }
    } else printf("cipher.%d too big for %d\n",cipherlen,LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES);
    *recvlenp = recvlen;
    return(jsonstr);
}

char *LP_process_message(void *ctx,char *typestr,char *myipaddr,int32_t pubsock,uint8_t *ptr,int32_t recvlen,int32_t recvsock)
{
    static uint32_t dup,uniq;
    uint8_t jdecoded[LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES]; int32_t i,len,cipherlen,datalen=0,duplicate=0,encrypted=0; char *method,*method2,*tmp,*cipherstr,*retstr=0,*jsonstr=0; cJSON *argjson; uint32_t crc32;
    //double millis = OS_milliseconds();
    crc32 = calc_crc32(0,&ptr[2],recvlen-2);
    if ( (crc32 & 0xff) == ptr[0] && ((crc32>>8) & 0xff) == ptr[1] )
        encrypted = 1;
    i = LP_crc32find(&duplicate,-1,crc32);
    if ( duplicate != 0 )
        dup++;
    else uniq++;
    portable_mutex_lock(&LP_commandmutex);
    if ( (LP_rand() % 100000) == 0 )
        printf("%s dup.%d (%u / %u) %.1f%% encrypted.%d recv.%u [%02x %02x] vs %02x %02x\n",typestr,duplicate,dup,dup+uniq,(double)100*dup/(dup+uniq),encrypted,crc32,ptr[0],ptr[1],crc32&0xff,(crc32>>8)&0xff);
    if ( duplicate == 0 )
    {
        if ( i >= 0 )
            LP_crc32find(&duplicate,i,crc32);
        if ( encrypted != 0 )
            jsonstr = LP_decrypt(jdecoded,ptr,&recvlen);
        else if ( (datalen= is_hexstr((char *)ptr,0)) > 0 )
        {
            datalen >>= 1;
            jsonstr = malloc(datalen + 1);
            decode_hex((void *)jsonstr,datalen,(char *)ptr);
            jsonstr[datalen] = 0;
        } else jsonstr = (char *)ptr;
        if ( jsonstr != 0 && (argjson= cJSON_Parse(jsonstr)) != 0 )
        {
            uint8_t decoded[LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES];
            //printf("[%s]\n",jsonstr);
            cipherlen = 0;
            if ( (cipherstr= jstr(argjson,"cipher")) != 0 && (cipherlen= is_hexstr(cipherstr,0)) > 32 && cipherlen <= sizeof(decoded)*2 )
            {
                method2 = jstr(argjson,"method2");
                if ( (method= jstr(argjson,"method")) != 0 && (strcmp(method,"encrypted") == 0 ||(method2 != 0 && strcmp(method2,"encrypted") == 0)) )
                {
                    cipherlen >>= 1;
                    decode_hex(decoded,cipherlen,cipherstr);
                    crc32 = calc_crc32(0,&decoded[2],cipherlen-2);
                    if ( (tmp= LP_decrypt(jdecoded,decoded,&cipherlen)) != 0 )
                    {
                        jsonstr = tmp;
                        free_json(argjson);
                        argjson = cJSON_Parse(jsonstr);
                        recvlen = cipherlen;
                        encrypted = 1;
                        if ( (crc32 & 0xff) == decoded[0] && ((crc32>>8) & 0xff) == decoded[1] )
                        {
                            i = LP_crc32find(&duplicate,-1,crc32);
                            if ( duplicate == 0 && i >= 0 )
                                LP_crc32find(&duplicate,i,crc32);
                        }
                        printf("%02x %02x %08x duplicate.%d decrypted.(%s)\n",decoded[0],decoded[1],crc32,duplicate,jsonstr);
                    }
                    else
                    {
                        //printf("packet not for this node %u\n",crc32);
                    }
                } else printf("error (%s) method is %s\n",jsonstr,method);
            }
            if ( jsonstr != 0 && argjson != 0 )
            {
                len = (int32_t)strlen(jsonstr) + 1;
                if ( (method= jstr(argjson,"method")) != 0 && strcmp(method,"gettradestatus") != 0 && strcmp(method,"psock") != 0 && strcmp(method,"broadcast") == 0 )
                {
                    bits256 zero; cJSON *reqjson; char *cipherstr; int32_t cipherlen; uint8_t cipher[LP_ENCRYPTED_MAXSIZE];
                    if ( (reqjson= LP_dereference(argjson,"broadcast")) != 0 )
                    {
                        Broadcaststr = jprint(reqjson,0);
                        if ( (cipherstr= jstr(reqjson,"cipher")) != 0 )
                        {
                            cipherlen = (int32_t)strlen(cipherstr) >> 1;
                            if ( cipherlen <= sizeof(cipher) )
                            {
                                decode_hex(cipher,cipherlen,cipherstr);
                                LP_queuesend(calc_crc32(0,&cipher[2],cipherlen-2),LP_mypubsock,"","",cipher,cipherlen);
                            } else retstr = clonestr("{\"error\":\"cipher too big\"}");
                        }
                        else
                        {
                            memset(zero.bytes,0,sizeof(zero));
                            if ( 0 && (method= jstr(reqjson,"method")) != 0 && (strcmp(method,"tradestatus") == 0) )
                                    printf("broadcast.(%s)\n",Broadcaststr);
                            LP_reserved_msg(0,"","",zero,jprint(reqjson,0));
                        }
                        retstr = clonestr("{\"result\":\"success\"}");
                        free_json(reqjson);
                    } else retstr = clonestr("{\"error\":\"couldnt dereference sendmessage\"}");
                }
                else
                {
                    LP_queuecommand(0,jsonstr,pubsock,0,0);
                    //if ( (retstr= LP_command_process(ctx,myipaddr,pubsock,argjson,&((uint8_t *)ptr)[len],recvlen - len)) != 0 )
                    //{
                    //}
                }
            }
            if ( argjson != 0 )
                free_json(argjson);
        }
    } //else printf("DUPLICATE.(%s)\n",(char *)ptr);
    portable_mutex_unlock(&LP_commandmutex);
    if ( jsonstr != 0 && (void *)jsonstr != (void *)ptr && encrypted == 0 )
        free(jsonstr);
    return(retstr);
}

int32_t LP_sock_check(char *typestr,void *ctx,char *myipaddr,int32_t pubsock,int32_t sock,char *remoteaddr,int32_t maxdepth)
{
    static char *line;
    int32_t recvlen=1,msglen,nonz = 0; cJSON *recvjson; void *ptr,*msg; char methodstr[64],*decodestr,*retstr,*str; struct nn_pollfd pfd;
    if ( line == 0 )
        line = calloc(1,1024*1024);
    if ( sock >= 0 )
    {
        while ( nonz < maxdepth && recvlen > 0 )
        {
            decodestr = 0;
            nonz++;
            memset(&pfd,0,sizeof(pfd));
            pfd.fd = sock;
            pfd.events = NN_POLLIN;
            if ( nn_poll(&pfd,1,1) != 1 )
                break;
            ptr = 0;
            if ( (recvlen= nn_recv(sock,&ptr,NN_MSG,0)) > 0 )
            {
                //printf("%s nn_recv.%d\n",typestr,recvlen);
                decodestr = 0;
                if ( recvlen > 32768 )
                {
                    printf("unexpectedly large packet\n");
                }
                else
                {
                    msg = ptr;
                    msglen = recvlen;
                    if ( (recvjson= cJSON_Parse((char *)ptr)) == 0 )
                    {
                        if ( (decodestr= MMJSON_decode(ptr,recvlen)) != 0 )
                        {
                            if ( (recvjson= cJSON_Parse(decodestr)) != 0 )
                            {
                                msg = decodestr;
                                msglen = (int32_t)strlen(decodestr) + 1;
                            }
                            //printf("decoded.(%s)\n",decodestr);
                        } else printf("couldnt decode linebuf[%d]\n",recvlen);
                    }
                    methodstr[0] = 0;
                    if ( recvjson != 0 )
                    {
                        safecopy(LP_methodstr,jstr(recvjson,"method"),sizeof(LP_methodstr));
                        free_json(recvjson);
                    }
                    int32_t validreq = 1;
                    /*if ( strlen((char *)ptr)+sizeof(bits256) <= recvlen )
                     {
                     if ( LP_magic_check(ptr,recvlen,remoteaddr) <= 0 )
                     {
                     //printf("magic check error\n");
                     } else validreq = 1;
                     recvlen -= sizeof(bits256);
                     }*/
                    if ( validreq != 0 )
                    {
#if !defined(NDEBUG) && !defined(NOTETOMIC) && defined(__linux__)
                        printf("Msg from %s\n", remoteaddr);
#endif
                        if ( (retstr= LP_process_message(ctx,typestr,myipaddr,pubsock,msg,msglen,sock)) != 0 )
                            free(retstr);
                        
                        if ( Broadcaststr != 0 )
                        {
                            //printf("self broadcast.(%s)\n",Broadcaststr);
                            str = Broadcaststr;
                            Broadcaststr = 0;
                            LP_queuecommand(0,str,pubsock,0,0);
                            /*if ( (argjson= cJSON_Parse(str)) != 0 )
                            {
                                //portable_mutex_lock(&LP_commandmutex);
                                if ( LP_tradecommand(0,ctx,myipaddr,pubsock,argjson,0,0) <= 0 )
                                {
                                    if ( (retstr= stats_JSON(ctx,0,myipaddr,pubsock,argjson,remoteaddr,0)) != 0 )
                                        free(retstr);
                                }
                                //portable_mutex_unlock(&LP_commandmutex);
                                free_json(argjson);
                            }*/
                            free(str);
                        }
                    }
                }
            }
            if ( ptr != 0 )
            {
                nn_freemsg(ptr), ptr = 0;
                //free(buf);
            }
            if ( decodestr != 0 )
                free(decodestr);
        }
    }
    return(nonz);
}

int32_t LP_nanomsg_recvs(void *ctx)
{
    int32_t n=0,nonz = 0; char *origipaddr; struct LP_peerinfo *peer,*tmp;
    if ( (origipaddr= LP_myipaddr) == 0 )
        origipaddr = "127.0.0.1";
    portable_mutex_lock(&LP_nanorecvsmutex);
    HASH_ITER(hh,LP_peerinfos,peer,tmp)
    {
        if ( n++ > 0 && peer->errors >= LP_MAXPEER_ERRORS )
        {
            if ( (LP_rand() % 10000) == 0 )
                peer->errors--;
            else
            {
                //printf("skip %s\n",peer->ipaddr);
                continue;
            }
        }
        nonz += LP_sock_check("SUB",ctx,origipaddr,LP_mypubsock,peer->subsock,peer->ipaddr,1);
    }
    /*HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
     {
     if ( coin->inactive != 0 )
     continue;
     if ( coin->bussock >= 0 )
     nonz += LP_sock_check(coin->symbol,ctx,origipaddr,-1,coin->bussock,LP_profitratio - 1.);
     }*/
    if ( LP_mypullsock >= 0 )
    {
        nonz += LP_sock_check("PULL",ctx,origipaddr,-1,LP_mypullsock,"127.0.0.1",1);
    }
    portable_mutex_unlock(&LP_nanorecvsmutex);
    //if ( G.mpnet != 0 )
        LP_mpnet_check(ctx,origipaddr,LP_mypubsock);
    return(nonz);
}

void command_rpcloop(void *ctx)
{
    int32_t nonz = 0;
    strcpy(command_rpcloop_stats.name,"command_rpcloop");
    command_rpcloop_stats.threshold = 2500.;
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        LP_millistats_update(&command_rpcloop_stats);
        nonz = LP_nanomsg_recvs(ctx);
        //if ( LP_mybussock >= 0 )
        //    nonz += LP_sock_check("BUS",ctx,origipaddr,-1,LP_mybussock);
        if ( nonz == 0 )
        {
            if ( IAMLP != 0 )
                usleep(10000);
            else usleep(50000);
        }
        else if ( IAMLP == 0 )
            usleep(1000);
    }
}

void LP_coinsloop(void *_coins)
{
    static int32_t didfilescreate;
    struct LP_address *ap=0; struct LP_transaction *tx; cJSON *retjson; struct LP_address_utxo *up,*tmp; struct iguana_info *coin,*ctmp; char str[65],*retstr,*hexstr,*txidstr; struct electrum_info *ep,*backupep=0; bits256 zero; int32_t notarized,oldht,j,nonz; char *coins = _coins;
    if ( strcmp("BTC",coins) == 0 )
    {
        strcpy(LP_coinsloopBTC_stats.name,"BTC coin loop");
        LP_coinsloopBTC_stats.threshold = 200000.;
    }
    else if ( strcmp("KMD",coins) == 0 )
    {
        strcpy(LP_coinsloopKMD_stats.name,"KMD coin loop");
        LP_coinsloopKMD_stats.threshold = 100000.;
    }
    else
    {
        strcpy(LP_coinsloop_stats.name,"other coins loop");
        LP_coinsloop_stats.threshold = 50000.;
    }
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        if ( strcmp("BTC",coins) == 0 )
            LP_millistats_update(&LP_coinsloopBTC_stats);
        else if ( strcmp("KMD",coins) == 0 )
            LP_millistats_update(&LP_coinsloopKMD_stats);
        else LP_millistats_update(&LP_coinsloop_stats);
        nonz = 0;
        HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
        {
            if ( coins != 0 )
            {
                if ( coins[0] != 0 )
                {
                    if ( strcmp(coins,coin->symbol) != 0 )
                        continue;
                }
                else // avoid hardcode special case LP_coinsloop
                {
                    if ( strcmp("BTC",coin->symbol) == 0 || strcmp("KMD",coin->symbol) == 0 )
                        continue;
                }
            }
            if ( coin->smartaddr[0] == 0 )
            {
                //printf("%s has no smartaddress??\n",coin->symbol);
                continue;
            }
            if ( didfilescreate == 0 && strcmp("KMD",coin->symbol) == 0 )
            {
                LP_instantdex_filescreate(coin->smartaddr);
                didfilescreate = 1;
            }
            memset(&zero,0,sizeof(zero));
            if ( coin->inactive != 0 )
                continue;
            if ( coin->did_addrutxo_reset == 0 )
            {
                int32_t num;
                LP_address_utxo_reset(&num,coin);
                coin->did_addrutxo_reset = 1;
            }
            //free_json(LP_address_balance(coin,coin->smartaddr,1)); expensive invoking gettxout
            if ( coin->do_autofill_merge != 0 )
            {
                if ( (retstr= LP_autofillbob(coin,coin->do_autofill_merge*1.02)) != 0 )
                {
                    if ( (retjson= cJSON_Parse(retstr)) != 0 )
                    {
                        if ( (hexstr= jstr(retjson,"hex")) != 0 )
                        {
                            if ( (txidstr= LP_sendrawtransaction(coin->symbol,hexstr,0)) != 0 )
                            {
                                printf("autofill created %s\n",txidstr);
                                free(txidstr);
                                coin->fillsatoshis = coin->do_autofill_merge;
                                coin->do_autofill_merge = 0;
                                coin->bobfillheight = LP_getheight(&notarized,coin);
                            }
                        }
                        free_json(retjson);
                    }
                    free(retstr);
                }
            }
            if ( coin->longestchain == 1 ) // special init value
                coin->longestchain = LP_getheight(&notarized,coin);
            if ( (ep= coin->electrum) != 0 )
            {
                if ( (backupep= ep->prev) == 0 )
                    backupep = ep;
                if ( (retjson= electrum_address_listunspent(coin->symbol,ep,&retjson,coin->smartaddr,1,zero,zero)) != 0 )
                    free_json(retjson);
                if ( (ap= LP_addressfind(coin,coin->smartaddr)) != 0 )
                {
                    DL_FOREACH_SAFE(ap->utxos,up,tmp)
                    {
                        if ( up->U.height > 0 && up->spendheight < 0 )
                        {
                            if ( up->SPV == 0 )
                            {
                                nonz++;
                                up->SPV = LP_merkleproof(coin,coin->smartaddr,backupep,up->U.txid,up->U.height);
                                if ( up->SPV > 0 )
                                {
                                    if ( (tx= LP_transactionfind(coin,up->U.txid)) != 0 && tx->SPV == 0 )
                                    {
                                        tx->SPV = up->SPV;
                                        //printf("%s %s: SPV.%d\n",coin->symbol,bits256_str(str,up->U.txid),up->SPV);
                                    }
                                }
                            }
                            else if ( up->SPV == -1 )
                            {
                                nonz++;
                                printf("SPV failure for %s %s\n",coin->symbol,bits256_str(str,up->U.txid));
                                oldht = up->U.height;
                                LP_txheight_check(coin,ap->coinaddr,up->U.txid);
                                if ( oldht != up->U.height )
                                    up->SPV = LP_merkleproof(coin,coin->smartaddr,backupep,up->U.txid,up->U.height);
                                if ( up->SPV <= 0 )
                                    up->SPV = -2;
                                else printf("%s %s: corrected SPV.%d\n",coin->symbol,bits256_str(str,up->U.txid),up->SPV);
                            }
                        }
                    }
                }
                while ( ep != 0 )
                {
                    if ( time(NULL) > ep->keepalive+LP_ELECTRUM_KEEPALIVE )
                    {
                        //printf("%s electrum.%p needs a keepalive: lag.%d\n",ep->symbol,ep,(int32_t)(time(NULL) - ep->keepalive));
                        if ( (retjson= electrum_banner(coin->symbol,ep,&retjson)) != 0 )
                            free_json(retjson);
                        ep->keepalive = (uint32_t)time(NULL);
                    }
                    ep = ep->prev;
                }
                continue;
            }
            if ( coin->firstrefht == 0 )
                continue;
            else if ( coin->firstscanht == 0 )
                coin->lastscanht = coin->firstscanht = coin->firstrefht;
            else if ( coin->firstrefht < coin->firstscanht )
            {
                printf("detected %s firstrefht.%d < firstscanht.%d\n",coin->symbol,coin->firstrefht,coin->firstscanht);
                coin->lastscanht = coin->firstscanht = coin->firstrefht;
            }
            if ( coin->lastscanht == coin->longestchain+1 )
            {
                //printf("%s lastscanht.%d is longest.%d + 1\n",coin->symbol,coin->lastscanht,coin->longestchain);
                continue;
            }
            else if ( coin->lastscanht > coin->longestchain+1 )
            {
                printf("detected chain rewind lastscanht.%d vs longestchain.%d, first.%d ref.%d\n",coin->lastscanht,coin->longestchain,coin->firstscanht,coin->firstrefht);
                LP_undospends(coin,coin->longestchain-1);
                //LP_mempoolscan(coin->symbol,zero);
                coin->lastscanht = coin->longestchain - 1;
                if ( coin->firstscanht < coin->lastscanht )
                    coin->lastscanht = coin->firstscanht;
                continue;
            }
            //if ( strcmp(coin->symbol,"BTC") != 0 && strcmp(coin->symbol,"KMD") != 0 ) // SPV as backup
            if ( coin->lastscanht < coin->longestchain )
            {
                nonz++;
                if ( strcmp("BTC",coins) == 0 )//&& coin->lastscanht < coin->longestchain-3 )
                    printf("[%s]: %s ref.%d scan.%d to %d, longest.%d\n",coins,coin->symbol,coin->firstrefht,coin->firstscanht,coin->lastscanht,coin->longestchain);
                for (j=0; j<100; j++)
                {
                    if ( LP_blockinit(coin,coin->lastscanht) < 0 )
                    {
                        printf("please ignore this blockinit.%s %d error\n",coin->symbol,coin->lastscanht);
                        sleep(10);
                        break;
                    }
                    coin->lastscanht++;
                    if ( coin->lastscanht == coin->longestchain+1 || strcmp("BTC",coins) == 0 )
                        break;
                }
                if ( strcmp("BTC",coins) == 0 )
                    printf("done [%s]: %s ref.%d scan.%d to %d, longest.%d\n",coins,coin->symbol,coin->firstrefht,coin->firstscanht,coin->lastscanht,coin->longestchain);
            }
        }
        if ( coins == 0 )
            return;
        //if ( nonz == 0 )
            usleep(100000);
    }
}

int32_t LP_mainloop_iter(void *ctx,char *myipaddr,struct LP_peerinfo *mypeer,int32_t pubsock)
{
    static uint32_t counter;//,didinstantdex;
    struct iguana_info *coin,*ctmp; char *origipaddr; uint32_t now; int32_t notarized,height,nonz = 0;
    if ( (origipaddr= myipaddr) == 0 )
        origipaddr = "127.0.0.1";
    if ( mypeer == 0 )
        myipaddr = "127.0.0.1";
    HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
    {
        now = (uint32_t)time(NULL);
#ifdef bruteforce
        if ( IAMLP != 0 && coin->inactive == 0 && coin->electrum == 0 && didinstantdex == 0 && strcmp("KMD",coin->symbol) == 0 )
        {
            LP_instantdex_deposits(coin);
            didinstantdex = now;
        }
#endif
        /*if ( (coin->addr_listunspent_requested != 0 && now > coin->lastpushtime+LP_ORDERBOOK_DURATION*.5) || now > coin->lastpushtime+LP_ORDERBOOK_DURATION*5 )
        {
            //printf("PUSH addr_listunspent_requested %u\n",coin->addr_listunspent_requested);
            coin->lastpushtime = (uint32_t)now;
            LP_smartutxos_push(coin);
            coin->addr_listunspent_requested = 0;
        }*/
        if ( coin->electrum == 0 && coin->inactive == 0 && now > coin->lastgetinfo+LP_GETINFO_INCR )
        {
            nonz++;
            if ( (height= LP_getheight(&notarized,coin)) > coin->longestchain )
            {
                coin->longestchain = height;
                if ( notarized != 0 && notarized > coin->notarized )
                {
                    coin->notarized = notarized;
                    if ( IAMLP != 0 )
                        LP_dPoW_broadcast(coin);
                }
                if ( 0 && coin->firstrefht != 0 )
                    printf(">>>>>>>>>> set %s longestchain %d (ref.%d [%d, %d])\n",coin->symbol,height,coin->firstrefht,coin->firstscanht,coin->lastscanht);
            } //else LP_mempoolscan(coin->symbol,zero);
            coin->lastgetinfo = (uint32_t)now;
        }
    }
    counter++;
    return(nonz);
}

int my_strncasecmp(const char *s1,const char *s2,size_t n)
{
    size_t i = 0;
    while ( i < n )
    {
        char c1 = s1[i];
        char c2 = s2[i];
        if ( c1 >= 'A' && c1 <= 'Z')
            c1 = (c1 - 'A') + 'a';
        if ( c2 >= 'A' && c2 <= 'Z')
            c2 = (c2 - 'A') + 'a';
        if ( c1 < c2 )
            return(-1);
        if ( c1 > c2 )
            return(1);
        if ( c1 == 0 )
            return(0);
        ++i;
    }
    return(0);
}

void bech32_tests()
{
    //char *test = "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs";
    //char *test = "bitcoincash:qpm2qsznhks23z7629mms6s4cwef74vcwvy22gdx6a";
    //char *test = "bitcoincash:qr95sy3j9xwd2ap32xkykttr4cvcu7as4y0qverfuy";
    //char *test = "prefix:x64nx6hz";
    char *test = "bitcoincash:pq4p38fll9uuh2mzkesqhmgt66du4u0zzy92jchqqa"; // 35Xbnq3kLoNsjN67knFewiRc9fqewrCzMW
    uint8_t data[82],data2[64],rmd160[21],addrtype; char rebuild[92],hrp[84]; int32_t data_len,data_len2; int32_t i;
    if ( bech32_decode(hrp,data,&data_len,test) == 0 )
    {
        printf("bech32_decode fails: '%s'\n",test);
    }
    else
    {
        bitcoin_addr2rmd160("BCH",0,&addrtype,rmd160,"pq4p38fll9uuh2mzkesqhmgt66du4u0zzy92jchqqa");
        bitcoin_address("BTC",rebuild,0,5,rmd160,20);
        for (i=0; i<20; i++)
            printf("%02x",rmd160[i]);
        printf("addr2rmd160 %d -> %s\n",addrtype,rebuild);
        
        data_len2 = 0;
        if ( bech32_convert_bits(data2,&data_len2,8,data,data_len,5,0) == 0 )
            printf("error converting data5\n");
        for (i=0; i<data_len2; i++)
            printf("%02x",data2[i]);
        printf(" compacted 5's -> %d\n",data_len2);
        bitcoin_addr2rmd160("BTC",0,&addrtype,rmd160+1,"35Xbnq3kLoNsjN67knFewiRc9fqewrCzMW");
        for (i=0; i<data_len; i++)
            printf("%02x",data[i]);
        printf(" datalen.%d <- %s (%s) -> ",(int32_t)data_len,test,"35Xbnq3kLoNsjN67knFewiRc9fqewrCzMW");
        for (i=0; i<20; i++)
            printf("%02x",rmd160[i+1]);
        printf("\n");
    }
    data_len2 = 0;
    rmd160[0] = (1 << 3);
    bech32_convert_bits(data2,&data_len2,5,rmd160,21,8,1);
    for (i=0; i<data_len2; i++)
        printf("%02x",data2[i]);
    printf(" converted bits.%d\n",(int32_t)data_len2);
    if ( bech32_encode(rebuild,hrp,data2,data_len2) == 0 )
    {
        for (i=0; i<data_len; i++)
            printf("%02x",data[i]);
        printf(" bech32_encode fails: '%s' -> hrp.(%s) datalen.%d\n",test,hrp,(int32_t)data_len);
    }
    if ( my_strncasecmp(rebuild,test,92))
    {
        printf("bech32_encode produces incorrect result: '%s' vs (%s)\n",test,rebuild);
    }
    printf("end of bech32 tests\n");
}

char *Notaries_elected1[][4] =
{
    {"0dev1_jl777", "03b7621b44118017a16043f19b30cc8a4cfe068ac4e42417bae16ba460c80f3828", "RNJmgYaFF5DbnrNUX6pMYz9rcnDKC2tuAc", "GWsW2A1ud72KoKJZysVLtEAYmgYZZzbMxG" },
    {"0dev2_kolo", "030f34af4b908fb8eb2099accb56b8d157d49f6cfb691baa80fdd34f385efed961" },
    {"0dev3_kolo", "025af9d2b2a05338478159e9ac84543968fd18c45fd9307866b56f33898653b014" },
    {"0dev4_decker", "028eea44a09674dda00d88ffd199a09c9b75ba9782382cc8f1e97c0fd565fe5707" },
    {"a-team_SH", "03b59ad322b17cb94080dc8e6dc10a0a865de6d47c16fb5b1a0b5f77f9507f3cce" },
    {"artik_AR", "029acf1dcd9f5ff9c455f8bb717d4ae0c703e089d16cf8424619c491dff5994c90" },
    {"artik_EU", "03f54b2c24f82632e3cdebe4568ba0acf487a80f8a89779173cdb78f74514847ce" },
    {"artik_NA", "0224e31f93eff0cc30eaf0b2389fbc591085c0e122c4d11862c1729d090106c842" },
    {"artik_SH", "02bdd8840a34486f38305f311c0e2ae73e84046f6e9c3dd3571e32e58339d20937" },
    {"badass_EU", "0209d48554768dd8dada988b98aca23405057ac4b5b46838a9378b95c3e79b9b9e" },
    {"badass_NA", "02afa1a9f948e1634a29dc718d218e9d150c531cfa852843a1643a02184a63c1a7" }, // 10
    {"batman_AR", "033ecb640ec5852f42be24c3bf33ca123fb32ced134bed6aa2ba249cf31b0f2563" },
    {"batman_SH", "02ca5898931181d0b8aafc75ef56fce9c43656c0b6c9f64306e7c8542f6207018c" },
    {"ca333_EU", "03fc87b8c804f12a6bd18efd43b0ba2828e4e38834f6b44c0bfee19f966a12ba99" },
    {"chainmakers_EU", "02f3b08938a7f8d2609d567aebc4989eeded6e2e880c058fdf092c5da82c3bc5ee" },
    {"chainmakers_NA", "0276c6d1c65abc64c8559710b8aff4b9e33787072d3dda4ec9a47b30da0725f57a" },
    {"chainstrike_SH", "0370bcf10575d8fb0291afad7bf3a76929734f888228bc49e35c5c49b336002153" },
    {"cipi_AR", "02c4f89a5b382750836cb787880d30e23502265054e1c327a5bfce67116d757ce8" },
    {"cipi_NA", "02858904a2a1a0b44df4c937b65ee1f5b66186ab87a751858cf270dee1d5031f18" },
    {"crackers_EU", "03bc819982d3c6feb801ec3b720425b017d9b6ee9a40746b84422cbbf929dc73c3" },
    {"crackers_NA", "03205049103113d48c7c7af811b4c8f194dafc43a50d5313e61a22900fc1805b45" }, // 20
    {"dwy_EU", "0259c646288580221fdf0e92dbeecaee214504fdc8bbdf4a3019d6ec18b7540424" },
    {"emmanux_SH", "033f316114d950497fc1d9348f03770cd420f14f662ab2db6172df44c389a2667a" },
    {"etszombi_EU", "0281b1ad28d238a2b217e0af123ce020b79e91b9b10ad65a7917216eda6fe64bf7" },
    {"fullmoon_AR", "03380314c4f42fa854df8c471618751879f9e8f0ff5dbabda2bd77d0f96cb35676" },
    {"fullmoon_NA", "030216211d8e2a48bae9e5d7eb3a42ca2b7aae8770979a791f883869aea2fa6eef" },
    {"fullmoon_SH", "03f34282fa57ecc7aba8afaf66c30099b5601e98dcbfd0d8a58c86c20d8b692c64" },
    {"goldenman_EU", "02d6f13a8f745921cdb811e32237bb98950af1a5952be7b3d429abd9152f8e388d" },
    {"indenodes_AR", "02ec0fa5a40f47fd4a38ea5c89e375ad0b6ddf4807c99733c9c3dc15fb978ee147" },
    {"indenodes_EU", "0221387ff95c44cb52b86552e3ec118a3c311ca65b75bf807c6c07eaeb1be8303c" },
    {"indenodes_NA", "02698c6f1c9e43b66e82dbb163e8df0e5a2f62f3a7a882ca387d82f86e0b3fa988" }, // 30
    {"indenodes_SH", "0334e6e1ec8285c4b85bd6dae67e17d67d1f20e7328efad17ce6fd24ae97cdd65e" },
    {"jackson_AR", "038ff7cfe34cb13b524e0941d5cf710beca2ffb7e05ddf15ced7d4f14fbb0a6f69" },
    {"jeezy_EU", "023cb3e593fb85c5659688528e9a4f1c4c7f19206edc7e517d20f794ba686fd6d6" },
    {"karasugoi_NA", "02a348b03b9c1a8eac1b56f85c402b041c9bce918833f2ea16d13452309052a982" },
    {"komodoninja_EU", "038e567b99806b200b267b27bbca2abf6a3e8576406df5f872e3b38d30843cd5ba" },
    {"komodoninja_SH", "033178586896915e8456ebf407b1915351a617f46984001790f0cce3d6f3ada5c2" },
    {"komodopioneers_SH", "033ace50aedf8df70035b962a805431363a61cc4e69d99d90726a2d48fb195f68c" },
    {"libscott_SH", "03301a8248d41bc5dc926088a8cf31b65e2daf49eed7eb26af4fb03aae19682b95" },
    {"lukechilds_AR", "031aa66313ee024bbee8c17915cf7d105656d0ace5b4a43a3ab5eae1e14ec02696" },
    {"madmax_AR", "03891555b4a4393d655bf76f0ad0fb74e5159a615b6925907678edc2aac5e06a75" }, // 40
    {"meshbits_AR", "02957fd48ae6cb361b8a28cdb1b8ccf5067ff68eb1f90cba7df5f7934ed8eb4b2c" },
    {"meshbits_SH", "025c6e94877515dfd7b05682b9cc2fe4a49e076efe291e54fcec3add78183c1edb" },
    {"metaphilibert_AR", "02adad675fae12b25fdd0f57250b0caf7f795c43f346153a31fe3e72e7db1d6ac6" },
    {"metaphilibert_SH", "0284af1a5ef01503e6316a2ca4abf8423a794e9fc17ac6846f042b6f4adedc3309" },
    {"patchkez_SH", "0296270f394140640f8fa15684fc11255371abb6b9f253416ea2734e34607799c4" },
    {"pbca26_NA", "0276aca53a058556c485bbb60bdc54b600efe402a8b97f0341a7c04803ce204cb5" },
    {"peer2cloud_AR", "034e5563cb885999ae1530bd66fab728e580016629e8377579493b386bf6cebb15" },
    {"peer2cloud_SH", "03396ac453b3f23e20f30d4793c5b8ab6ded6993242df4f09fd91eb9a4f8aede84" },
    {"polycryptoblog_NA", "02708dcda7c45fb54b78469673c2587bfdd126e381654819c4c23df0e00b679622" },
    {"hyper_AR", "020f2f984d522051bd5247b61b080b4374a7ab389d959408313e8062acad3266b4" }, // 50
    {"hyper_EU", "03d00cf9ceace209c59fb013e112a786ad583d7de5ca45b1e0df3b4023bb14bf51" },
    {"hyper_SH", "0383d0b37f59f4ee5e3e98a47e461c861d49d0d90c80e9e16f7e63686a2dc071f3" },
    {"hyper_NA", "03d91c43230336c0d4b769c9c940145a8c53168bf62e34d1bccd7f6cfc7e5592de" },
    {"popcornbag_AR", "02761f106fb34fbfc5ddcc0c0aa831ed98e462a908550b280a1f7bd32c060c6fa3" },
    {"popcornbag_NA", "03c6085c7fdfff70988fda9b197371f1caf8397f1729a844790e421ee07b3a93e8" },
    {"alien_AR", "0348d9b1fc6acf81290405580f525ee49b4749ed4637b51a28b18caa26543b20f0" },
    {"alien_EU", "020aab8308d4df375a846a9e3b1c7e99597b90497efa021d50bcf1bbba23246527" },
    {"thegaltmines_NA", "031bea28bec98b6380958a493a703ddc3353d7b05eb452109a773eefd15a32e421" },
    {"titomane_AR", "029d19215440d8cb9cc6c6b7a4744ae7fb9fb18d986e371b06aeb34b64845f9325" },
    {"titomane_EU", "0360b4805d885ff596f94312eed3e4e17cb56aa8077c6dd78d905f8de89da9499f" }, // 60
    {"titomane_SH", "03573713c5b20c1e682a2e8c0f8437625b3530f278e705af9b6614de29277a435b" },
    {"webworker01_NA", "03bb7d005e052779b1586f071834c5facbb83470094cff5112f0072b64989f97d7" },
    {"xrobesx_NA", "03f0cc6d142d14a40937f12dbd99dbd9021328f45759e26f1877f2a838876709e1" },
};

void gameaddrs()
{
    struct iguana_info *gamecoin,*kmdcoin; int32_t i; uint8_t pubkey33[33]; char gameaddr[64],kmdaddr[64];
    gamecoin = LP_coinfind("GAME");
    kmdcoin = LP_coinfind("KMD");
    if ( gamecoin != 0 && kmdcoin != 0 )
    {
        for (i=0; i<64; i++)
        {
            decode_hex(pubkey33,33,Notaries_elected1[i][1]);
            bitcoin_address(gamecoin->symbol,gameaddr,gamecoin->taddr,gamecoin->pubtype,pubkey33,33);
            bitcoin_address(kmdcoin->symbol,kmdaddr,kmdcoin->taddr,kmdcoin->pubtype,pubkey33,33);
            printf("{\"%s\", \"%s\", \"%s\", \"%s\"},\n",Notaries_elected1[i][0],Notaries_elected1[i][1],kmdaddr,gameaddr);
        }
    }
}

void emc2addrs()
{
    struct iguana_info *emc2coin,*kmdcoin; int32_t i; uint8_t pubkey33[33]; char emc2addr[64],kmdaddr[64];
    emc2coin = LP_coinfind("EMC2");
    kmdcoin = LP_coinfind("KMD");
    if ( emc2coin != 0 && kmdcoin != 0 )
    {
        for (i=0; i<64; i++)
        {
            decode_hex(pubkey33,33,Notaries_elected1[i][1]);
            bitcoin_address(emc2coin->symbol,emc2addr,emc2coin->taddr,emc2coin->pubtype,pubkey33,33);
            bitcoin_address(kmdcoin->symbol,kmdaddr,kmdcoin->taddr,kmdcoin->pubtype,pubkey33,33);
            printf("{\"%s\", \"%s\", \"%s\", \"%s\"},\n",Notaries_elected1[i][0],Notaries_elected1[i][1],kmdaddr,emc2addr);
        }
    }
}


void LP_initcoins(void *ctx,int32_t pubsock,cJSON *coins)
{
    int32_t i,n,notarized; cJSON *item; char *symbol,*etomic; struct iguana_info *coin;
    for (i=0; i<sizeof(activecoins)/sizeof(*activecoins); i++)
    {
        printf("%s, ",activecoins[i]);
        LP_coinfind(activecoins[i]);
        LP_priceinfoadd(activecoins[i]);
        if ( (coin= LP_coinfind(activecoins[i])) != 0 )
        {
            if ( LP_getheight(&notarized,coin) <= 0 )
                coin->inactive = (uint32_t)time(NULL);
            else
            {
                LP_unspents_load(coin->symbol,coin->smartaddr);
                if ( strcmp(coin->symbol,"KMD") == 0 )
                {
                    LP_importaddress("KMD",BOTS_BONDADDRESS);
                    LP_dPoW_request(coin);
                }
            }
            if ( coin->txfee == 0 && strcmp(coin->symbol,"BTC") != 0 )
                coin->txfee = LP_MIN_TXFEE;
        }
    }
    if ( (n= cJSON_GetArraySize(coins)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = jitem(coins,i);
            if ( (symbol= jstr(item,"coin")) != 0 )
            {
                printf("%s.%d ",jstr(item,"coin"),LP_numpriceinfos);
                LP_coincreate(item);
                LP_priceinfoadd(jstr(item,"coin"));
                if ( (coin= LP_coinfind(symbol)) != 0 )
                {
                    if ( (etomic= jstr(item,"etomic")) != 0 )
                        safecopy(coin->etomic,etomic,sizeof(coin->etomic));
                    else
                    {
                        if ( LP_getheight(&notarized,coin) <= 0 )
                            coin->inactive = (uint32_t)time(NULL);
                        else LP_unspents_load(coin->symbol,coin->smartaddr);
                    }
                    if ( coin->txfee == 0 && strcmp(coin->symbol,"BTC") != 0 )
                        coin->txfee = LP_MIN_TXFEE;
                    if ( 0 && strcmp(coin->symbol,"BCH") == 0 )
                    {
                        bech32_tests();
                    }
                    else if ( 0 && strcmp(coin->symbol,"GAME") == 0 )
                    {
                        gameaddrs();
                    }
                    else if ( 0 && strcmp(coin->symbol,"EMC2") == 0 )
                    {
                        emc2addrs();
                    }
                    else if ( 0 && strcmp(coin->symbol,"SMART") == 0 )
                    {
                        uint8_t txdata[8129]; int32_t len; bits256 txid,txid2,ktxid; char str[65];
                        char *txstr = "0100000005c9a9c56f4e702766c582127587ee49695ba5b9e5c449290a4f8a0505c12beeb7000000006b483045022100867f85d9f8d7f543225448f1d2383ff7d60a325b5f643557ea36325372de9993022034ccd202ee017c3d8a2dfa615b72d661b891d2071b1c15d4d7ab063762c79d2f012103f9be43471012e3e6daaa7a91a68cbc667fa61791e4a93f8e8d53255a93b68d03ffffffffe062eb6845b69856d62ceb3fd9b7e05c383b8b88b9359bc7c8ac49cd7dd2b2bb010000006b483045022100d4ef1c6d5f24ad3877f57f8ae4a1c0f8aa2f6de5ccaa8ed8e10b2937a4316c1402200d41b154a892a98d40c4d39e619992cca1eba4b5939bbf2b418062cd1275a8b101210302649cc91eda9d5fbc9d41b4a14f98917a00cdc6fc952c9fdf8a98a544cfcca9ffffffff561ee4189323bf8f619bcedd1f9e02033adb588db31cb3dc7ec3bda23f73aac4000000006b483045022100d4a5ab03675f585cc055c76fdaa80333757c42583883dfcde08cb0aeb57f256a022063440e24ef4ef5b44dcb0e7aacaa4562effd80f9a1d9b273daa581066c8359fb0121035c0e6d900a5e8c27901ce7edcdc9bfafea44dbfb714274271114245d7e895198ffffffff56cf1fa6d5779fd2bded063d63455809373ff2bf79edf921be7e92f918ef43df010000006b48304502210091d15d5fcb518103f04fdc819513200d30033f9a1e29960458375e7c71247d31022064a233c4073ac88652a0324f6973914b08bfcf76e928afae0099dc2d7eb227750121032946d47c35c0a98ae7ccad30fee846007434fff25cf70973b5c76deb4be2a14fffffffff4754ee214a33da0116730b43238cc6ff0510b8cebc59a42b1f0609e2a10aa8c8000000006b483045022100f3cea95cd6451d706fb1766cec30450ef000649df15d7d7bebcecbfe6ec48e98022016e687a7d956d2ab089d76306046d33ce5ace55fcb4595021de3f97c4293d1b601210367db63755cf13760c81b8cb0c13eee10474064dfacc7c45a8c3271b5b058f0ccffffffff025e489467030000001976a9147283e4813a5e5fb8d723e75403ba10e05a7820db88ac60f71b00000000001976a914d740ff057317d3b12d5a6dadac5bb7ff87e48afe88ac578a0500";
                        len = (int32_t)strlen(txstr) >> 1;
                        decode_hex(txdata,len,txstr);
                        vcalc_sha256(0,txid.bytes,txdata,len);
                        txid2 = bits256_doublesha256(0,txdata,len);
                        HashKeccak(ktxid.bytes,txdata,len);
                        printf("txid %s\n",bits256_str(str,txid));
                        printf("txid2 %s\n",bits256_str(str,txid2));
                        printf("ktxid %s\n",bits256_str(str,ktxid));
                    }
                }
            }
        }
        for (i=0; i<n; i++)
        {
            item = jitem(coins,i);
            printf("\"%s\", ",jstr(item,"coin"));
        }
    }
    printf("privkey updates\n");
}

void LP_initpeers(int32_t pubsock,struct LP_peerinfo *mypeer,char *myipaddr,uint16_t myport,uint16_t netid,char *seednode)
{
    int32_t i,j; uint32_t r; uint16_t pushport,subport,busport; char fixedseed[64];
    LP_ports(&pushport,&subport,&busport,netid);
    if ( IAMLP != 0 )
    {
        LP_mypeer = mypeer = LP_addpeer(mypeer,pubsock,myipaddr,myport,pushport,subport,1,G.LP_sessionid,netid);
        if ( myipaddr == 0 || mypeer == 0 )
        {
            printf("couldnt get myipaddr or null mypeer.%p\n",mypeer);
            exit(-1);
        }
        if ( seednode == 0 || seednode[0] == 0 )
        {
            if ( netid == 0 )
            {
                printf("load default seednodes\n");
                for (i=0; i<sizeof(default_LPnodes)/sizeof(*default_LPnodes); i++)
                {
                    LP_addpeer(mypeer,pubsock,default_LPnodes[i],myport,pushport,subport,0,G.LP_sessionid,netid);
                }
            }
        } else LP_addpeer(mypeer,pubsock,seednode,myport,pushport,subport,1,G.LP_sessionid,netid);
    }
    else
    {
        if ( myipaddr == 0 )
        {
            printf("couldnt get myipaddr\n");
            exit(-1);
        }
        if ( (netid > 0 && netid < 9) && (seednode == 0 || seednode[0] == 0) )
        {
            if ( (netid & 1) != 0 )
                strcpy(fixedseed,"46.4.78.11");
            else strcpy(fixedseed,"46.4.87.18");
            //sprintf(fixedseed,"5.9.253.%d",195 + netid);
            seednode = fixedseed;
        }
        if ( seednode == 0 || seednode[0] == 0 )
        {
            printf("default seed nodes for netid.%d\n",netid);
            OS_randombytes((void *)&r,sizeof(r));
            r = 0;
            for (j=0; j<sizeof(default_LPnodes)/sizeof(*default_LPnodes); j++)
            {
                i = (r + j) % (sizeof(default_LPnodes)/sizeof(*default_LPnodes));
                LP_addpeer(mypeer,pubsock,default_LPnodes[i],myport,pushport,subport,0,G.LP_sessionid,netid);
            }
        } else LP_addpeer(mypeer,pubsock,seednode,myport,pushport,subport,1,G.LP_sessionid,netid);
    }
}

void LP_pubkeysloop(void *ctx)
{
    static uint32_t lasttime;
    strcpy(LP_pubkeysloop_stats.name,"LP_pubkeysloop");
    LP_pubkeysloop_stats.threshold = 15000.;
    sleep(10);
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        {
            LP_millistats_update(&LP_pubkeysloop_stats);
            if ( time(NULL) > lasttime+100 )
            {
                //printf("LP_pubkeysloop %u\n",(uint32_t)time(NULL));
                LP_notify_pubkeys(ctx,LP_mypubsock);
                lasttime = (uint32_t)time(NULL);
            }
        }
        sleep(3);
    }
}

struct LP_pendswap
{
    struct LP_pendswap *next,*prev;
    uint32_t expiration,requestid,quoteid,finished;
};

struct LP_pendswap *LP_pendingswaps;

void LP_pendswap_add(uint32_t expiration,uint32_t requestid,uint32_t quoteid)
{
    struct LP_pendswap *sp;
    printf("LP_pendswap_add expiration.%u %u-%u\n",expiration,requestid,quoteid);
    portable_mutex_lock(&LP_pendswap_mutex);
    sp = calloc(1,sizeof(*sp));
    sp->expiration = expiration;
    sp->requestid = requestid;
    sp->quoteid = quoteid;
    DL_APPEND(LP_pendingswaps,sp);
    portable_mutex_unlock(&LP_pendswap_mutex);
}

void LP_swapsloop(void *ctx)
{
    char *retstr; cJSON *retjson; uint32_t requestid,quoteid; int32_t i,nonz; struct LP_pendswap *sp,*tmp;
    strcpy(LP_swapsloop_stats.name,"LP_swapsloop");
    LP_swapsloop_stats.threshold = 605000.;
    if ( (retstr= basilisk_swapentry(0,0,0,1)) != 0 )
    {
        if ( (retjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( (requestid= juint(retjson,"requestid")) != 0 && (quoteid= juint(retjson,"quoteid")) != 0 && jobj(retjson,"error") == 0 )
                LP_pendswap_add(0,requestid,quoteid);
        }
        free(retstr);
    }
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        LP_millistats_update(&LP_swapsloop_stats);
        nonz = 0;
        DL_FOREACH_SAFE(LP_pendingswaps,sp,tmp)
        {
            if ( sp->finished == 0 )
            {
                nonz++;
                if ( (sp->finished= LP_swapwait(0,sp->requestid,sp->quoteid,-1,0)) != 0 )
                {
                }
                sleep(3);
            }
        }
        if ( nonz == 0 )
        {
            for (i=0; i<10; i++)
            {
                //fprintf(stderr,"check on alice expiration\n");
                LP_alice_eligible((uint32_t)time(NULL));
                sleep(6);
            }
        } else sleep(10);
        LP_gtc_iteration(ctx,LP_myipaddr,LP_mypubsock);
    }
}

void gc_loop(void *ctx)
{
    uint32_t now; struct LP_address_utxo *up,*utmp; struct rpcrequest_info *req,*rtmp; int32_t flag = 0;
    strcpy(LP_gcloop_stats.name,"gc_loop");
    LP_gcloop_stats.threshold = 11000.;
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        flag = 0;
        LP_millistats_update(&LP_gcloop_stats);
        portable_mutex_lock(&LP_gcmutex);
        DL_FOREACH_SAFE(LP_garbage_collector,req,rtmp)
        {
            DL_DELETE(LP_garbage_collector,req);
            //printf("garbage collect ipbits.%x\n",req->ipbits);
            free(req);
            flag++;
        }
        now = (uint32_t)time(NULL);
        DL_FOREACH_SAFE(LP_garbage_collector2,up,utmp)
        {
            if ( now > (uint32_t)up->spendheight+120 )
            {
                DL_DELETE(LP_garbage_collector2,up);
                //char str[65]; printf("garbage collect %s/v%d lag.%d\n",bits256_str(str,up->U.txid),up->U.vout,now-up->spendheight);
                free(up);
            }
            flag++;
        }
        portable_mutex_unlock(&LP_gcmutex);
        if ( 0 && flag != 0 )
            printf("gc_loop.%d\n",flag);
        sleep(10);
    }
}

void queue_loop(void *ctx)
{
    struct LP_queue *ptr,*tmp; cJSON *json; uint8_t linebuf[32768]; int32_t k,sentbytes,nonz,flag,duplicate,n=0;
    strcpy(queue_loop_stats.name,"queue_loop");
    queue_loop_stats.threshold = 1000.;
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        LP_millistats_update(&queue_loop_stats);
        n = nonz = flag = 0;
        DL_FOREACH_SAFE(LP_Q,ptr,tmp)
        {
            n++;
            flag = 0;
            if ( ptr->sock >= 0 )
            {
                //printf("sock.%d len.%d notready.%d\n",ptr->sock,ptr->msglen,ptr->notready);
                if ( ptr->notready == 0 || (LP_rand() % ptr->notready) == 0 )
                {
                    if ( LP_sockcheck(ptr->sock) > 0 )
                    {
                        //bits256 magic;
                        //magic = LP_calc_magic(ptr->msg,(int32_t)(ptr->msglen - sizeof(bits256)));
                        //memcpy(&ptr->msg[ptr->msglen - sizeof(bits256)],&magic,sizeof(magic));
                        if ( 0 )
                        {
                            static FILE *fp;
                            if ( fp == 0 )
                                fp = fopen("packet.log","wb");
                            if ( fp != 0 )
                            {
                                fprintf(fp,"%s\n",(char *)ptr->msg);
                                fflush(fp);
                            }
                        }
                        if ( (json= cJSON_Parse((char *)ptr->msg)) != 0 )
                        {
                            if ( ptr->msglen < sizeof(linebuf) )
                            {
                                if ( (k= MMJSON_encode(linebuf,(char *)ptr->msg)) > 0 )
                                {
                                    if ( (sentbytes= nn_send(ptr->sock,linebuf,k,0)) != k )
                                        printf("%d LP_send mmjson sent %d instead of %d\n",n,sentbytes,k);
                                    else
                                    {
                                        flag++;
                                        ptr->sock = -1;
                                    }
                                }
                                //printf("k.%d flag.%d SEND.(%s) sock.%d\n",k,flag,(char *)ptr->msg,ptr->sock);
                            }
                            free_json(json);
                        }
                        if ( flag == 0 )
                        {
                           // printf("non-encoded len.%d SEND.(%s) sock.%d\n",ptr->msglen,(char *)ptr->msg,ptr->sock);
                            if ( (sentbytes= nn_send(ptr->sock,ptr->msg,ptr->msglen,0)) != ptr->msglen )
                                printf("%d LP_send sent %d instead of %d\n",n,sentbytes,ptr->msglen);
                            else
                            {
                                flag++;
                                ptr->sock = -1;
                            }
                        }
                        if ( ptr->peerind > 0 )
                            ptr->starttime = (uint32_t)time(NULL);
                    }
                    else
                    {
                        if ( ptr->notready++ > 100 )
                        {
                            flag = 1;
                            //printf("queue_loop sock.%d len.%d notready.%d, skip\n",ptr->sock,ptr->msglen,ptr->notready);
                            ptr->sock = -1;
                        }
                    }
                }
            }
            else if ( 0 && time(NULL) > ptr->starttime+13 )
            {
                LP_crc32find(&duplicate,-1,ptr->crc32);
                if ( duplicate > 0 )
                {
                    LP_Qfound++;
                    if ( (LP_Qfound % 100) == 0 )
                        printf("found.%u Q.%d err.%d match.%d\n",ptr->crc32,LP_Qenqueued,LP_Qerrors,LP_Qfound);
                    flag++;
                }
                else if ( 0 ) // too much beyond duplicate filter when network is busy
                {
                    printf("couldnt find.%u peerind.%d Q.%d err.%d match.%d\n",ptr->crc32,ptr->peerind,LP_Qenqueued,LP_Qerrors,LP_Qfound);
                    ptr->peerind++;
                    if ( (ptr->sock= LP_peerindsock(&ptr->peerind)) < 0 )
                    {
                        printf("%d no more peers to try at peerind.%d %p Q_LP.%p\n",n,ptr->peerind,ptr,LP_Q);
                        flag++;
                        LP_Qerrors++;
                    }
                }
            }
            if ( flag != 0 )
            {
                nonz++;
                portable_mutex_lock(&LP_networkmutex);
                DL_DELETE(LP_Q,ptr);
                portable_mutex_unlock(&LP_networkmutex);
                free(ptr);
                ptr = 0;
                break;
            }
        }
        if ( nonz == 0 )
        {
            if ( IAMLP == 0 )
                usleep(50000);
            else usleep(10000);
        }
    }
}

void LP_reserved_msgs(void *ignore)
{
    bits256 zero; int32_t flag,nonz; struct nn_pollfd pfd;
    memset(zero.bytes,0,sizeof(zero));
    strcpy(LP_reserved_msgs_stats.name,"LP_reserved_msgs");
    LP_reserved_msgs_stats.threshold = 1000.;
    while ( LP_STOP_RECEIVED == 0 )
    {
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        nonz = 0;
        LP_millistats_update(&LP_reserved_msgs_stats);
        if ( num_Reserved_msgs[1] > 0 )
        {
            nonz++;
            portable_mutex_lock(&LP_reservedmutex);
            if ( num_Reserved_msgs[1] > 0 )
            {
                num_Reserved_msgs[1]--;
                //printf("PRIORITY BROADCAST.(%s)\n",Reserved_msgs[1][num_Reserved_msgs[1]]);
                LP_broadcast_message(LP_mypubsock,"","",zero,Reserved_msgs[1][num_Reserved_msgs[1]]);
                Reserved_msgs[1][num_Reserved_msgs[1]] = 0;
            }
            portable_mutex_unlock(&LP_reservedmutex);
        }
        else if ( num_Reserved_msgs[0] > 0 )
        {
            nonz++;
            flag = 0;
            if ( flag == 0 && LP_mypubsock >= 0 )
            {
                memset(&pfd,0,sizeof(pfd));
                pfd.fd = LP_mypubsock;
                pfd.events = NN_POLLOUT;
                if ( nn_poll(&pfd,1,1) == 1 )
                    flag = 1;
            } else flag = 1;
            if ( flag == 1 )
            {
                portable_mutex_lock(&LP_reservedmutex);
                num_Reserved_msgs[0]--;
                //printf("BROADCAST.(%s)\n",Reserved_msgs[0][num_Reserved_msgs[0]]);
                LP_broadcast_message(LP_mypubsock,"","",zero,Reserved_msgs[0][num_Reserved_msgs[0]]);
                Reserved_msgs[0][num_Reserved_msgs[0]] = 0;
                portable_mutex_unlock(&LP_reservedmutex);
            }
        }
        if ( ignore == 0 )
            break;
        if ( nonz == 0 )
            usleep(5000);
    }
}

int32_t LP_reserved_msg(int32_t priority,char *base,char *rel,bits256 pubkey,char *msg)
{
    struct LP_pubkey_info *pubp; uint32_t timestamp; char *method; cJSON *argjson; int32_t skip,sentbytes,n = 0;
    skip = 0;
    if ( (argjson= cJSON_Parse(msg)) != 0 )
    {
        if ( (method= jstr(argjson,"method")) != 0 )
        {
            if ( strcmp(method,"gettradestatus") == 0 || strcmp(method,"wantnotify") == 0 || strcmp(method,"getdPoW") == 0 )
                skip = 1;
        }
        if ( (timestamp= juint(argjson,"timestamp")) != 0 && time(NULL) > timestamp+60 )
            skip = 1;
        free_json(argjson);
    }
    if ( skip != 0 )
        return(-1);
    //if ( strcmp(G.USERPASS,"1d8b27b21efabcd96571cd56f91a40fb9aa4cc623d273c63bf9223dc6f8cd81f") == 0 )
    //    return(-1);
    if ( priority > 0 && bits256_nonz(pubkey) != 0 )
    {
        if ( (pubp= LP_pubkeyfind(pubkey)) != 0 )
        {
            if ( pubp->pairsock >= 0 )
            {
                if ( (sentbytes= nn_send(pubp->pairsock,msg,(int32_t)strlen(msg)+1,0)) < 0 )
                {
                    //pubp->pairsock = -1;
                    //LP_peer_pairsock(pubkey);
                    //printf("mark cmdchannel %d closed sentbytes.%d\n",pubp->pairsock,sentbytes);
                }
                else
                {
                    printf("sent %d bytes to cmdchannel.%d\n",sentbytes,pubp->pairsock);
                    return(sentbytes);
                }
            }
        }
    }
#if !defined(NDEBUG) && !defined(NOTETOMIC) && defined(__linux__)
    printf("Message %s\n", msg);
    print_stack_trace();
#endif
    portable_mutex_lock(&LP_reservedmutex);
    if ( num_Reserved_msgs[priority] < sizeof(Reserved_msgs[priority])/sizeof(*Reserved_msgs[priority]) )
    {
        Reserved_msgs[priority][num_Reserved_msgs[priority]++] = msg;
        n = num_Reserved_msgs[priority];
    } //else LP_broadcast_message(LP_mypubsock,base,rel,pubkey,msg);
    if ( num_Reserved_msgs[priority] > max_Reserved_msgs[priority] )
    {
        max_Reserved_msgs[priority] = num_Reserved_msgs[priority];
        //if ( (max_Reserved_msgs[priority] % 100) == 0 )
            printf("New priority.%d max_Reserved_msgs.%d\n",priority,max_Reserved_msgs[priority]);
    }
    portable_mutex_unlock(&LP_reservedmutex);
    return(n);
}

extern int32_t bitcoind_RPC_inittime;

void LPinit(uint16_t myport,uint16_t mypullport,uint16_t mypubport,uint16_t mybusport,char *passphrase,int32_t amclient,char *userhome,cJSON *argjson)
{
    char *myipaddr=0; long filesize,n; int32_t valid,timeout; struct LP_peerinfo *mypeer=0; char pushaddr[128],subaddr[128],bindaddr[128],*coins_str=0; cJSON *coinsjson=0; void *ctx = bitcoin_ctx();
    bitcoind_RPC_inittime = 1;
    if ( LP_MAXPRICEINFOS > 256 )
    {
        printf("LP_MAXPRICEINFOS %d wont fit in a uint8_t, need to increase the width of the baseind and relind for struct LP_pubkey_quote\n",LP_MAXPRICEINFOS);
        exit(-1);
    }
    LP_showwif = juint(argjson,"wif");
    printf("showwif.%d version: %s %u\n",LP_showwif,MM_VERSION,calc_crc32(0,MM_VERSION,(int32_t)strlen(MM_VERSION)));
    if ( passphrase == 0 || passphrase[0] == 0 )
    {
        printf("jeezy says we cant use the nullstring as passphrase and I agree\n");
        exit(-1);
    }
    IAMLP = !amclient;
#ifndef __linux__
    if ( IAMLP != 0 )
    {
        printf("must run a unix node for LP node\n");
        exit(-1);
    }
#endif
    OS_randombytes((void *)&n,sizeof(n));
    srand((uint32_t)n);
    if ( jobj(argjson,"gui") != 0 )
        safecopy(LP_gui,jstr(argjson,"gui"),sizeof(LP_gui));
    if ( jobj(argjson,"canbind") == 0 )
    {
#ifndef __linux__
        LP_canbind = IAMLP;
#else
        LP_canbind = IAMLP;
#endif
    }
    else
    {
        LP_canbind = jint(argjson,"canbind");
        printf(">>>>>>>>>>> set LP_canbind.%d\n",LP_canbind);
    }
    if ( LP_canbind > 1000 && LP_canbind < 65536 )
        LP_fixed_pairport = LP_canbind;
    if ( LP_canbind != 0 )
        LP_canbind = 1;
    srand((int32_t)n);
    if ( userhome != 0 && userhome[0] != 0 )
    {
        safecopy(USERHOME,userhome,sizeof(USERHOME));
#ifdef __APPLE__
        strcat(USERHOME,"/Library/Application Support");
#endif
    }
    portable_mutex_init(&LP_peermutex);
    portable_mutex_init(&LP_utxomutex);
    portable_mutex_init(&LP_UTXOmutex);
    portable_mutex_init(&LP_commandmutex);
    portable_mutex_init(&LP_swaplistmutex);
    portable_mutex_init(&LP_cachemutex);
    portable_mutex_init(&LP_networkmutex);
    portable_mutex_init(&LP_gcmutex);
    portable_mutex_init(&LP_forwardmutex);
    portable_mutex_init(&LP_inusemutex);
    portable_mutex_init(&LP_psockmutex);
    portable_mutex_init(&LP_coinmutex);
    portable_mutex_init(&LP_pubkeymutex);
    portable_mutex_init(&LP_electrummutex);
    portable_mutex_init(&LP_messagemutex);
    portable_mutex_init(&LP_portfoliomutex);
    portable_mutex_init(&LP_butxomutex);
    portable_mutex_init(&LP_reservedmutex);
    portable_mutex_init(&LP_nanorecvsmutex);
    portable_mutex_init(&LP_tradebotsmutex);
    portable_mutex_init(&LP_cJSONmutex);
    portable_mutex_init(&LP_logmutex);
    portable_mutex_init(&LP_statslogmutex);
    portable_mutex_init(&LP_tradesmutex);
    portable_mutex_init(&LP_commandQmutex);
    portable_mutex_init(&LP_blockinit_mutex);
    portable_mutex_init(&LP_pendswap_mutex);
    portable_mutex_init(&LP_listmutex);
    portable_mutex_init(&LP_gtcmutex);
    myipaddr = clonestr("127.0.0.1");
#ifndef _WIN32
#ifndef FROM_JS
    char ipfname[64];
    strcpy(ipfname,"myipaddr");
    if ( access( ipfname, F_OK ) != -1 || system("curl -s4 checkip.amazonaws.com > myipaddr") == 0 )
    {
        if ( (myipaddr= OS_filestr(&filesize,ipfname)) != 0 && myipaddr[0] != 0 )
        {
            n = strlen(myipaddr);
            if ( myipaddr[n-1] == '\n' )
                myipaddr[--n] = 0;
            strcpy(LP_myipaddr,myipaddr);
        } else printf("error getting myipaddr\n");
    } else printf("error issuing curl\n");
#else
    IAMLP = 0;
#endif
#endif
    if ( IAMLP != 0 )
    {
        G.netid = juint(argjson,"netid");
        LP_mypubsock = -1;
        nanomsg_transportname(0,subaddr,myipaddr,mypubport);
        nanomsg_transportname(1,bindaddr,myipaddr,mypubport);
        valid = 0;
        if ( (LP_mypubsock= nn_socket(AF_SP,NN_PUB)) >= 0 )
        {
            valid = 0;
            if ( nn_bind(LP_mypubsock,bindaddr) >= 0 )
                valid++;
            if ( valid > 0 )
            {
                timeout = 100;
                nn_setsockopt(LP_mypubsock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout));
                //timeout = 10;
                //nn_setsockopt(LP_mypubsock,NN_SOL_SOCKET,NN_MAXTTL,&timeout,sizeof(timeout));
            }
            else
            {
                printf("error binding to (%s).%d\n",subaddr,LP_mypubsock);
                if ( LP_mypubsock >= 0 )
                    nn_close(LP_mypubsock), LP_mypubsock = -1;
            }
        } else printf("error getting pubsock %d\n",LP_mypubsock);
        printf(">>>>>>>>> myipaddr.(%s) (%s) valid.%d pubbindaddr.%s pubsock.%d\n",bindaddr,subaddr,valid,bindaddr,LP_mypubsock);
        LP_mypullsock = LP_initpublicaddr(ctx,&mypullport,pushaddr,myipaddr,mypullport,0);
    }
    if ( (coinsjson= jobj(argjson,"coins")) == 0 )
    {
        if ( (coins_str= OS_filestr(&filesize,"coins.json")) != 0 || (coins_str= OS_filestr(&filesize,"exchanges/coins.json")) != 0 )
        {
            unstringify(coins_str);
            printf("UNSTRINGIFIED.(%s)\n",coins_str);
            coinsjson = cJSON_Parse(coins_str);
            free(coins_str);
            // yes I know this coinsjson is not freed, not sure about if it is referenced
        }
    }
    if ( coinsjson == 0 )
    {
        printf("no coins object or coins.json file, must abort\n");
        exit(-1);
    }
    LP_initcoins(ctx,LP_mypubsock,coinsjson);
    RPC_port = myport;
    G.waiting = 1;
        LP_initpeers(LP_mypubsock,LP_mypeer,LP_myipaddr,RPC_port,juint(argjson,"netid"),jstr(argjson,"seednode"));
    //LP_mypullsock = LP_initpublicaddr(ctx,&mypullport,pushaddr,myipaddr,mypullport,0);
    //strcpy(LP_publicaddr,pushaddr);
    //LP_publicport = mypullport;
    //LP_mybussock = LP_coinbus(mybusport);
    printf("got %s, initpeers. LP_mypubsock.%d pullsock.%d RPC_port.%u mypullport.%d mypubport.%d\n",myipaddr,LP_mypubsock,LP_mypullsock,RPC_port,mypullport,mypubport);
    LP_passphrase_init(passphrase,jstr(argjson,"gui"),juint(argjson,"netid"),jstr(argjson,"seednode"));
#ifndef FROM_JS
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_psockloop,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_psockloop for (%s)\n",myipaddr);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_reserved_msgs,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_reserved_msgs for (%s)\n",myipaddr);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&myport) != 0 )
    {
        printf("error launching stats rpcloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)command_rpcloop,ctx) != 0 )
    {
        printf("error launching command_rpcloop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)queue_loop,ctx) != 0 )
    {
        printf("error launching queue_loop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)gc_loop,ctx) != 0 )
    {
        printf("error launching gc_loop for port.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)prices_loop,ctx) != 0 )
    {
        printf("error launching prices_loop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_coinsloop,(void *)"") != 0 )
    {
        printf("error launching LP_coinsloop for (%s)\n","");
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_coinsloop,(void *)"BTC") != 0 )
    {
        printf("error launching LP_coinsloop for (%s)\n","BTC");
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_coinsloop,(void *)"KMD") != 0 )
    {
        printf("error launching LP_coinsloop for (%s)\n","KMD");
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_pubkeysloop,ctx) != 0 )
    {
        printf("error launching LP_pubkeysloop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_tradesloop,ctx) != 0 )
    {
        printf("error launching LP_tradessloop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_commandQ_loop,ctx) != 0 )
    {
        printf("error launching LP_commandQ_loop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_swapsloop,ctx) != 0 )
    {
        printf("error launching LP_swapsloop for ctx.%p\n",ctx);
        exit(-1);
    }
    int32_t nonz,didremote=0;
    LP_statslog_parse();
    bitcoind_RPC_inittime = 0;
    //LP_mpnet_init(); seems better to have the GUI send in persistent orders, exit mm is a cancel all
    while ( LP_STOP_RECEIVED == 0 )
    {
        nonz = 0;
        G.waiting = 1;
        while ( G.initializing != 0 ) //&& strcmp(G.USERPASS,"1d8b27b21efabcd96571cd56f91a40fb9aa4cc623d273c63bf9223dc6f8cd81f") == 0 )
        {
            //fprintf(stderr,".");
            sleep(3);
        }
        if ( G.initializing != 0 )
        {
            sleep(1);
            continue;
        }
        if ( LP_mainloop_iter(ctx,myipaddr,mypeer,LP_mypubsock) != 0 )
            nonz++;
        if ( IAMLP != 0 && didremote == 0 && LP_cmdcount > 0 )
        {
            didremote = 1;
            uint16_t myport2 = RPC_port-1;
            printf("start remote port\n");
            if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&myport2) != 0 )
            {
                printf("error launching stats rpcloop for port.%u\n",myport);
                exit(-1);
            }
        }
        if ( nonz == 0 )
            usleep(1000);
        else if ( IAMLP == 0 )
            usleep(1000);
    }
#endif
    printf("marketmaker exiting in 5 seconds\n");
    sleep(5);
    exit(0);
}

#ifdef FROM_JS
extern void *Nanomsg_threadarg;
void *nn_thread_main_routine(void *arg);

void emscripten_usleep(int32_t x)
{
}

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params,int32_t timeout)
{
    static uint32_t counter; char fname[512],*retstr; long fsize;
    if ( strncmp("http://",url,strlen("http://")) != 0 )
        return(clonestr("{\"error\":\"only http allowed\"}"));
    sprintf(fname,"bitcoind_RPC/request.%d",counter % 10);
    counter++;
    //printf("issue.(%s)\n",url);
    emscripten_wget(url,fname);
    retstr = OS_filestr(&fsize,fname);
    //printf("bitcoind_RPC(%s) -> fname.(%s) %s\n",url,fname,retstr);
    return(retstr);
}

char *barterDEX(char *argstr)
{
    static void *ctx;
    cJSON *argjson; char *retstr;
    if ( ctx == 0 )
        ctx = bitcoin_ctx();
    printf("barterDEX.(%s)\n",argstr);
    if ( (argjson= cJSON_Parse(argstr)) != 0 )
    {
        LP_queuecommand(&retstr,argstr,LP_mypubsock);
        //retstr = LP_command_process(ctx,LP_myipaddr,LP_mypubsock,argjson,(uint8_t *)argstr,(int32_t)strlen(argstr));
        while ( retstr == 0 )
            usleep(50000);
        free_json(argjson);
    } else retstr = clonestr("{\"error\":\"couldnt parse request\"}");
    return(retstr);
}

void LP_fromjs_iter()
{
    static void *ctx; char *retstr;
    if ( G.initializing != 0 )
    {
        printf("LP_fromjs_iter during G.initializing, skip\n");
        return;
    }
    if ( ctx == 0 )
        ctx = bitcoin_ctx();
    //if ( Nanomsg_threadarg != 0 )
    //    nn_thread_main_routine(Nanomsg_threadarg);
    //LP_pubkeys_query();
    //LP_utxosQ_process();
    //LP_nanomsg_recvs(ctx);
    LP_mainloop_iter(ctx,LP_myipaddr,0,LP_mypubsock);
    //queue_loop(0);
    if ( 0 ) // 10 seconds
    {
        LP_coinsloop(0);
        if ( 0 ) // 100 seconds
        {
            LP_notify_pubkeys(ctx,LP_mypubsock);
            LP_privkey_updates(ctx,LP_mypubsock,0);
            if ( (retstr= basilisk_swapentry(0,0,0,0)) != 0 )
                free(retstr);
        }
    }
}

#endif

#undef calloc
#undef free
#undef realloc
#undef clonestr

struct LP_memory_list
{
    struct LP_memory_list *next,*prev;
    uint32_t timestamp,len;
    void *ptr;
} *LP_memory_list;
int32_t zeroval() { return(0); }

void *LP_alloc(uint64_t len)
{
//return(calloc(1,len));
    LP_cjson_allocated += len;
    LP_cjson_total += len;
    LP_cjson_count++;
    struct LP_memory_list *mp;
    mp = calloc(1,sizeof(*mp) + len);
    mp->ptr = calloc(1,len);
    //printf(">>>>>>>>>>> LP_alloc mp.%p ptr.%p len.%llu %llu\n",mp,mp->ptr,(long long)len,(long long)LP_cjson_allocated);
    mp->timestamp = (uint32_t)time(NULL);
    mp->len = (uint32_t)len;
    portable_mutex_lock(&LP_cJSONmutex);
    DL_APPEND(LP_memory_list,mp);
    portable_mutex_unlock(&LP_cJSONmutex);
    return(mp->ptr);
}

void LP_free(void *ptr)
{
    static uint32_t lasttime,unknown; static int64_t lasttotal;
//free(ptr); return;
    uint32_t now; char str[65]; int32_t n,lagging; uint64_t total = 0; struct LP_memory_list *mp,*tmp;
    if ( (now= (uint32_t)time(NULL)) > lasttime+1 )
    {
        n = lagging = 0;
        DL_FOREACH_SAFE(LP_memory_list,mp,tmp)
        {
            total += mp->len;
            n++;
            if ( 0 && now > mp->timestamp+120 )
            {
                lagging++;
                if ( now > mp->timestamp+240 )
                {
                    portable_mutex_lock(&LP_cJSONmutex);
                    DL_DELETE(LP_memory_list,mp);
                    portable_mutex_unlock(&LP_cJSONmutex);
                    free(mp->ptr);
                    free(mp);
                }
            }
        }
        printf("[%lld] total %d allocated total %llu/%llu [%llu %llu] %.1f ave %s unknown.%u lagging.%d\n",(long long)(total-lasttotal),n,(long long)total,(long long)LP_cjson_allocated,(long long)LP_cjson_total,(long long)LP_cjson_count,(double)LP_cjson_total/LP_cjson_count,mbstr(str,total),unknown,lagging);
        lasttime = (uint32_t)time(NULL);
        lasttotal = total;
    }
    DL_FOREACH_SAFE(LP_memory_list,mp,tmp)
    {
        if ( mp->ptr == ptr )
            break;
        mp = 0;
    }
    if ( mp != 0 )
    {
        LP_cjson_allocated -= mp->len;
        portable_mutex_lock(&LP_cJSONmutex);
        DL_DELETE(LP_memory_list,mp);
        portable_mutex_unlock(&LP_cJSONmutex);
        //printf(">>>>>>>>>>> LP_free ptr.%p mp.%p len.%u %llu\n",ptr,mp,mp->len,(long long)LP_cjson_allocated);
        free(mp->ptr);
        free(mp);
    } else unknown++; // free from source file with #define redirect for alloc that wasnt
}

/*char *LP_clonestr(char *str)
{
    char *retstr = LP_alloc(strlen(str)+1);
    strcpy(retstr,str);
    return(retstr);
}

void *LP_realloc(void *ptr,uint64_t len)
{
    return(realloc(ptr,len));
}*/


