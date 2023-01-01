
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
//  LP_portfolio.c
//  marketmaker
//

struct LP_portfoliotrade { double metric; char buycoin[65],sellcoin[65]; };

struct LP_autoprice_ref
{
    char refbase[65],refrel[65],base[65],rel[65],fundbid[16],fundask[16],usdpeg;
    double buymargin,sellmargin,factor,offset,lastbid,lastask;
    cJSON *fundvalue;
    uint32_t count;
} LP_autorefs[1024];

int32_t LP_autoprices,num_LP_autorefs;
char LP_portfolio_base[128],LP_portfolio_rel[128];
double LP_portfolio_relvolume;

void LP_portfolio_reset()
{
    struct iguana_info *coin,*tmp; cJSON *fundjson; int32_t i; struct LP_autoprice_ref *ptr;
    for (i=0; i<num_LP_autorefs; i++)
    {
        ptr = &LP_autorefs[i];
        if ( (fundjson= ptr->fundvalue) != 0 )
        {
            ptr->fundvalue = 0;
            free_json(fundjson);
        }
    }
    memset(LP_autorefs,0,sizeof(LP_autorefs));
    LP_autoprices = 0;
    num_LP_autorefs = 0;
    strcpy(LP_portfolio_base,"");
    strcpy(LP_portfolio_rel,"");
    LP_portfolio_relvolume = 0.;
    HASH_ITER(hh,LP_coins,coin,tmp)
    {
        coin->maxamount = 0;
        coin->perc = 0;
        coin->goal = 0;
        coin->goalperc = 0;
        coin->relvolume = 0;
        coin->force = 0;
        coin->balanceA = 0;
        coin->valuesumA = 0;
        coin->balanceB = 0;
        coin->valuesumB = 0;
    }
}

cJSON *LP_portfolio_entry(struct iguana_info *coin)
{
    cJSON *item = cJSON_CreateObject();
    jaddstr(item,"coin",coin->symbol);
    jaddstr(item,"address",coin->smartaddr);
    jaddnum(item,"amount",dstr(coin->maxamount));
    jaddnum(item,"price",coin->price_kmd);
    jaddnum(item,"kmd_equiv",dstr(coin->kmd_equiv));
    jaddnum(item,"perc",coin->perc);
    jaddnum(item,"goal",coin->goal);
    jaddnum(item,"goalperc",coin->goalperc);
    jaddnum(item,"relvolume",coin->relvolume);
    jaddnum(item,"force",coin->force);
    jaddnum(item,"balanceA",dstr(coin->balanceA));
    jaddnum(item,"valuesumA",dstr(coin->valuesumA));
    if ( coin->valuesumA != 0 )
        jaddnum(item,"aliceutil",100. * (double)coin->balanceA/coin->valuesumA);
    jaddnum(item,"balanceB",dstr(coin->balanceB));
    jaddnum(item,"valuesumB",dstr(coin->valuesumB));
    jaddnum(item,"balance",dstr(coin->maxamount));
    if ( coin->valuesumB != 0 )
        jaddnum(item,"bobutil",100. * (double)coin->balanceB/coin->valuesumB);
    return(item);
}

uint64_t LP_balance(uint64_t *valuep,int32_t iambob,char *symbol,char *coinaddr)
{
    cJSON *array,*item; bits256 zero; int32_t i,n; uint64_t valuesum,satoshisum,value;
    valuesum = satoshisum = 0;
    memset(zero.bytes,0,sizeof(zero));
#ifndef NOTETOMIC
    struct iguana_info *coin = LP_coinfind(symbol);
    if (coin->etomic[0] != 0 && coin->inactive == 0) {
        int error = 0;
        uint64_t etomic_balance = LP_etomic_get_balance(coin, coinaddr, &error);
        if (error == 0) {
            valuesum = etomic_balance;
        } else {
            valuesum = *valuep;
        }
    } else
#endif
    if ( (array= LP_listunspent(symbol,coinaddr,zero,zero)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                item = jitem(array,i);
                value = LP_value_extract(item,0,zero);
                valuesum += value;
            }
        }
        free_json(array);
    }
    /*if ( (array= LP_inventory(symbol)) != 0 )
    {
        if ( (n= cJSON_GetArraySize(array)) > 0 && is_cJSON_Array(array) != 0 )
        {
            for (i=0; i<n; i++)
            {
                item = jitem(array,i);
                //valuesum += j64bits(item,"value") + j64bits(item,"value2");
                satoshisum += j64bits(item,"satoshis");
            }
        }
        free_json(array);
    }*/
    *valuep = valuesum;
    satoshisum = valuesum;
    return(satoshisum);
}

