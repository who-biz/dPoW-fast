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

#include "../iguana/iguana777.h"

int32_t smallprimes[168] =
{
	2,      3,      5,      7,     11,     13,     17,     19,     23,     29,
	31,     37,     41,     43,     47,     53,     59,     61,     67,     71,
	73,     79,     83,     89,     97,    101,    103,    107,    109,    113,
	127,    131,    137,    139,    149,    151,    157,    163,    167,    173,
	179,    181,    191,    193,    197,    199,    211,    223,    227,    229,
	233,    239,    241,    251,    257,    263,    269,    271,    277,    281,
	283,    293,    307,    311,    313,    317,    331,    337,    347,    349,
	353,    359,    367,    373,    379,    383,    389,    397,    401,    409,
	419,    421,    431,    433,    439,    443,    449,    457,    461,    463,
	467,    479,    487,    491,    499,    503,    509,    521,    523,    541,
	547,    557,    563,    569,    571,    577,    587,    593,    599,    601,
	607,    613,    617,    619,    631,    641,    643,    647,    653,    659,
	661,    673,    677,    683,    691,    701,    709,    719,    727,    733,
	739,    743,    751,    757,    761,    769,    773,    787,    797,    809,
	811,    821,    823,    827,    829,    839,    853,    857,    859,    863,
	877,    881,    883,    887,    907,    911,    919,    929,    937,    941,
	947,    953,    967,    971,    977,    983,    991,    997
};

bits256 bits256_doublesha256(char *hashstr,uint8_t *data,int32_t datalen)
{
    bits256 hash,hash2; int32_t i;
    vcalc_sha256(0,hash.bytes,data,datalen);
    vcalc_sha256(0,hash2.bytes,hash.bytes,sizeof(hash));
    for (i=0; i<sizeof(hash); i++)
        hash.bytes[i] = hash2.bytes[sizeof(hash) - 1 - i];
    if ( hashstr != 0 )
        init_hexbytes_noT(hashstr,hash.bytes,sizeof(hash));
    return(hash);
}

char *bits256_str(char hexstr[65],bits256 x)
{
    init_hexbytes_noT(hexstr,x.bytes,sizeof(x));
    return(hexstr);
}

bits256 bits256_conv(char *hexstr)
{
    bits256 x;
    memset(&x,0,sizeof(x));
    if ( strlen(hexstr) == sizeof(x)*2)
        decode_hex(x.bytes,sizeof(x.bytes),hexstr);
    return(x);
}

char *bits256_lstr(char hexstr[65],bits256 x)
{
    bits256 revx; int32_t i;
    for (i=0; i<32; i++)
        revx.bytes[i] = x.bytes[31-i];
    init_hexbytes_noT(hexstr,revx.bytes,sizeof(revx));
    return(hexstr);
}

bits256 bits256_add(bits256 a,bits256 b)
{
    int32_t i; bits256 sum; uint64_t x,carry = 0;
    memset(sum.bytes,0,sizeof(sum));
    for (i=0; i<4; i++)
    {
        x = a.ulongs[i] + b.ulongs[i];
        sum.ulongs[i] = (x + carry);
        if ( x < a.ulongs[i] || x < b.ulongs[i] )
            carry = 1;
        else carry = 0;
    }
    return(sum);
}

int32_t bits256_cmp(bits256 a,bits256 b)
{
    int32_t i;
    for (i=3; i>=0; i--)
    {
        //printf("%llx %llx, ",(long long)a.ulongs[i],(long long)b.ulongs[i]);
        if ( a.ulongs[i] > b.ulongs[i] )
            return(1);
        else if ( a.ulongs[i] < b.ulongs[i] )
            return(-1);
    }
    //printf("thesame\n");
    return(0);
}

bits256 bits256_rshift(bits256 x)
{
    int32_t i; uint64_t carry,prevcarry = 0;
    for (i=3; i>=0; i--)
    {
        carry = (1 & x.ulongs[i]) << 63;
        x.ulongs[i] = prevcarry | (x.ulongs[i] >> 1);
        prevcarry = carry;
    }
    return(x);
}

bits256 bits256_lshift(bits256 x)
{
    int32_t i,carry,prevcarry = 0; uint64_t mask = (1LL << 63);
    for (i=0; i<4; i++)
    {
        carry = ((mask & x.ulongs[i]) != 0);
        x.ulongs[i] = (x.ulongs[i] << 1) | prevcarry;
        prevcarry = carry;
    }
    return(x);
}

bits256 bits256_ave(bits256 a,bits256 b)
{
    return(bits256_rshift(bits256_add(a,b)));
}

bits256 bits256_from_compact(uint32_t c)
{
    
	uint32_t nbytes,nbits,i; bits256 x;
    memset(x.bytes,0,sizeof(x));
    nbytes = (c >> 24) & 0xFF;
    if ( nbytes >= 3 )
    {
        nbits = (8 * (nbytes - 3));
        x.ulongs[0] = c & 0xFFFFFF;
        for (i=0; i<nbits; i++)
            x = bits256_lshift(x);
    }
    return(x);
}

