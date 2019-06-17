# Spectre Attack Implementation code on x86

Meltdown attack is proposed by Moritz Lipp et al. in their research paper entitled, Meltdown: Reading Kernel Memory from User Space at 27th {USENIX} Security Symposium ({USENIX} Security 18) in 2018. This attack exploits un-authorized access to kernal space memery and allow adversaries to read complete physical memory at user level. This attack exploit out-of-order executions and with combination of cache-based side channel attack (Flush+Reload) as covert channel able to read kernal memory without necessary permissions. This implmenentation use same code as available at (https://github.com/IAIK/meltdown).

## Machine Specifications:

Intel Core i7_4770 CPU @ 3.40GHz
CPU: 8, L1d Cache: 32KB, L1i Cache : 32KB L2 Cache : 256KB, L3 Cache : 8192 KB
Ubuntu 16.04.3 LTS, Kernel: Linux 4.10.0-28-generic

## Attack

Compile attack using
```
    make
```

### 1. Test

```
    taskset 0x1 ./test
```

### 2. Attack - Reading physical memory

Run secret application which contian some secret data.

```
    sudo ./secret
```

Copy the physical location of secret data and run adversary.

```
    taskset 0x1 ./physical_reader 0x390fff400
```

## Note

This attack requires KASLR disabled, if KASLR is enabled, this attack will not work. To disable KASLR use 'nopti' and 'nokaslr' as kernal parameters.

For more details, please look at (https://github.com/IAIK/meltdown).