char *LP_portfolio()
{
    double maxval = 0.,minval = 0.,goalsum = 0.; uint64_t kmdsum = 0; int32_t iter; cJSON *retjson,*array; struct iguana_info *coin,*tmp,*sellcoin = 0,*buycoin = 0;
    array = cJSON_CreateArray();
    retjson = cJSON_CreateObject();
    for (iter=0; iter<2; iter++)
    {
        HASH_ITER(hh,LP_coins,coin,tmp)
        {
            if ( coin->inactive != 0 )//|| (coin->electrum != 0 && coin->obooktime == 0) )
                continue;
            if ( iter == 0 )
            {
                //printf("from portfolio\n");
                //LP_privkey_init(-1,coin,G.LP_privkey,G.LP_mypub25519);
                coin->balanceA = LP_balance(&coin->valuesumA,0,coin->symbol,coin->smartaddr);
                coin->balanceB = LP_balance(&coin->valuesumB,1,coin->symbol,coin->smartaddr);
                if ( strcmp(coin->symbol,"KMD") != 0 )
                    coin->price_kmd = LP_price(1,coin->symbol,"KMD");
                else coin->price_kmd = 1.;
                coin->maxamount = coin->valuesumA;
                if ( coin->valuesumB > coin->maxamount )
                    coin->maxamount = coin->valuesumB;
                coin->kmd_equiv = coin->maxamount * coin->price_kmd;
                kmdsum += coin->kmd_equiv;
                goalsum += coin->goal;
            }
            else
            {
                coin->relvolume = 0.;
                if ( kmdsum > SMALLVAL )
                    coin->perc = 100. * coin->kmd_equiv / kmdsum;
                if ( goalsum > SMALLVAL && coin->goal > SMALLVAL )
                {
                    coin->goalperc = 100. * coin->goal / goalsum;
                    if ( (coin->force= (coin->goalperc - coin->perc)) < 0. )
                    {
                        coin->force *= -coin->force;
                        if ( coin->price_kmd > SMALLVAL )
                            coin->relvolume = (dstr(coin->maxamount) * (coin->perc - coin->goalperc)) / 100.;
                    } else coin->force *= coin->force;
                    if ( coin->force > maxval )
                    {
                        maxval = coin->force;
                        buycoin = coin;
                    }
                    if ( coin->force < minval )
                    {
                        minval = coin->force;
                        sellcoin = coin;
                    }
                } else coin->goalperc = coin->force = 0.;
                jaddi(array,LP_portfolio_entry(coin));
            }
        }
    }
    jaddstr(retjson,"result","success");
    jaddnum(retjson,"kmd_equiv",dstr(kmdsum));
    if ( buycoin != 0 )
    {
        jaddstr(retjson,"buycoin",buycoin->symbol);
        jaddnum(retjson,"buyforce",maxval);
    }
    if ( sellcoin != 0 )
    {
        jaddstr(retjson,"sellcoin",sellcoin->symbol);
        jaddnum(retjson,"sellforce",minval);
    }
    if ( LP_portfolio_relvolume > SMALLVAL )
    {
        jaddstr(retjson,"base",LP_portfolio_base);
        jaddstr(retjson,"rel",LP_portfolio_rel);
        jaddnum(retjson,"relvolume",LP_portfolio_relvolume);
    }
    jadd(retjson,"portfolio",array);
    return(jprint(retjson,1));
}

char *LP_portfolio_goal(char *symbol,double goal)
{
    struct iguana_info *coin,*tmp; int32_t iter,n = 0; double kmdbtc = 50.;
    if ( strcmp(symbol,"*") == 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            HASH_ITER(hh,LP_coins,coin,tmp)
            {
                if ( coin->inactive != 0 )
                    continue;
                if ( iter == 0 )
                    coin->goal = 0;
                if ( coin->inactive == 0 && strcmp(coin->symbol,"KMD") != 0 && strcmp(coin->symbol,"BTC") != 0 )
                {
                    if ( iter == 0 )
                        n++;
                    else coin->goal = (100. - kmdbtc) / n;
                }
            }
            if ( n == 0 )
                break;
        }
        if ( (coin= LP_coinfind("KMD")) != 0 && coin->inactive == 0 )
            coin->goal = kmdbtc * 0.5;
        if ( (coin= LP_coinfind("BTC")) != 0 && coin->inactive == 0 )
            coin->goal = kmdbtc * 0.5;
        if ( coin->goal != 0 )
            coin->obooktime = (uint32_t)time(NULL);
        return(LP_portfolio());
    }
    else if ( (coin= LP_coinfind(symbol)) != 0 && coin->inactive == 0 )
    {
        coin->goal = goal;
        printf("set %s goal %f\n",coin->symbol,goal);
        if ( coin->goal != 0 )
            coin->obooktime = (uint32_t)time(NULL);
        return(LP_portfolio());
    } else return(clonestr("{\"error\":\"cant set goal for inactive coin\"}"));
}

/*int32_t LP_autofill(char *base,char *rel,double maxprice,double totalrelvolume)
{
    struct LP_priceinfo *basepp,*relpp;
    if ( (basepp= LP_priceinfofind(base)) != 0 && (relpp= LP_priceinfofind(rel)) != 0 )
    {
        basepp->maxprices[relpp->ind] = maxprice;
        basepp->relvols[relpp->ind] = totalrelvolume;
        LP_autofills++;
        return(0);
    }
    return(-1);
}*/

void LP_autopriceset(int32_t ind,void *ctx,int32_t dir,struct LP_priceinfo *basepp,struct LP_priceinfo *relpp,double price,char *refbase,char *refrel)
{
    static uint32_t lasttime;
    double margin,minprice,newprice,oppomargin,fixedprice,factor,offset; double bid,ask; int32_t changed;
    margin = basepp->sellmargins[relpp->ind];
    oppomargin = basepp->buymargins[relpp->ind];
    if ( (fixedprice= basepp->fixedprices[relpp->ind]) > SMALLVAL )
    {
        LP_mypriceset(1,&changed,relpp->symbol,basepp->symbol,fixedprice);
        //printf("autoprice FIXED %s/%s <- %.8f\n",basepp->symbol,relpp->symbol,fixedprice);
        LP_pricepings(ctx,LP_myipaddr,LP_mypubsock,relpp->symbol,basepp->symbol,fixedprice);
        return;
    }
    if ( margin != 0. || oppomargin != 0. )
    {
        offset = basepp->offsets[relpp->ind];
        factor = basepp->factors[relpp->ind];
        if ( fabs(price) < SMALLVAL && refbase != 0 && refrel != 0 )
        {
            price = LP_myprice(1,&bid,&ask,refbase,refrel);
            //printf("%s/%s USE ref %s/%s %.8f factor %.8f offset %.8f margin %.8f/%.8f\n",basepp->symbol,relpp->symbol,refbase,refrel,price,factor,offset,oppomargin,margin);
        }
        if ( LP_pricevalid(price) > 0 )
        {
            if ( factor > SMALLVAL )
            {
                //double tmp = (price * factor) + offset;
                //printf("price %.8f -> %.8f factor %.8f offset %.8f margin %.8f [%.8f %.8f] [%.8f %.8f]\n",price,tmp,factor,offset,margin,(tmp * (1. + margin)),1./(tmp * (1. - margin)),(tmp * (1. - margin)),1./(tmp * (1. + margin)));
                price = (price * factor) + offset;
            }
            if ( margin == 0. )
                margin = oppomargin;
            //printf("min %.8f %s/%s %.8f dir.%d margin %.8f (%.8f %.8f)\n",basepp->minprices[relpp->ind],relpp->symbol,basepp->symbol,price,dir,margin,1. / (price * (1. - margin)),(price * (1. + margin)));
            if ( dir > 0 )
                newprice = (1. / price) * (1. + margin);
            else newprice = (price * (1. + margin));
            if ( (minprice= basepp->minprices[relpp->ind]) == 0. || price >= minprice )
            {
                if ( ind >= 0 )
                {
                    if ( LP_autorefs[ind].lastask < SMALLVAL )
                        LP_autorefs[ind].lastask = newprice;
                    else LP_autorefs[ind].lastask = (LP_autorefs[ind].lastask * 0.99) + (0.01 *newprice);
                    newprice = LP_autorefs[ind].lastask;
                    //printf("autopriceset %s/%s <- %.8f %.8f (%.8f %.8f)\n",basepp->symbol,relpp->symbol,price,newprice,LP_autorefs[ind].lastbid,LP_autorefs[ind].lastask);
                }
                LP_mypriceset(1,&changed,relpp->symbol,basepp->symbol,newprice);
                if ( changed != 0 || time(NULL) > lasttime+LP_ORDERBOOK_DURATION*.777)
                {
                    lasttime = (uint32_t)time(NULL);
                    LP_pricepings(ctx,LP_myipaddr,LP_mypubsock,relpp->symbol,basepp->symbol,newprice);
                }
            }
        }
    }
}