uint32_t bits256_to_compact(bits256 x)
{
    int32_t i; uint32_t nbits;
    for (i=31; i>2; i--)
        if ( x.bytes[i] != 0 )
            break;
    if ( (x.bytes[i] & 0x80) != 0 )
        i++;
    nbits = x.bytes[i] << 16;
    nbits |= x.bytes[i-1] << 8;
    nbits |= x.bytes[i-2];
    nbits |= ((i+1) << 24);
    return(nbits);
}

int32_t bitweight(uint64_t x)
{
    int i,wt = 0;
    for (i=0; i<64; i++)
        if ( (1LL << i) & x )
            wt++;
    return(wt);
}

void calc_OP_HASH160(char hexstr[41],uint8_t rmd160[20],char *pubkey)
{
    uint8_t buf[4096]; int32_t len;
    len = (int32_t)strlen(pubkey)/2;
    if ( len > sizeof(buf) )
    {
        printf("calc_OP_HASH160 overflow len.%d vs %d\n",len,(int32_t)sizeof(buf));
        return;
    }
    decode_hex(buf,len,pubkey);
    calc_rmd160_sha256(rmd160,buf,len);
    if ( (0) )
    {
        int i;
        for (i=0; i<20; i++)
            printf("%02x",rmd160[i]);
        printf("<- (%s)\n",pubkey);
    }
    if ( hexstr != 0 )
        init_hexbytes_noT(hexstr,rmd160,20);
}

double _xblend(float *destp,double val,double decay)
{
    double oldval;
	if ( (oldval = *destp) != 0. )
		return((oldval * decay) + ((1. - decay) * val));
	else return(val);
}

double _dxblend(double *destp,double val,double decay)
{
    double oldval;
	if ( (oldval = *destp) != 0. )
		return((oldval * decay) + ((1. - decay) * val));
	else return(val);
}

double dxblend(double *destp,double val,double decay)
{
	double newval,slope;
	if ( isnan(*destp) != 0 )
		*destp = 0.;
	if ( isnan(val) != 0 )
		return(0.);
	if ( *destp == 0 )
	{
		*destp = val;
		return(0);
	}
	newval = _dxblend(destp,val,decay);
	if ( newval < SMALLVAL && newval > -SMALLVAL )
	{
		// non-zero marker for actual values close to or even equal to zero
		if ( newval < 0. )
			newval = -SMALLVAL;
		else newval = SMALLVAL;
	}
	if ( *destp != 0. && newval != 0. )
		slope = (newval - *destp);
	else slope = 0.;
	*destp = newval;
	return(slope);
}

int32_t TerminateQ_queued; queue_t TerminateQ;
/*void iguana_terminator(void *arg)
{
    struct iguana_thread *t; uint32_t lastdisp = 0; int32_t terminated = 0;
    printf("iguana_terminator\n");
    while ( 1 )
    {
        if ( (t= queue_dequeue(&TerminateQ,0)) != 0 )
        {
            printf("terminate.%p\n",t);
            iguana_terminate(t);
            terminated++;
            continue;
        }
        sleep(1);
        if ( time(NULL) > lastdisp+60 )
        {
            lastdisp = (uint32_t)time(NULL);
            printf("TerminateQ %d terminated of %d queued\n",terminated,TerminateQ_queued);
        }
    }
}*/


int32_t iguana_numthreads(struct iguana_info *coin,int32_t mask)
{
    int32_t i,sum = 0;
    for (i=0; i<8; i++)
        if ( ((1 << i) & mask) != 0 )
            sum += (coin->Launched[i] - coin->Terminated[i]);
    return(sum);
}

void iguana_launcher(void *ptr)
{
    struct iguana_thread *t = ptr; //struct iguana_info *coin;
    //coin = t->coin;
    t->funcp(t->arg);
    //if ( coin != 0 )
    //    coin->Terminated[t->type % (sizeof(coin->Terminated)/sizeof(*coin->Terminated))]++;
    queue_enqueue("TerminateQ",&TerminateQ,&t->DL);
}

void iguana_terminate(struct iguana_thread *t)
{
    int32_t retval;
#ifndef _WIN32
    retval = pthread_join(t->handle,NULL);
    if ( retval != 0 )
        printf("error.%d terminating t.%p thread.%s\n",retval,t,t->name);
#endif
    myfree(t,sizeof(*t));
}

struct iguana_thread *iguana_launch(struct iguana_info *coin,char *name,iguana_func funcp,void *arg,uint8_t type)
{
    int32_t retval; struct iguana_thread *t;
    t = mycalloc('Z',1,sizeof(*t));
    strcpy(t->name,name);
    t->coin = coin;
    t->funcp = funcp;
    t->arg = arg;
    t->type = (type % (sizeof(coin->Terminated)/sizeof(*coin->Terminated)));
    if ( coin != 0 )
        coin->Launched[t->type]++;
    retval = OS_thread_create(&t->handle,NULL,(void *)iguana_launcher,(void *)t);
    if ( retval != 0 )
        printf("error launching %s retval.%d errno.%d\n",t->name,retval,errno);
    while ( (t= queue_dequeue(&TerminateQ)) != 0 )
    {
        if ( (rand() % 100000) == 0 && coin != 0 )
            printf("terminated.%d launched.%d terminate.%p\n",coin->Terminated[t->type],coin->Launched[t->type],t);
        iguana_terminate(t);
    }
    return(t);
}

