### dPoW 0.5.8 update information

-On your 3P node, update your Verus Coin's codebase to [d0aecf8](https://github.com/VerusCoin/VerusCoin/tree/d0aecf8fcf099491aadeb427ff47fab7ad96a84b), build it and then restart it

```bash
cd ~/VerusCoin
git pull
git checkout d0aecf8
./zcutil/build.sh -j$(expr $(nproc) - 1)
```

- Restart it

```bash
cd ~/VerusCoin/src
./verus stop
source ~/dPoW/iguana/pubkey.txt
./verusd -pubkey=$pubkey &
```

- Update your dPoW repo

```bash
cd dPoW
git checkout master
git pull
```

Make sure your iguana is running properly.
