![alt text](https://bitbucket.org/andersced/cppsrtbondingexample/raw/02a3724aa21c95d4d3e3f59bdcab2f3d68de3ba0/efpbond.png)

# SRT + EFPBonding example

This example uses EFP ([ElasticFramingProtocol](https://bitbucket.org/unitxtra/efp/src/master/)) as a layer between the producer/consumer of data and the bonding layer. 

[EFPBonding](https://bitbucket.org/unitxtra/efpbond/src/master/) plugin sits between EFP and the networ klayer.

As network protocol/transport layer SRT ([SecureReliableTransport](https://github.com/Haivision/srt)) is used.

a SRT C++ wrapper used by this example. The wrapper is located [here](https://github.com/andersc/cppSRTWrapper).

## build


**All architectures:**

```
cmake .
make
mkdir tmp
mv cppSRTFramingClient tmp/
```

**Run the system:**