char hexbyte(int32_t c)
{
    c &= 0xf;
    if ( c < 10 )
        return('0'+c);
    else if ( c < 16 )
        return('a'+c-10);
    else return(0);
}

int32_t _unhex(char c)
{
    if ( c >= '0' && c <= '9' )
        return(c - '0');
    else if ( c >= 'a' && c <= 'f' )
        return(c - 'a' + 10);
    else if ( c >= 'A' && c <= 'F' )
        return(c - 'A' + 10);
    return(-1);
}

int32_t is_hexstr(char *str,int32_t n)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
    {
        if ( n > 0 && i >= n )
            break;
        if ( _unhex(str[i]) < 0 )
            break;
    }
    if ( n == 0 )
        return(i);
    return(i == n);
}

int32_t unhex(char c)
{
    int32_t hex;
    if ( (hex= _unhex(c)) < 0 )
    {
        //printf("unhex: illegal hexchar.(%c)\n",c);
    }
    return(hex);
}

unsigned char _decode_hex(char *hex) { return((unhex(hex[0])<<4) | unhex(hex[1])); }

int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex)
{
    int32_t adjust,i = 0;
    //printf("decode.(%s)\n",hex);
    if ( is_hexstr(hex,n) <= 0 )
    {
        memset(bytes,0,n);
        return(n);
    }
    if ( hex[n-1] == '\n' || hex[n-1] == '\r' )
        hex[--n] = 0;
    if ( hex[n-1] == '\n' || hex[n-1] == '\r' )
        hex[--n] = 0;
    if ( n == 0 || (hex[n*2+1] == 0 && hex[n*2] != 0) )
    {
        if ( n > 0 )
        {
            bytes[0] = unhex(hex[0]);
            printf("decode_hex n.%d hex[0] (%c) -> %d hex.(%s) [n*2+1: %d] [n*2: %d %c] len.%ld\n",n,hex[0],bytes[0],hex,hex[n*2+1],hex[n*2],hex[n*2],(long)strlen(hex));
        }
        bytes++;
        hex++;
        adjust = 1;
    } else adjust = 0;
    if ( n > 0 )
    {
        for (i=0; i<n; i++)
            bytes[i] = _decode_hex(&hex[i*2]);
    }
    //bytes[i] = 0;
    return(n + adjust);
}

int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len)
{
    int32_t i;
    if ( len <= 0 )
    {
        hexbytes[0] = 0;
        return(1);
    }
    for (i=0; i<len; i++)
    {
        hexbytes[i*2] = hexbyte((message[i]>>4) & 0xf);
        hexbytes[i*2 + 1] = hexbyte(message[i] & 0xf);
        //printf("i.%d (%02x) [%c%c]\n",i,message[i],hexbytes[i*2],hexbytes[i*2+1]);
    }
    hexbytes[len*2] = 0;
    //printf("len.%ld\n",len*2+1);
    return((int32_t)len*2+1);
}

