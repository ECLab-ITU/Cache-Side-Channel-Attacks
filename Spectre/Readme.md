# Spectre Attack Implementation code on x86

Spectre attack is by Paul Kocher et. al. in their research paper entitled, Spectre Attacks: Exploiting Speculative Execution at 40th IEEE Symposium on Security and Privacy (S\&P'19) in 2019. This attack exploits un-authorized access to program memery by exploiting branch prediction and using cache-based side channel attack (Flush+Reload) as covert channel. This implmenentation use same code as proposed by authors with some modifications (https://github.com/crozone/SpectrePoC).

## Machine Specifications:

Intel Core i7_4770 CPU @ 3.40GHz
CPU: 8, L1d Cache: 32KB, L1i Cache : 32KB L2 Cache : 256KB, L3 Cache : 8192 KB
Ubuntu 16.04.3 LTS, Kernel: Linux 4.10.0-28-generic

## Attack


```
	make
	./spectre 150
```

Default threshold for Flush+Reload is 80 on this attack configurations, where on our machine, it is 180. You should use threshold according to your machine.

