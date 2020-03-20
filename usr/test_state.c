#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <linux/unistd.h>
#include "includeme.h"


#define STATE_DEBUG_ON 1

#define STATUS(retval) (retval == 0 ? "Success" : "Failed")


static void lego_test_state_save(const char * name, const char * state, size_t th_id)
{
    long retval = syscall(667, name, strlen(name)+1, strlen(state)+1, state);
	if (STATE_DEBUG_ON) printf ("(%d) [SAVE] inputs {name: %s, state %s}\nreturns {%s}", th_id, name, state, STATUS(retval));
}

#define BUFFER_SIZE 256
static void lego_test_state_load(const char * name, size_t th_id)
{
    char buf[BUFFER_SIZE] = {0,};
    long retval = syscall(668, name, strlen(name)+1, BUFFER_SIZE, buf);
	if (STATE_DEBUG_ON) printf ("(%d) [LOAD] inputs {name: %s}\nreturns {%s, state: %s}", th_id, name, STATUS(retval), buf);
}

static void lego_test_state_delete(const char * name, size_t th_id)
{
	long retval = syscall(669, name, strlen(name)+1);
	if (STATE_DEBUG_ON) printf ("(%d) [DELETE] inputs {name: %s}\nreturns {%s}", th_id, name, STATUS(retval));
}

static void lego_test_state_check(const char * name, size_t th_id)
{
	long retval = syscall(670, name, strlen(name)+1);
	if (STATE_DEBUG_ON) printf ("(%d) [CHECK] inputs {name: %s}\nreturns {%s}", th_id, name, STATUS(retval));
}

static int rand_by_range(int lower, int upper)
{
	int i;
	return (rand() % (upper - lower + 1)) + lower;
}

static void gen_rand_alphanum(char *s, const int len) {
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;

}

#define MAX_ITER_PER_TH 10000
#define NAME_SIZE 25
#define STATE_SIZE 1024

static void *state_user_thread(size_t id)
{

	struct timeval ts, te, result;
	char name[NAME_SIZE];
	char state[STATE_SIZE];
	long retval;

	int func_id;
	char buf[STATE_SIZE] = {0,};
	double timetaken = 0;

	int i;
	for (i = 0; i < MAX_ITER_PER_TH; i++){
		func_id = rand_by_range(0, 3);
		gen_rand_alphanum(name, NAME_SIZE-1);
		gen_rand_alphanum(state, STATE_SIZE-1);

		gettimeofday(&ts, NULL);

		if (func_id == 0){
			retval = syscall(667, name, NAME_SIZE, STATE_SIZE, state);
		} else if (func_id == 1){
			retval = syscall(668, name, NAME_SIZE, STATE_SIZE, buf);
		} else if (func_id == 2){
			retval = syscall(669, name, NAME_SIZE);
		} else {
			retval = syscall(670, name, NAME_SIZE);
		}

		gettimeofday(&te, NULL);
		timeval_sub(&result, &te, &ts);

		/* Get elapsed time in ms */
		timetaken = timetaken + (double)result.tv_sec*(double)1000 + (double)result.tv_usec/(double)1000;
	}

	// End timing
	printf("Thread [%d] time elapsed %lf ms\n", id, timetaken);

	//	char * name = "Bob";
	//	char char_id[2] = ".";
	//	char_id[0] = id + '0';
	//	char state[BUFFER_SIZE] = "Bob is handled by ";
	//	strcat(state, char_id);
	//	sleep(rand_by_range(1, 5));
	//	lego_test_state_save(name, &state, id);
	//	sleep(rand_by_range(1, 3));
	//	lego_test_state_load(name, id);

}

#define TH_NUM 4
int main(void)
{
	if (STATE_DEBUG_ON) printf("Test starts\n");
	int ret;
	pthread_t th[TH_NUM];
	unsigned int args[TH_NUM];

	setbuf(stdout, NULL);

	size_t i;

	if (STATE_DEBUG_ON) printf("Creating %d threads\n", TH_NUM);
	for(i=0;i<TH_NUM;i++) {
		ret = pthread_create( &th[i], NULL , state_user_thread , i);
		if (ret) {
			if (STATE_DEBUG_ON) printf("pthread_create failed at id %ld\n", i);
		}
	}

	if (STATE_DEBUG_ON) printf("Waiting %d threads to finish\n", TH_NUM);

	for(i=0;i<TH_NUM;i++) {
		pthread_join(th[i], NULL);
	}

	if (STATE_DEBUG_ON) printf("Test exits successfully\n");

}