long _stripwhite(char *buf,int accept)
{
    int32_t i,j,c;
    if ( buf == 0 || buf[0] == 0 )
        return(0);
    for (i=j=0; buf[i]!=0; i++)
    {
        buf[j] = c = buf[i];
        if ( c == accept || (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\b') )
            j++;
    }
    buf[j] = 0;
    return(j);
}

char *clonestr(char *str)
{
    char *clone;
    if ( str == 0 || str[0] == 0 )
    {
        printf("warning cloning nullstr.%p\n",str);
//#ifdef __APPLE__
//        while ( 1 ) sleep(1);
//#endif
        str = (char *)"<nullstr>";
    }
    clone = (char *)malloc(strlen(str)+16);
    strcpy(clone,str);
    return(clone);
}

int32_t safecopy(char *dest,char *src,long len)
{
    int32_t i = -1;
    if ( src != 0 && dest != 0 && src != dest )
    {
        if ( dest != 0 )
            memset(dest,0,len);
        for (i=0; i<len&&src[i]!=0; i++)
            dest[i] = src[i];
        if ( i == len )
        {
            printf("safecopy: %s too long %ld\n",src,len);
            //printf("divide by zero! %d\n",1/zeroval());
#ifdef __APPLE__
            //getchar();
#endif
            return(-1);
        }
        dest[i] = 0;
    }
    return(i);
}

void escape_code(char *escaped,char *str)
{
    int32_t i,j,c; char esc[16];
    for (i=j=0; str[i]!=0; i++)
    {
        if ( ((c= str[i]) >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )
            escaped[j++] = c;
        else
        {
            sprintf(esc,"%%%02X",c);
            //sprintf(esc,"\\\\%c",c);
            strcpy(escaped + j,esc);
            j += strlen(esc);
        }
    }
    escaped[j] = 0;
    //printf("escape_code: (%s) -> (%s)\n",str,escaped);
}

int32_t is_zeroes(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(1);
    for (i=0; str[i]!=0; i++)
        if ( str[i] != '0' )
            return(0);
    return(1);
}

int64_t conv_floatstr(char *numstr)
{
    double val,corr;
    val = atof(numstr);
    corr = (val < 0.) ? -0.50000000001 : 0.50000000001;
    return((int64_t)(val * SATOSHIDEN + corr));
}

int32_t has_backslash(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
        if ( str[i] == '\\' )
            return(1);
    return(0);
}

static int _increasing_double(const void *a,const void *b)
{
#define double_a (*(double *)a)
#define double_b (*(double *)b)
    if ( double_b > double_a )
        return(-1);
    else if ( double_b < double_a )
        return(1);
    return(0);
#undef double_a
#undef double_b
}

static int _decreasing_double(const void *a,const void *b)
{
#define double_a (*(double *)a)
#define double_b (*(double *)b)
    if ( double_b > double_a )
        return(1);
    else if ( double_b < double_a )
        return(-1);
    return(0);
#undef double_a
#undef double_b
}

int _increasing_uint64(const void *a,const void *b)
{
#define uint64_a (*(uint64_t *)a)
#define uint64_b (*(uint64_t *)b)
	if ( uint64_b > uint64_a )
		return(-1);
	else if ( uint64_b < uint64_a )
		return(1);
	return(0);
#undef uint64_a
#undef uint64_b
}

int _decreasing_uint64(const void *a,const void *b)
{
#define uint64_a (*(uint64_t *)a)
#define uint64_b (*(uint64_t *)b)
	if ( uint64_b > uint64_a )
		return(1);
	else if ( uint64_b < uint64_a )
		return(-1);
	return(0);
#undef uint64_a
#undef uint64_b
}

int _decreasing_uint32(const void *a,const void *b)
{
#define uint32_a (*(uint32_t *)a)
#define uint32_b (*(uint32_t *)b)
	if ( uint32_b > uint32_a )
		return(1);
	else if ( uint32_b < uint32_a )
		return(-1);
	return(0);
#undef uint32_a
#undef uint32_b
}

int32_t sortds(double *buf,uint32_t num,int32_t size)
{
    qsort(buf,num,size,_increasing_double);
    return(0);
}

int32_t revsortds(double *buf,uint32_t num,int32_t size)
{
    qsort(buf,num,size,_decreasing_double);
    return(0);
}

int32_t sort64s(uint64_t *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_increasing_uint64);
	return(0);
}

int32_t revsort64s(uint64_t *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_decreasing_uint64);
	return(0);
}

int32_t revsort32(uint32_t *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_decreasing_uint32);
	return(0);
}

/*int32_t iguana_sortbignum(void *buf,int32_t size,uint32_t num,int32_t structsize,int32_t dir)
{
    int32_t retval = 0;
    if ( dir > 0 )
    {
        if ( size == 32 )
            qsort(buf,num,structsize,_increasing_bits256);
        else if ( size == 20 )
            qsort(buf,num,structsize,_increasing_rmd160);
        else retval = -1;
    }
    else
    {
        if ( size == 32 )
            qsort(buf,num,structsize,_decreasing_bits256);
        else if ( size == 20 )
            qsort(buf,num,structsize,_decreasing_rmd160);
        else retval = -1;
    }
    if ( retval < 0 )
        printf("iguana_sortbignum only does bits256 and rmd160 for now\n");
	return(retval);
}*/

void touppercase(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return;
    for (i=0; str[i]!=0; i++)
        str[i] = toupper(((int32_t)str[i]));
}

void tolowercase(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return;
    for (i=0; str[i]!=0; i++)
        str[i] = tolower(((int32_t)str[i]));
}

char *uppercase_str(char *buf,char *str)
{
    if ( str != 0 )
    {
        strcpy(buf,str);
        touppercase(buf);
    } else buf[0] = 0;
    return(buf);
}

char *lowercase_str(char *buf,char *str)
{
    if ( str != 0 )
    {
        strcpy(buf,str);
        tolowercase(buf);
    } else buf[0] = 0;
    return(buf);
}

int32_t strsearch(char *strs[],int32_t num,char *name)
{
    int32_t i; char strA[32],refstr[32];
    strcpy(refstr,name), touppercase(refstr);
    for (i=0; i<num; i++)
    {
        strcpy(strA,strs[i]), touppercase(strA);
        if ( strcmp(strA,refstr) == 0 )
            return(i);
    }
    return(-1);
}

int32_t is_decimalstr(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
        if ( str[i] < '0' || str[i] > '9' )
            return(0);
    return(i);
}

int32_t unstringbits(char *buf,uint64_t bits)
{
    int32_t i;
    for (i=0; i<8; i++,bits>>=8)
        if ( (buf[i]= (char)(bits & 0xff)) == 0 )
            break;
    buf[i] = 0;
    return(i);
}