double LP_pricesparse(void *ctx,int32_t trexflag,char *retstr,struct LP_priceinfo *btcpp)
{
    //{"success":true,"message":"","result":[{"MarketName":"BTC-KMD","High":0.00040840,"Low":0.00034900,"Volume":328042.46061669,"Last":0.00037236,"BaseVolume":123.36439511,"TimeStamp":"2017-07-15T13:50:21.87","Bid":0.00035721,"Ask":0.00037069,"OpenBuyOrders":343,"OpenSellOrders":1690,"PrevDay":0.00040875,"Created":"2017-02-11T23:04:01.853"},
    //{"TradePairId":4762,"Label":"WAVES/BTC","AskPrice":0.00099989,"BidPrice":0.00097350,"Low":0.00095000,"High":0.00108838,"Volume":6501.24403100,"LastPrice":0.00098028,"BuyVolume":1058994.86554882,"SellVolume":2067.87377158,"Change":-7.46,"Open":0.00105926,"Close":0.00098028,"BaseVolume":6.52057452,"BuyBaseVolume":2.33098660,"SellBaseVolume":1167.77655709},
    int32_t i,j,n,iter; double price,kmdbtc,bid,ask,nxtkmd=0.; struct LP_priceinfo *coinpp,*refpp; char symbol[65],*name,*refcoin; cJSON *retjson,*array,*item;
    if ( (retjson= cJSON_Parse(retstr)) != 0 )
    {
        //printf("got.(%s)\n",retstr);
        kmdbtc = 0.;
        refcoin = "BTC";
        refpp = btcpp;
        if ( (array= jarray(&n,retjson,trexflag != 0 ? "result" : "Data")) != 0 )
        {
            for (iter=0; iter<2; iter++)
            {
                for (i=0; i<n; i++)
                {
                    item = jitem(array,i);
                    if ( (name= jstr(item,trexflag != 0 ? "MarketName" : "Label")) != 0 )
                    {
                        symbol[0] = 0;
                        if ( trexflag != 0 )
                        {
                            if ( strncmp("BTC-",name,4) == 0 )
                            {
                                name += 4;
                                strcpy(symbol,name);
                            }
                        }
                        else
                        {
                            for (j=0; j<sizeof(symbol)-1; j++)
                                if ( (symbol[j]= name[j]) == '/' )
                                    break;
                            symbol[j] = 0;
                            if ( strcmp(name+j+1,"BTC") != 0 )
                                continue;
                        }
                        if ( symbol[0] != 0 )
                        {
                            //printf("%s\n",jprint(item,0));
                            bid = jdouble(item,trexflag != 0 ? "Bid" : "BidPrice");
                            ask = jdouble(item,trexflag != 0 ? "Ask" : "AskPrice");
                            if ( iter == 1 && kmdbtc > SMALLVAL && strcmp(symbol,"NXT") == 0 )
                                nxtkmd = 0.5 * (bid + ask) / kmdbtc;
                            if ( (coinpp= LP_priceinfofind(symbol)) != 0 )
                            {
                                coinpp->high[trexflag] = jdouble(item,"High");
                                coinpp->low[trexflag] = jdouble(item,"Low");
                                //coinpp->volume = jdouble(item,"Volume");
                                //coinpp->btcvolume = jdouble(item,"BaseVolume");
                                coinpp->last[trexflag] = jdouble(item,trexflag != 0 ? "Last" : "LastPrice");
                                coinpp->bid[trexflag] = bid;
                                coinpp->ask[trexflag] = ask;
                                //coinpp->prevday = jdouble(item,"PrevDay");
                                //printf("iter.%d trexflag.%d %s high %.8f, low %.8f, last %.8f hbla.(%.8f %.8f)\n",iter,trexflag,symbol,coinpp->high[trexflag],coinpp->low[trexflag],coinpp->last[trexflag],coinpp->bid[trexflag],coinpp->ask[trexflag]);
                                if ( coinpp->bid[trexflag] > SMALLVAL && coinpp->ask[trexflag] > SMALLVAL )
                                {
                                    price = 0.5 * (coinpp->bid[trexflag] + coinpp->ask[trexflag]);
                                    if ( iter == 0 )
                                    {
                                        if ( strcmp(symbol,"KMD") == 0 )
                                            kmdbtc = price;
                                    }
                                    else
                                    {
                                        if ( strcmp(symbol,"KMD") == 0 )
                                            continue;
                                        //printf("(%s/%s) iter.%d trexflag.%d %s %.8f %.8f\n",refpp->symbol,coinpp->symbol,iter,trexflag,symbol,price,price/kmdbtc);
                                        price /= kmdbtc;
                                    }
                                    if ( trexflag == 0 && coinpp->bid[1] > SMALLVAL && coinpp->ask[1] > SMALLVAL )
                                    {
                                        //printf("have trex: iter.%d trexflag.%d %s %.8f %.8f\n",iter,trexflag,symbol,coinpp->bid[1],coinpp->ask[1]);
                                        continue;
                                    }
                                    LP_autopriceset(-1,ctx,1,coinpp,refpp,price,0,0);
                                    LP_autopriceset(-1,ctx,-1,refpp,coinpp,price,0,0);
                                }
                            }
                        }
                    }
                }
                refcoin = "KMD";
                if ( kmdbtc == 0. || (refpp= LP_priceinfofind("KMD")) == 0 )
                    break;
            }
        }
        free_json(retjson);
    }
    return(nxtkmd);
}

