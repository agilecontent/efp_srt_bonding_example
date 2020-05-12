![alt text](efpbond.png)

# SRT + EFPBonding example

This example uses EFP ([ElasticFramingProtocol](https://github.com/Unit-X/efp)) as a layer between the producer/consumer of data and the bonding layer. 

[EFPBonding](https://github.com/Unit-X/efpbond) plugin sits between EFP and the networ klayer.

As network protocol/transport layer SRT ([SecureReliableTransport](https://github.com/Haivision/srt)) is used.

a SRT C++ wrapper used by this example. The wrapper is located [here](https://github.com/andersc/cppSRTWrapper).

## build


**All architectures:**

All dependencies should download and compile.

Change the IP's and ports in the code to work for your environment. Default local host is used.

```sh
cmake .
make
mkdir tmp
mv cppsrtbondingclient tmp/
```

**Run the system:**


```
1. Start the server 
./cppsrtbondingserver
2. then open up another terminal (sampe path) and.
cd tmp
./cppsrtbondingclient
```