uint64_t stringbits(char *str)
{
    uint64_t bits = 0;
    if ( str == 0 )
        return(0);
    int32_t i,n = (int32_t)strlen(str);
    if ( n > 8 )
        n = 8;
    for (i=n-1; i>=0; i--)
        bits = (bits << 8) | (str[i] & 0xff);
    //printf("(%s) -> %llx %llu\n",str,(long long)bits,(long long)bits);
    return(bits);
}

char *unstringify(char *str)
{
    int32_t i,j,n;
    if ( str == 0 )
        return(0);
    else if ( str[0] == 0 )
        return(str);
    n = (int32_t)strlen(str);
    if ( str[0] == '"' && str[n-1] == '"' )
        str[n-1] = 0, i = 1;
    else i = 0;
    for (j=0; str[i]!=0; i++)
    {
        if ( str[i] == '\\' && (str[i+1] == 't' || str[i+1] == 'n' || str[i+1] == 'b' || str[i+1] == 'r') )
            i++;
        else if ( str[i] == '\\' && str[i+1] == '"' )
            str[j++] = '"', i++;
        else str[j++] = str[i];
    }
    str[j] = 0;
    return(str);
}

void reverse_hexstr(char *str)
{
    int i,n;
    char *rev;
    n = (int32_t)strlen(str);
    rev = (char *)malloc(n + 1);
    for (i=0; i<n; i+=2)
    {
        rev[n-2-i] = str[i];
        rev[n-1-i] = str[i+1];
    }
    rev[n] = 0;
    strcpy(str,rev);
    free(rev);
}

int32_t nn_base64_decode (const char *in, size_t in_len,uint8_t *out, size_t out_len)
{
    uint32_t ii,io,rem,v; uint8_t ch;
    //  Unrolled lookup of ASCII code points. 0xFF represents a non-base64 valid character.
    const uint8_t DECODEMAP [256] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
        0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF,
        0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
        0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    for (io = 0, ii = 0, v = 0, rem = 0; ii < in_len; ii++) {
        if (isspace ((uint32_t)in [ii]))
            continue;

        if (in [ii] == '=')
            break;

        ch = DECODEMAP [(uint32_t)in [ii]];

        // Discard invalid characters as per RFC 2045.
        if (ch == 0xFF)
            break;

        v = (v << 6) | ch;
        rem += 6;

        if (rem >= 8) {
            rem -= 8;
            if (io >= out_len)
                return -ENOBUFS;
            out [io++] = (v >> rem) & 255;
        }
    }
    if (rem >= 8) {
        rem -= 8;
        if (io >= out_len)
            return -ENOBUFS;
        out [io++] = (v >> rem) & 255;
    }
    return io;
}

