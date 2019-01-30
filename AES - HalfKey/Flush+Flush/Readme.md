# Flush+Flush Attack Implementation on AES with Half Key Recovery

Flush+Flush is Cache Side-Channel Attack that was originally proposed by Daniel Gruss et al. of the Graz University of Technology, Austria in their paper entitled, Flush+Flush: A Fast and Stealthy Cache Attack at DIMVA in 2016. This repository contains the code for Flush+Flush attack on AES cryptosystem with half key recovery as implemented by Daniel Gruss. We have reproduced this attack in our own settings/system configuration for experiments. Implementation details are provided in the following.

## AES T-Table Implementations:

Advance Encryption Standard (AES) is widely used symmetric-key encryption algorithm standardised by National Institute of Standards and Technology in 2001.The T-table is optimisation of AES algorithm. Although it requires more memory as compared to standard algorithm, it offers superior performance in implementation. Most libraries use T-table optimisations in their implementations. Bernstein et al. in their paper Cache-timing attacks on AES have presented timing Side-Channel attacks on AES algorithm at first. 

## Machine Specifications:

Intel Core i7_4770 CPU @ 3.40GHz
CPU: 8, L1d Cache: 32KB, L1i Cache : 32KB L2 Cache : 256KB, L3 Cache : 8192 KB
Ubuntu 16.04.3 LTS, Kernel: Linux 4.10.0-28-generic

## AES Library:

We have used 2 version of OpenSSL library, i.e., OpenSSL-0.9.7 and OpenSSL-1.0.1f. Following commands are used to compile library.
	
```
	./config -shared no-hw no-asm
	make
```

## Threshold Calculation:

This implantation contains a tool to generate access timings for clflush operation when data is present in cache and cache is empty. Threshold is taken as average of both timing values for this implementation, Threshold varies for different machines. This tool requires gnuplot and its dependencies must be installed on machine before usage. Threshold on our specified machine is 156-cycles.

## Attack Files (spy.c):
This attack monitors first round of AES encryption. By using probabilistic method and recorded stats of caches, we are able to successfully create upper half bytes of secret key. Attack is compiled using 'make' command.

## Configurations:
This attack requires some machine specific details, you need to edit 'config' file and set it according to your machine, such as:

```
OPENSSL: Contains path of OpenSSL Library
NUMBER_OF_ENCRYPTIONS: Minimum 350, More rounds mean more accurate results 
	
THRESHOLD: Your machine threshold.
TABLE0: AES T-Table 0 starting address
TABLE1: AES T-Table 1 starting address
TABLE2: AES T-Table 2 starting address
TABLE3: AES T-Table 3 starting address
```	