double LP_autoprice_newprice(int32_t bidask,double price,double newprice)
{
    double gap; int32_t r;
    if ( price > SMALLVAL && ((bidask == 0 && newprice < price) || (bidask != 0 && newprice > price)) )
    {
        gap = fabs(newprice - price) * 2;
        r = (rand() % 100);
        if ( bidask == 0 )
            price -= (gap * r) / 100.;
        else price += (gap * r) / 100.;
    }
    else if ( price > SMALLVAL )
        price = (price * 0.95) + (0.05 * newprice);
    else price = newprice;
    return(price);
}

double LP_tickered_price(int32_t bidask,char *base,char *rel,double price,cJSON *tickerjson)
{
    int32_t i,n; cJSON *item; double basevol,relvol,itemprice;
    //printf("%s %s/%s %.8f -> ",bidask == 0 ? "bid" : "ask",base,rel,price);
    if ( (n= cJSON_GetArraySize(tickerjson)) > 0 )
    {
        for (i=n-1; i>=0; i--)
        {
            // {"timestamp":1513235320,"KMD":860.45202538,"SUPERNET":20.00010000,"price":0.02324371}
            item = jitem(tickerjson,i);
            if ( (basevol= jdouble(item,base)) > SMALLVAL && (relvol= jdouble(item,rel)) > SMALLVAL )
            {
                itemprice = (relvol / basevol);
                //printf("%.8f ",itemprice);
                price = LP_autoprice_newprice(bidask,price,itemprice);
            }
        }
    }
    //printf("-> %.8f\n",price);
    return(price);
}

int32_t LP_autoref_clear(char *base,char *rel)
{
    int32_t i;
    for (i=0; i<num_LP_autorefs; i++)
    {
        if ( (strcmp(rel,LP_autorefs[i].rel) == 0 && strcmp(base,LP_autorefs[i].base) == 0) ||
            (strcmp(base,LP_autorefs[i].rel) == 0 && strcmp(rel,LP_autorefs[i].base) == 0) )
        {
            memset(&LP_autorefs[i],0,sizeof(LP_autorefs[i]));
            return(i);
        }
    }
    return(-1);
}