int32_t nn_base64_encode (const uint8_t *in, size_t in_len,char *out, size_t out_len)
{
    uint32_t ii,io,rem,v; uint8_t ch;
    const uint8_t ENCODEMAP [64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

    for (io = 0, ii = 0, v = 0, rem = 0; ii < in_len; ii++) {
        ch = in [ii];
        v = (v << 8) | ch;
        rem += 8;
        while (rem >= 6) {
            rem -= 6;
            if (io >= out_len)
                return -ENOBUFS;
            out [io++] = ENCODEMAP [(v >> rem) & 63];
        }
    }

    if (rem) {
        v <<= (6 - rem);
        if (io >= out_len)
            return -ENOBUFS;
        out [io++] = ENCODEMAP [v & 63];
    }

    //  Pad to a multiple of 3
    while (io & 3) {
        if (io >= out_len)
            return -ENOBUFS;
        out [io++] = '=';
    }

    if (io >= out_len)
        return -ENOBUFS;

    out [io] = '\0';

    return io;
}

/*
 NXT address converter,
 Ported from original javascript (nxtchg)
 To C by Jones
 */

int32_t gexp[] = {1, 2, 4, 8, 16, 5, 10, 20, 13, 26, 17, 7, 14, 28, 29, 31, 27, 19, 3, 6, 12, 24, 21, 15, 30, 25, 23, 11, 22, 9, 18, 1};
int32_t glog[] = {0, 0, 1, 18, 2, 5, 19, 11, 3, 29, 6, 27, 20, 8, 12, 23, 4, 10, 30, 17, 7, 22, 28, 26, 21, 25, 9, 16, 13, 14, 24, 15};
int32_t cwmap[] = {3, 2, 1, 0, 7, 6, 5, 4, 13, 14, 15, 16, 12, 8, 9, 10, 11};
char alphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

int32_t gmult(int32_t a,int32_t b)
{
    if ( a == 0 || b == 0 )
        return 0;
    int32_t idx = (glog[a] + glog[b]) % 31;
    return gexp[idx];
}

int32_t letterval(char letter)
{
    int32_t ret = 0;
    if ( letter < '9' )
        ret = letter - '2';
    else
    {
        ret = letter - 'A' + 8;
        if ( letter > 'I' )
            ret--;
        if ( letter > 'O' )
            ret--;
    }
    return ret;
}

uint64_t RS_decode(char *rs)
{
    int32_t code[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int32_t i,p = 4;
    if ( strncmp("NXT-",rs,4) != 0 )
        return(0);
    for (i=0; i<17; i++)
    {
        code[cwmap[i]] = letterval(rs[p]);
        p++;
        if ( rs[p] == '-' )
            p++;
    }
    uint64_t out = 0;
    for (i=12; i>=0; i--)
        out = out * 32 + code[i];
    return out;
}

int32_t RS_encode(char *rsaddr,uint64_t id)
{
    int32_t a,code[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int32_t inp[32],out[32],i,j,fb,pos = 0,len = 0;
    char acc[64];
    rsaddr[0] = 0;
    memset(inp,0,sizeof(inp));
    memset(out,0,sizeof(out));
    memset(acc,0,sizeof(acc));
    expand_nxt64bits(acc,id);
    //sprintf(acc,"%llu",(long long)id);
    for (a=0; *(acc+a) != '\0'; a++)
        len++;
    if ( len == 20 && *acc != '1' )
    {
        printf("error (%s) doesnt start with 1",acc);
        return(-1);
    }
    for (i=0; i<len; i++)
        inp[i] = (int32_t)*(acc+i) - (int32_t)'0';
    int32_t divide = 0;
    int32_t newlen = 0;
    do // base 10 to base 32 conversion
    {
        divide = 0;
        newlen = 0;
        for (i=0; i<len; i++)
        {
            divide = divide * 10 + inp[i];
            if (divide >= 32)
            {
                inp[newlen++] = divide >> 5;
                divide &= 31;
            }
            else if ( newlen > 0 )
                inp[newlen++] = 0;
        }
        len = newlen;
        out[pos++] = divide;
    } while ( newlen != 0 );
    for (i=0; i<13; i++) // copy to code in reverse, pad with 0's
        code[i] = (--pos >= 0 ? out[i] : 0);
    int32_t p[] = {0, 0, 0, 0};
    for (i=12; i>=0; i--)
    {
        fb = code[i] ^ p[3];
        p[3] = p[2] ^ gmult(30, fb);
        p[2] = p[1] ^ gmult(6, fb);
        p[1] = p[0] ^ gmult(9, fb);
        p[0] = gmult(17, fb);
    }
    code[13] = p[0];
    code[14] = p[1];
    code[15] = p[2];
    code[16] = p[3];
    strcpy(rsaddr,"NXT-");
    j=4;
    for (i=0; i<17; i++)
    {
        rsaddr[j++] = alphabet[code[cwmap[i]]];
        if ( (j % 5) == 3 && j < 20 )
            rsaddr[j++] = '-';
    }
    rsaddr[j] = 0;
    //printf("%llu -> NXT RS (%s)\n",(long long)id,rsaddr);
    return(0);
}

uint64_t conv_acctstr(char *acctstr)
{
    uint64_t nxt64bits = 0;
    int32_t len;
    if ( acctstr != 0 )
    {
        if ( (len= is_decimalstr(acctstr)) > 0 && len < 24 )
            nxt64bits = calc_nxt64bits(acctstr);
        else if ( strncmp("NXT-",acctstr,4) == 0 )
        {
            nxt64bits = RS_decode(acctstr);
            //nxt64bits = conv_rsacctstr(acctstr,0);
        }
    }
    return(nxt64bits);
}

int32_t base32byte(int32_t val)
{
    if ( val < 26 )
        return('A' + val);
    else if ( val < 32 )
        return('2' + val - 26);
    else return(-1);
}

int32_t unbase32(char c)
{
    if ( c >= 'A' && c <= 'Z' )
        return(c - 'A');
    else if ( c >= '2' && c <= '7' )
        return(c - '2' + 26);
    else return(-1);
}

int init_base32(char *tokenstr,uint8_t *token,int32_t len)
{
    int32_t i,j,n,val5,offset = 0;
    for (i=n=0; i<len; i++)
    {
        for (j=val5=0; j<5; j++,offset++)
            if ( GETBIT(token,offset) != 0 )
                SETBIT(&val5,offset);
        tokenstr[n++] = base32byte(val5);
    }
    tokenstr[n] = 0;
    return(n);
}

int decode_base32(uint8_t *token,uint8_t *tokenstr,int32_t len)
{
    int32_t i,j,n,val5,offset = 0;
    for (i=n=0; i<len; i++)
    {
        if ( (val5= unbase32(tokenstr[i])) >= 0 )
        {
            for (j=val5=0; j<5; j++,offset++)
            {
                if ( GETBIT(&val5,j) != 0 )
                    SETBIT(token,offset);
                else CLEARBIT(token,offset);
            }
        } else return(-1);
    }
    while ( (offset & 7) != 0 )
    {
        CLEARBIT(token,offset);
        offset++;
    }
    return(offset);
}

void calc_hexstr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    init_hexbytes_noT(hexstr,(void *)msg,len+1);
}

void calc_unhexstr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    decode_hex((void *)hexstr,len>>1,(void *)msg);
    hexstr[len>>1] = 0;
}

void calc_base64_encodestr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    nn_base64_encode(msg,len,hexstr,len);
}

void calc_base64_decodestr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    nn_base64_decode((void *)msg,len,(void *)hexstr,1024);
}

