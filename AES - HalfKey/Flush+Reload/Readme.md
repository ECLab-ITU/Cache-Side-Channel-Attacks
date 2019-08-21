# Flush+Reload Attack Implementation on AES with Half Key Recovery

Flush+Reload is Cache Side-Channel Attack that was originally proposed by Yuval Yarom and Katrina Falkner of the The University of Adelaide in their paper entitled, Flush+Reload: a High Resolution, Low Noise, L3 Cache Side-Channel Attack at 23rd USENIX Security Symposium in 2014. This attack was proposed on RSA Cryptosystem. Later, Danial Gruss extended the work on AES cryptosystem. This repository contains the code for Flush+Reload attack on AES cryptosystem with half key recovery as implemented by Daniel Gruss. We have reproduced this attack in our own settings/system configuration for experiments. Implementation details are provided in the following.

## AES T-Table Implementations:

Advance Encryption Standard (AES) is widely used symmetric-key encryption algorithm standardised by National Institute of Standards and Technology in 2001.The T-table is optimisation of AES algorithm. Although it requires more memory as compared to standard algorithm, it offers superior performance in implementation. Most libraries use T-table optimisations in their implementations. Bernstein et al. in their paper Cache-timing attacks on AES have presented timing Side-Channel attacks on AES algorithm at first. 

## Machine Specifications:

Intel Core i7_4770 CPU @ 3.40GHz
CPU: 8, L1d Cache: 32KB, L1i Cache : 32KB L2 Cache : 256KB, L3 Cache : 8192 KB
Ubuntu 16.04.3 LTS, Kernel: Linux 4.10.0-28-generic

## AES Library:

We have used 2 version of OpenSSL library, i.e., OpenSSL-0.9.7l and OpenSSL-1.0.1f. Following commands are used to compile library.

```
	./config -shared no-hw no-asm
	make
```


## Threshold Calculation:

This implantation contains a tool to generate memory access timings for READ operation from both cache and main memory. Threshold is taken as average of cache and main memory access time for this implementation, Threshold varies for different machines. This tool requires gnuplot and its dependencies must be installed on machine before usage. Threshold on our specified machine is 180-cycles.

## Attack Files (spy.c):

This attack monitors first round of AES encryption. By using probabilistic method and recorded stats of caches, we are able to successfully create upper half bytes of secret key. Attack is compiled using ?make? command.

## Configurations:

This attack requires some machine specific details, you need to edit ?config? file and set it according to your machine, such as:

```
OPENSSL: Contains path of OpenSSL Library
NUMBER_OF_ENCRYPTIONS: Minimum 250, More rounds mean more accurate results 
	
THRESHOLD: Your machine threshold.
TABLE0: AES T-Table 0 starting address
TABLE1: AES T-Table 1 starting address
TABLE2: AES T-Table 2 starting address
TABLE3: AES T-Table 3 starting address
```