void LP_autoprice_iter(void *ctx,struct LP_priceinfo *btcpp)
{
    static cJSON *tickerjson; static uint32_t lasttime;
    char *retstr,*base,*rel; cJSON *retjson,*bid,*ask,*fundjson,*argjson; uint64_t bidsatoshis,asksatoshis; int32_t i,changed; double bidprice,askprice,bch_usd,ltc_btc,bch_btc,nxtkmd,price,factor,offset,newprice,buymargin,sellmargin,price_btc,price_usd,kmd_btc,kmd_usd; struct LP_priceinfo *kmdpp,*fiatpp,*nxtpp,*basepp,*relpp;
    printf("AUTOPRICE numautorefs.%d\n",num_LP_autorefs);
    if ( (retstr= issue_curlt("https://bittrex.com/api/v1.1/public/getmarketsummaries",LP_HTTP_TIMEOUT*10)) == 0 )
    {
        printf("trex error getting marketsummaries\n");
        sleep(40);
        //return;
    }
    else
    {
        nxtkmd = LP_pricesparse(ctx,1,retstr,btcpp);
        free(retstr);
    }
    if ( (retstr= issue_curlt("https://www.cryptopia.co.nz/api/GetMarkets",LP_HTTP_TIMEOUT*10)) == 0 )
    {
        printf("cryptopia error getting marketsummaries\n");
        sleep(40);
        //return;
    }
    else
    {
        LP_pricesparse(ctx,0,retstr,btcpp);
        free(retstr);
    }
    if ( (kmdpp= LP_priceinfofind("KMD")) != 0 )
    {
        for (i=0; i<32; i++)
        {break;
            if ( (fiatpp= LP_priceinfofind(CURRENCIES[i])) != 0 )
            {
                if ( (retjson= LP_paxprice(CURRENCIES[i])) != 0 )
                {
                    //printf("(%s %.8f %.8f) ",CURRENCIES[i],jdouble(retjson,"price"),jdouble(retjson,"invprice"));
                    price = jdouble(retjson,"price");
                    LP_autopriceset(-1,ctx,1,fiatpp,kmdpp,price,0,0);
                    LP_autopriceset(-1,ctx,-1,kmdpp,fiatpp,price,0,0);
                    free_json(retjson);
                }
            }
        }
    }
    if ( 0 && nxtkmd > SMALLVAL )
    {
        for (i=0; i<sizeof(assetids)/sizeof(*assetids); i++)
        {
            if ( (nxtpp= LP_priceinfofind(assetids[i][1])) != 0 )
            {
                price = 0.;
                bidsatoshis = asksatoshis = 0;
                if ( (retjson= LP_assethbla(assetids[i][0])) != 0 )
                {
                    if ( (bid= jobj(retjson,"bid")) != 0 && (ask= jobj(retjson,"ask")) != 0 )
                    {
                        bidsatoshis = j64bits(bid,"priceNQT") * atoi(assetids[i][2]);
                        asksatoshis = j64bits(ask,"priceNQT") * atoi(assetids[i][2]);
                        if ( bidsatoshis != 0 && asksatoshis != 0 )
                            price = 0.5 * dstr(bidsatoshis + asksatoshis) * nxtkmd;
                    }
                    LP_autopriceset(-1,ctx,1,nxtpp,kmdpp,price,0,0);
                    LP_autopriceset(-1,ctx,-1,kmdpp,nxtpp,price,0,0);
                    //printf("%s %s -> (%s) nxtkmd %.8f %.8f %.8f\n",assetids[i][1],assetids[i][0],jprint(retjson,0),nxtkmd,0.5*dstr(bidsatoshis + asksatoshis),price);
                    free_json(retjson);
                }
            }
        }
    }
    if ( time(NULL) > lasttime+60 )
    {
        if ( tickerjson != 0 )
            free_json(tickerjson);
        if ( (retstr= LP_ticker("","")) != 0 )
        {
            tickerjson = cJSON_Parse(retstr);
            free(retstr);
        }
    }
    kmd_btc = LP_CMCbtcprice(&kmd_usd,"komodo");
    bch_btc = LP_CMCbtcprice(&bch_usd,"bitcoin-cash");
    ltc_btc = LP_CMCbtcprice(&bch_usd,"litecoin");
    for (i=0; i<num_LP_autorefs; i++)
    {
        rel = LP_autorefs[i].rel;
        base = LP_autorefs[i].base;
        if ( rel[0] == 0 || base[0] == 0 )
            continue;
        buymargin = LP_autorefs[i].buymargin;
        sellmargin = LP_autorefs[i].sellmargin;
        offset = LP_autorefs[i].offset;
        factor = LP_autorefs[i].factor;
        if ( (argjson= LP_autorefs[i].fundvalue) != 0 )
        {
            if ( (fundjson= LP_fundvalue(argjson)) != 0 )
            {
                //printf("%s\n",jprint(fundjson,0));
                if ( jint(fundjson,"missing") == 0 )
                {
                    if ( LP_autorefs[i].fundbid[0] != 0 && (bidprice= jdouble(fundjson,LP_autorefs[i].fundbid)) > SMALLVAL && LP_autorefs[i].fundask[0] != 0 && (askprice= jdouble(fundjson,LP_autorefs[i].fundask)) > SMALLVAL )
                    {
                        price = (bidprice + askprice) * 0.5;
                        bidprice = (1. / price * (1. + buymargin));
                        if ( LP_autorefs[i].lastbid < SMALLVAL )
                            LP_autorefs[i].lastbid = bidprice;
                        else LP_autorefs[i].lastbid = (LP_autorefs[i].lastbid * 0.9) + (0.1 * bidprice);
                        bidprice = LP_autorefs[i].lastbid;
                        askprice = price * (1. + sellmargin);
                        if ( LP_autorefs[i].lastask < SMALLVAL )
                            LP_autorefs[i].lastask = askprice;
                        else LP_autorefs[i].lastask = (LP_autorefs[i].lastask * 0.9) + (0.1 * askprice);
                        askprice = LP_autorefs[i].lastask;
                        LP_mypriceset(1,&changed,rel,base,bidprice);
                        LP_pricepings(ctx,LP_myipaddr,LP_mypubsock,rel,base,bidprice);
                        LP_mypriceset(1,&changed,base,rel,askprice);
                        LP_pricepings(ctx,LP_myipaddr,LP_mypubsock,base,rel,askprice);
                        //printf("price %.8f -> %.8f %.8f\n",price,bidprice,askprice);
                    }
                    LP_autorefs[i].count++;
                }
                free_json(fundjson);
            }
        }
        else if ( strcmp(LP_autorefs[i].refrel,"coinmarketcap") == 0 )
        {
            //printf("%s/%s for %s/%s margin %.8f/%.8f\n",base,rel,LP_autorefs[i].refbase,LP_autorefs[i].refrel,buymargin,sellmargin);
            if ( (price_btc= LP_CMCbtcprice(&price_usd,LP_autorefs[i].refbase)) > SMALLVAL )
            {
                if ( LP_autorefs[i].usdpeg != 0 )
                {
                    if ( price_usd > SMALLVAL )
                        price = 1. / price_usd;
                    else continue;
                }
                else
                {
                    if ( strcmp(rel,"KMD") == 0 && kmd_btc > SMALLVAL )
                        price = kmd_btc / price_btc;
                    else if ( strcmp(rel,"BCH") == 0 && bch_btc > SMALLVAL )
                        price = bch_btc / price_btc;
                    else if ( strcmp(rel,"LTC") == 0 && ltc_btc > SMALLVAL )
                        price = ltc_btc / price_btc;
                    else if ( strcmp(rel,"BTC") == 0 )
                        price = 1. / price_btc;
                    else continue;
                }
                if ( factor > 0. )
                {
                    //printf("USD %.8f KMDBTC %.8f pricebtc %.8f price %.8f -> factor %.8f %.8f\n",price_usd,kmd_btc,price_btc,price,factor,(price * factor) + offset);
                    price = (price * factor) + offset;
                }
                newprice = (price * (1. + buymargin));
                if ( LP_autorefs[i].lastbid < SMALLVAL )
                    LP_autorefs[i].lastbid = newprice;
                else LP_autorefs[i].lastbid = (LP_autorefs[i].lastbid * 0.99) + (0.01 * newprice);
                newprice = LP_autorefs[i].lastbid;
                LP_mypriceset(1,&changed,rel,base,newprice);
                LP_pricepings(ctx,LP_myipaddr,LP_mypubsock,rel,base,newprice);
                //printf("%s/%s price %.8f margin %.8f/%.8f newprice %.8f %.8f\n",base,rel,price,buymargin,sellmargin,newprice,(1. / newprice) * (1. + sellmargin));
                newprice = (1. / price) * (1. + sellmargin);
                if ( LP_autorefs[i].lastask < SMALLVAL )
                    LP_autorefs[i].lastask = newprice;
                else LP_autorefs[i].lastask = (LP_autorefs[i].lastask * 0.99) + (0.01 * newprice);
                newprice = LP_autorefs[i].lastask;
                LP_mypriceset(1,&changed,base,rel,newprice);
                LP_pricepings(ctx,LP_myipaddr,LP_mypubsock,base,rel,newprice);
            } //else printf("null return from CMC\n");
        }
        else
        {
            basepp = LP_priceinfofind(base);
            relpp = LP_priceinfofind(rel);
            if ( basepp != 0 && relpp != 0 )
            {
                //printf("check ref-autoprice %s/%s %f %f (%.8f %.8f)\n",LP_autorefs[i].refbase,LP_autorefs[i].refrel,relpp->fixedprices[basepp->ind],basepp->fixedprices[relpp->ind],LP_autorefs[i].lastbid,LP_autorefs[i].lastask);
                LP_autopriceset(i,ctx,1,basepp,relpp,0.,LP_autorefs[i].refbase,LP_autorefs[i].refrel);
            }
        }
    }
}