void sha256_sha256(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    bits256_doublesha256(hexstr,msg,len);
}

void rmd160ofsha256(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    uint8_t sha256[32];
    if ( is_hexstr((char *)msg,len) > 0 )
    {
        decode_hex((uint8_t *)hexstr,len/2,(char *)msg);
        vcalc_sha256(0,sha256,(void *)hexstr,len/2);
        calc_rmd160(hexstr,buf,sha256,sizeof(sha256));
    } else vcalc_sha256(0,sha256,(void *)msg,len);
    calc_rmd160(hexstr,buf,sha256,sizeof(sha256));
}

void calc_crc32str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    uint32_t crc; uint8_t serialized[sizeof(crc)];
    crc = calc_crc32(0,msg,len);
    //iguana_rwnum(1,serialized,sizeof(crc),&crc);
    serialized[3] = (crc & 0xff), crc >>= 8;
    serialized[2] = (crc & 0xff), crc >>= 8;
    serialized[1] = (crc & 0xff), crc >>= 8;
    serialized[0] = (crc & 0xff), crc >>= 8;
    init_hexbytes_noT(hexstr,serialized,sizeof(crc));
    //printf("crc.%08x vs revcrc.%08x -> %s\n",crc,*(uint32_t *)serialized,hexstr);
}

void calc_NXTaddr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    uint8_t mysecret[32]; uint64_t nxt64bits;
    nxt64bits = conv_NXTpassword(mysecret,buf,msg,len);
    //printf("call RSencode with %llu\n",(long long)nxt64bits);
    RS_encode(hexstr,nxt64bits);
}

void calc_curve25519_str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len)
{
    bits256 x,priv,pub;
    if ( len != sizeof(bits256)*2 || is_hexstr((char *)msg,64) == 0 )
        conv_NXTpassword(priv.bytes,pub.bytes,msg,len);
    else priv = *(bits256 *)msg;
    x = curve25519(priv,curve25519_basepoint9());
    init_hexbytes_noT(hexstr,x.bytes,sizeof(x));
}

void calc_rmd160_sha256(uint8_t rmd160[20],uint8_t *data,int32_t datalen)
{
    bits256 hash;
    vcalc_sha256(0,hash.bytes,data,datalen);
    calc_rmd160(0,rmd160,hash.bytes,sizeof(hash));
}

char *cmc_ticker(char *base)
{
    char url[512];
    sprintf(url,"https://api.coinmarketcap.com/v1/ticker/%s/",base);
    return(issue_curl(url));
}

char *bittrex_orderbook(char *base,char *rel,int32_t maxdepth)
{
    char market[64],url[512];
    sprintf(market,"%s-%s",rel,base);
    sprintf(url,"http://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    return(issue_curl(url));
}

double calc_theoretical(double weighted,double CMC_average,double changes[3])
{
    double theoretical = 0.; //adjusted = 0.,
    if ( weighted > SMALLVAL && CMC_average > SMALLVAL )
    {
        theoretical = (weighted + CMC_average) * 0.5;
        /*if ( changes[0] > SMALLVAL && changes[1] > SMALLVAL && changes[2] > SMALLVAL )
        {
            if ( changes[0] > changes[1] && changes[1] > changes[2] ) // breakout
            {
                adjusted = theoretical * (1. - (changes[0] + changes[1]) * .005);
            }
        }
        else if ( changes[0] < -SMALLVAL && changes[1] < -SMALLVAL && changes[2] < -SMALLVAL ) //
        {
            if ( changes[0] < changes[1] && changes[1] < changes[2] ) // waterfall
            {
                adjusted = theoretical * (1. - (changes[0] + changes[1]) * .005);
            }
        }
        if ( adjusted != 0. && theoretical != 0. )
            theoretical = (theoretical + adjusted) * 0.5;*/
    }
    //printf("adjusted %.8f theoretical %.8f (%.8f + wt %.8f)\n",adjusted,theoretical,CMC_average,weighted);
    return(theoretical);
}

double calc_weighted(double *avebidp,double *aveaskp,double *bids,double *bidvols,int32_t numbids,double *asks,double *askvols,int32_t numasks,double limit)
{
    int32_t i; double weighted = 0.,bidsum = 0., asksum = 0.,totalbids = 0.,totalasks = 0.;
    bidsum = bids[0] * bidvols[0], totalbids = bidvols[0];
    asksum = asks[0] * askvols[0], totalasks = askvols[0];
    for (i=1; i<numbids; i++)
    {
        if ( totalbids > limit )
            break;
        bidsum += bids[i] * bidvols[i];
        totalbids += bidvols[i];
    }
    for (i=1; i<numasks; i++)
    {
        if ( totalasks > limit )
            break;
        asksum += asks[i] * askvols[i];
        totalasks += askvols[i];
    }
    if ( totalbids != 0. && totalasks != 0. )
    {
        *avebidp = (bidsum / totalbids);
        *aveaskp = (asksum / totalasks);
        weighted = (*avebidp + *aveaskp) * 0.5;
    }
    //printf("weighted %f\n",weighted);
    return(weighted);
}

double weighted_orderbook(double *avebidp,double *aveaskp,double *highbidp,double *lowaskp,char *orderbookstr,double limit)
{
    cJSON *bookjson,*bid,*ask,*resobj,*item; int32_t i,numbids,numasks; double bidvols[50],bids[50],askvols[50],asks[50],weighted = 0.;
    if ( orderbookstr != 0 )
    {
        if ( (bookjson= cJSON_Parse(orderbookstr)) != 0 )
        {
            if ( (resobj= jobj(bookjson,"result")) != 0 )
            {
                bid = jarray(&numbids,resobj,"buy");
                if ( numbids > sizeof(bids)/sizeof(*bids) )
                    numbids = (int32_t)(sizeof(bids)/sizeof(*bids));
                ask = jarray(&numasks,resobj,"sell");
                if ( numasks > sizeof(asks)/sizeof(*asks) )
                    numasks = (int32_t)(sizeof(asks)/sizeof(*asks));
                if ( bid != 0 && ask != 0 )
                {
                    for (i=0; i<numbids; i++)
                    {
                        item = jitem(bid,i);
                        bidvols[i] = jdouble(item,"Quantity");
                        bids[i] = jdouble(item,"Rate");
                    }
                    for (i=0; i<numasks; i++)
                    {
                        item = jitem(ask,i);
                        askvols[i] = jdouble(item,"Quantity");
                        asks[i] = jdouble(item,"Rate");
                    }
                    *highbidp = bids[0];
                    *lowaskp = asks[0];
                    weighted = calc_weighted(avebidp,aveaskp,bids,bidvols,numbids,asks,askvols,numasks,limit);
                    //printf("weighted %.8f (%.8f %.8f)\n",weighted,*highbidp,*lowaskp);
                }
            }
            free_json(bookjson);
        }
    }
    return(weighted);
}

double get_theoretical(double *avebidp,double *aveaskp,double *highbidp,double *lowaskp,double *CMC_averagep,double changes[3],char *name,char *base,char *rel,double *USD_averagep)
{
    static int32_t counter;
    char *cmcstr; cJSON *cmcjson,*item; double weighted,theoretical = 0.;
    *avebidp = *aveaskp = *highbidp = *lowaskp = *CMC_averagep = 0.;
    if ( (cmcstr= cmc_ticker(name)) != 0 )
    {
        if ( (cmcjson= cJSON_Parse(cmcstr)) != 0 )
        {
            if ( is_cJSON_Array(cmcjson) == 0 )
                item = cmcjson;
            else item = jitem(cmcjson,0);
            *CMC_averagep = jdouble(item,"price_btc");
            *USD_averagep = jdouble(item,"price_usd");
            changes[0] = jdouble(item,"percent_change_1h");
            changes[1] = jdouble(item,"percent_change_24h");
            changes[2] = jdouble(item,"percent_change_7d");
            weighted = weighted_orderbook(avebidp,aveaskp,highbidp,lowaskp,bittrex_orderbook(base,rel,25),1./(*CMC_averagep));
            if ( *CMC_averagep > SMALLVAL && weighted > SMALLVAL )
                theoretical = calc_theoretical(weighted,*CMC_averagep,changes);
            if ( (0) && counter++ < 100 )
                printf("HBLA.[%.8f %.8f] AVE.[%.8f %.8f] (%s) CMC %f %f %f %f\n",*highbidp,*lowaskp,*avebidp,*aveaskp,jprint(item,0),*CMC_averagep,changes[0],changes[1],changes[2]);
            free_json(cmcjson);
        }
        free(cmcstr);
    }
    return(theoretical);
}

bits256 bits256_calctxid(char *symbol,uint8_t *serialized,int32_t len)
{
    bits256 txid,revtxid; int32_t i;
    memset(txid.bytes,0,sizeof(txid));
    if ( strcmp(symbol,"GRS") != 0 && strcmp(symbol,"SMART") != 0 )
        txid = bits256_doublesha256(0,serialized,len);
    else
    {
        vcalc_sha256(0,revtxid.bytes,serialized,len);
        for (i=0; i<32; i++)
            txid.bytes[i] = revtxid.bytes[31 - i];
    }
    return(txid);
}

bits256 iguana_merkle(char *symbol,bits256 *tree,int32_t txn_count)
{
    int32_t i,n=0,prev; uint8_t serialized[sizeof(bits256) * 2];
    if ( txn_count == 1 )
        return(tree[0]);
    prev = 0;
    while ( txn_count > 1 )
    {
        if ( (txn_count & 1) != 0 )
            tree[prev + txn_count] = tree[prev + txn_count-1], txn_count++;
        n += txn_count;
        for (i=0; i<txn_count; i+=2)
        {
            iguana_rwbignum(1,serialized,sizeof(*tree),tree[prev + i].bytes);
            iguana_rwbignum(1,&serialized[sizeof(*tree)],sizeof(*tree),tree[prev + i + 1].bytes);
            tree[n + (i >> 1)] = bits256_calctxid(symbol,serialized,sizeof(serialized));
        }
        prev = n;
        txn_count >>= 1;
    }
    return(tree[n]);
}