void LP_autoprices_update(char *method,char *base,double basevol,char *rel,double relvol)
{
    int32_t i; double price,newprice;
    if ( basevol > 0. && relvol > 0. )
    {
        price = relvol/basevol;
        for (i=0; i<num_LP_autorefs; i++)
        {
            if ( strcmp(LP_autorefs[i].rel,rel) == 0 && strcmp(base,LP_autorefs[i].base) == 0 )
            {
                newprice = (LP_autorefs[i].lastask * 0.9) + (0.1 * price);
                if ( LP_autorefs[i].lastask > 0 )
                {
                    //printf("%s: autoprice ask update %s/%s %.8f vs myprice %.8f/%.8f -> %.8f\n",method,base,rel,price,LP_autorefs[i].lastbid,LP_autorefs[i].lastask,newprice);
                    LP_autorefs[i].lastask = newprice;
                } // else printf("%s: autoprice ask skip update %s/%s %.8f vs myprice %.8f/%.8f -> %.8f\n",method,base,rel,price,LP_autorefs[i].lastbid,LP_autorefs[i].lastask,newprice);
            }
            else if ( strcmp(LP_autorefs[i].rel,base) == 0 && strcmp(rel,LP_autorefs[i].base) == 0 )
            {
                newprice = (LP_autorefs[i].lastbid * 0.9) + (0.1 * price);
                if ( LP_autorefs[i].lastbid > 0 )
                {
                    //printf("%s: autoprice bid update %s/%s %.8f vs myprice %.8f/%.8f -> %.8f\n",method,base,rel,price,LP_autorefs[i].lastbid,LP_autorefs[i].lastask,newprice);
                    LP_autorefs[i].lastbid = newprice;
                } // else printf("%s: autoprice bid skip update %s/%s %.8f vs myprice %.8f/%.8f -> %.8f\n",method,base,rel,price,LP_autorefs[i].lastbid,LP_autorefs[i].lastask,newprice);
            }
        }
    }
}

int32_t LP_autoprice(void *ctx,char *base,char *rel,cJSON *argjson)
{
    //curl --url "http://127.0.0.1:7783" --data "{\"userpass\":\"$userpass\",\"method\":\"autoprice\",\"base\":\"MNZ\",\"rel\":\"KMD\",\"offset\":0.1,\"refbase\":\"KMD\",\refrel\":\"BTC\",\"factor\":15000,\"margin\":0.01}"
    struct LP_priceinfo *basepp,*relpp; int32_t i,retval = -1; char *fundvalue_bid,*fundvalue_ask,*refbase="",*refrel=""; double margin,minprice,buymargin,sellmargin,offset,factor,fixedprice; cJSON *fundvalue;
    //printf("autoprice.(%s %s) %s\n",base,rel,jprint(argjson,0));
    if ( (basepp= LP_priceinfofind(base)) != 0 && (relpp= LP_priceinfofind(rel)) != 0 )
    {
        if ( jobj(argjson,"minprice") != 0 )
            minprice = jdouble(argjson,"minprice");
        else minprice = 0.;
        if ( (margin= jdouble(argjson,"margin")) == 0. )
        {
            buymargin = jdouble(argjson,"buymargin");
            sellmargin = jdouble(argjson,"sellmargin");
        }
        else
        {
            buymargin = margin;
            sellmargin = margin;
        }
        offset = jdouble(argjson,"offset");
        factor = jdouble(argjson,"factor");
        fixedprice = jdouble(argjson,"fixed");
        basepp->fixedprices[relpp->ind] = fixedprice;
        basepp->minprices[relpp->ind] = minprice;
        if ( jobj(argjson,"maxprice") != 0 )
            relpp->minprices[basepp->ind] = 1. / jdouble(argjson,"maxprice");
        basepp->buymargins[relpp->ind] = buymargin;
        basepp->sellmargins[relpp->ind] = sellmargin;
        basepp->offsets[relpp->ind] = offset;
        basepp->factors[relpp->ind] = factor;
        refbase = jstr(argjson,"refbase");
        refrel = jstr(argjson,"refrel");
        fundvalue_bid = jstr(argjson,"fundvalue_bid");
        fundvalue_ask = jstr(argjson,"fundvalue_ask");
        if ( fundvalue_bid != 0 || fundvalue_ask != 0 || fixedprice > SMALLVAL || (refbase != 0 && refrel != 0 && strlen(refbase) > 0 && strlen(refrel) > 0) )
        {
            if ( fixedprice > SMALLVAL )
            {
                refbase = base;
                refrel = rel;
            }
            for (i=0; i<num_LP_autorefs; i++)
            {
                if ( strcmp(base,LP_autorefs[i].base) == 0 && strcmp(rel,LP_autorefs[i].rel) == 0 )
                {
                    if ( fundvalue_bid != 0 || fundvalue_ask != 0 )
                    {
                        fundvalue = jduplicate(argjson);
                        jdelete(fundvalue,"method");
                        jaddstr(fundvalue,"method","fundvalue");
                        LP_autorefs[i].fundvalue = fundvalue;
                        safecopy(LP_autorefs[i].fundbid,fundvalue_bid,sizeof(LP_autorefs[i].fundbid));
                        safecopy(LP_autorefs[i].fundask,fundvalue_ask,sizeof(LP_autorefs[i].fundask));
                    }
                    LP_autorefs[i].buymargin = buymargin;
                    LP_autorefs[i].sellmargin = sellmargin;
                    LP_autorefs[i].factor = factor;
                    LP_autorefs[i].offset = offset;
                    safecopy(LP_autorefs[i].refbase,refbase,sizeof(LP_autorefs[i].refbase));
                    safecopy(LP_autorefs[i].refrel,refrel,sizeof(LP_autorefs[i].refrel));
                    printf("%d Update ref %s/%s for %s/%s factor %.8f offset %.8f\n",i,refbase,refrel,base,rel,factor,offset);
                    break;
                }
            }
            if ( i == num_LP_autorefs && num_LP_autorefs < sizeof(LP_autorefs)/sizeof(*LP_autorefs) )
            {
                if ( fundvalue_bid != 0 || fundvalue_ask != 0 )
                {
                    fundvalue = jduplicate(argjson);
                    jdelete(fundvalue,"method");
                    jaddstr(fundvalue,"method","fundvalue");
                    LP_autorefs[num_LP_autorefs].fundvalue = fundvalue;
                    safecopy(LP_autorefs[num_LP_autorefs].fundbid,fundvalue_bid,sizeof(LP_autorefs[num_LP_autorefs].fundbid));
                    safecopy(LP_autorefs[num_LP_autorefs].fundask,fundvalue_ask,sizeof(LP_autorefs[num_LP_autorefs].fundask));
                }
                LP_autorefs[num_LP_autorefs].usdpeg = juint(argjson,"usdpeg");
                LP_autorefs[num_LP_autorefs].buymargin = buymargin;
                LP_autorefs[num_LP_autorefs].sellmargin = sellmargin;
                LP_autorefs[num_LP_autorefs].factor = factor;
                LP_autorefs[num_LP_autorefs].offset = offset;
                safecopy(LP_autorefs[num_LP_autorefs].refbase,refbase,sizeof(LP_autorefs[num_LP_autorefs].refbase));
                safecopy(LP_autorefs[num_LP_autorefs].refrel,refrel,sizeof(LP_autorefs[num_LP_autorefs].refrel));
                safecopy(LP_autorefs[num_LP_autorefs].base,base,sizeof(LP_autorefs[num_LP_autorefs].base));
                safecopy(LP_autorefs[num_LP_autorefs].rel,rel,sizeof(LP_autorefs[num_LP_autorefs].rel));
                printf("%d Using ref %s/%s for %s/%s factor %.8f, offset %.8f, margin %.8f/%.8f fixed %.8f\n",num_LP_autorefs,refbase,refrel,base,rel,factor,offset,buymargin,sellmargin,fixedprice);
                num_LP_autorefs++;
            }
        }
        LP_autoprices++;
        retval = 0;
    }
    //LP_autoprice_iter(ctx,LP_priceinfofind("BTC"));
    return(retval);
}

int32_t LP_portfolio_trade(void *ctx,uint32_t *requestidp,uint32_t *quoteidp,struct iguana_info *buy,struct iguana_info *sell,double relvolume,int32_t setbaserel,char *gui)
{
    char *retstr2; uint64_t txfee,desttxfee; double bid,ask,maxprice; bits256 zero; uint32_t requestid,quoteid,iter,i; cJSON *retjson2; struct LP_utxoinfo A; struct LP_address_utxo *utxos[1000]; int32_t max=(int32_t)(sizeof(utxos)/sizeof(*utxos));
    LP_txfees(&txfee,&desttxfee,buy->symbol,sell->symbol);
    requestid = quoteid = 0;
    LP_myprice(1,&bid,&ask,buy->symbol,sell->symbol);
    maxprice = ask;
    if ( setbaserel != 0 )
    {
        strcpy(LP_portfolio_base,"");
        strcpy(LP_portfolio_rel,"");
        LP_portfolio_relvolume = 0.;
    }
    printf("pending.%d base buy.%s, rel sell.%s relvolume %f maxprice %.8f (%.8f %.8f)\n",G.LP_pendingswaps,buy->symbol,sell->symbol,sell->relvolume,maxprice,bid,ask);
    if ( LP_pricevalid(maxprice) > 0 )
    {
        relvolume = sell->relvolume;
        for (iter=0; iter<2; iter++)
        {
            if ( relvolume < dstr(LP_MIN_TXFEE) )
                break;
            if ( LP_address_myutxopair(&A,0,utxos,max,sell,sell->symbol,txfee,relvolume,maxprice,desttxfee) == 0 )
            //if ( LP_utxo_bestfit(sell->symbol,SATOSHIDEN * relvolume) != 0 )
            {
                memset(zero.bytes,0,sizeof(zero));
                if ( (retstr2= LP_autobuy(ctx,0,"127.0.0.1",-1,buy->symbol,sell->symbol,maxprice,relvolume,60,24*3600,gui,LP_lastnonce+1,zero,1,0,0,0)) != 0 )
                {
                    if ( (retjson2= cJSON_Parse(retstr2)) != 0 )
                    {
                        if ( (requestid= juint(retjson2,"requestid")) != 0 && (quoteid= juint(retjson2,"quoteid")) != 0 )
                        {
                            
                        }
                        free_json(retjson2);
                    }
                    printf("%s relvolume %.8f LP_autotrade.(%s)\n",sell->symbol,relvolume,retstr2);
                    free(retstr2);
                }
                if ( requestid != 0 && quoteid != 0 )
                    break;
            } else printf("cant find alice %.8f %s\n",relvolume,sell->symbol);
            if ( iter == 0 )
            {
                for (i=0; i<100; i++)
                {
                    relvolume *= .99;
                    if ( LP_address_myutxopair(&A,0,utxos,max,sell,sell->symbol,txfee,relvolume,maxprice,desttxfee) == 0 )
                    //if ( LP_utxo_bestfit(sell->symbol,SATOSHIDEN * relvolume) != 0 )
                    {
                        printf("i.%d relvolume %.8f from %.8f\n",i,relvolume,sell->relvolume);
                        break;
                    }
                }
            }
        }
    }
    else if ( setbaserel != 0 )
    {
        strcpy(LP_portfolio_base,buy->symbol);
        strcpy(LP_portfolio_rel,sell->symbol);
        LP_portfolio_relvolume = sell->relvolume;
    }
    *requestidp = requestid;
    *quoteidp = quoteid;
    if ( requestid != 0 && quoteid != 0 )
        return(0);
    else return(-1);
}

int32_t LP_portfolio_order(struct LP_portfoliotrade *trades,int32_t max,cJSON *array)
{
    int32_t i,j,m,n = 0; cJSON *item; struct LP_portfoliotrade coins[256];
    memset(coins,0,sizeof(coins));
    if ( (m= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=j=0; i<m && i<sizeof(coins)/sizeof(*coins); i++)
        {
            item = jitem(array,i);
            safecopy(coins[j].buycoin,jstr(item,"coin"),sizeof(coins[j].buycoin));
            coins[j].metric = jdouble(item,"force");
            if ( fabs(coins[j].metric) > SMALLVAL && coins[j].buycoin[0] != 0 )
                j++;
        }
        if ( (m= j) > 1 )
        {
            for (i=n=0; i<m-1; i++)
                for (j=i+1; j<m; j++)
                    if ( coins[i].metric*coins[j].metric < 0. )
                    {
                        if ( coins[i].metric > 0. )
                        {
                            trades[n].metric = (coins[i].metric - coins[j].metric);
                            strcpy(trades[n].buycoin,coins[i].buycoin);
                            strcpy(trades[n].sellcoin,coins[j].buycoin);
                            printf("buy %s %f, sell %s %f -> %f\n",trades[n].buycoin,coins[i].metric,trades[n].sellcoin,coins[j].metric,trades[n].metric);
                        }
                        else
                        {
                            trades[n].metric = (coins[j].metric - coins[i].metric);
                            strcpy(trades[n].buycoin,coins[j].buycoin);
                            strcpy(trades[n].sellcoin,coins[i].buycoin);
                            printf("buy %s %f, sell %s %f -> %f\n",trades[n].buycoin,coins[j].metric,trades[n].sellcoin,coins[i].metric,trades[n].metric);
                        }
                        n++;
                        if ( n >= max )
                            break;
                    }
            revsortds((void *)trades,n,sizeof(*trades));
            for (i=0; i<n; i++)
                printf("%d: buy %s, sell %s -> %f\n",i,trades[i].buycoin,trades[i].sellcoin,trades[i].metric);
        }
    }
    return(n);
}

void prices_loop(void *ctx)
{
    char *retstr; cJSON *retjson,*array; char *buycoin,*sellcoin; struct iguana_info *buy,*sell; uint32_t requestid,quoteid; int32_t i,n,m; struct LP_portfoliotrade trades[256]; struct LP_priceinfo *btcpp;
    strcpy(prices_loop_stats.name,"prices_loop");
    prices_loop_stats.threshold = 600000.;
    printf("start prices_loop\n");
    while ( LP_STOP_RECEIVED == 0 )
    {
        //printf("G.initializing.%d prices loop autoprices.%d autorefs.%d\n",G.initializing,LP_autoprices,num_LP_autorefs);
        if ( G.initializing != 0 )
        {
            sleep(30);
            continue;
        }
        LP_millistats_update(&prices_loop_stats);
        LP_tradebots_timeslice(ctx);
        if ( (btcpp= LP_priceinfofind("BTC")) == 0 )
        {
            printf("prices_loop BTC not in LP_priceinfofind\n");
            sleep(60);
            continue;
        }
        if ( LP_autoprices != 0 )
            LP_autoprice_iter(ctx,btcpp);
        if ( (retstr= LP_portfolio()) != 0 )
        {
            if ( (retjson= cJSON_Parse(retstr)) != 0 )
            {
                if ( (buycoin= jstr(retjson,"buycoin")) != 0 && (buy= LP_coinfind(buycoin)) != 0 && (sellcoin= jstr(retjson,"sellcoin")) != 0 && (sell= LP_coinfind(sellcoin)) != 0 && buy->inactive == 0 && sell->inactive == 0 )
                {
                    if ( LP_portfolio_trade(ctx,&requestid,&quoteid,buy,sell,sell->relvolume,1,"portfolio") < 0 )
                    {
                        array = jarray(&m,retjson,"portfolio");
                        if ( array != 0 && (n= LP_portfolio_order(trades,(int32_t)(sizeof(trades)/sizeof(*trades)),array)) > 0 )
                        {
                            for (i=0; i<n; i++)
                            {
                                if ( strcmp(trades[i].buycoin,buycoin) != 0 || strcmp(trades[i].sellcoin,sellcoin) != 0 )
                                {
                                    buy = LP_coinfind(trades[i].buycoin);
                                    sell = LP_coinfind(trades[i].sellcoin);
                                    if ( buy != 0 && sell != 0 && LP_portfolio_trade(ctx,&requestid,&quoteid,buy,sell,sell->relvolume,0,"portfolio") == 0 )
                                        break;
                                }
                            }
                        }
                    }
                }
                free_json(retjson);
            }
            free(retstr);
        }
        sleep(30);
    }
}


