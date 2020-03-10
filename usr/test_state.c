#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include <string.h>
#include <pthread.h>

#define STATUS(retval) (retval == 0 ? "Success" : "Failed")

static void lego_test_state_save(const char * name, const char * state, size_t th_id)
{
    long retval = syscall(667, name, strlen(name)+1, strlen(state)+1, state);
	printf ("(%d) [SAVE] inputs {name: %s, state %s}\nreturns {%s}", th_id, name, state, STATUS(retval));
}

#define BUFFER_SIZE 256
static void lego_test_state_load(const char * name, size_t th_id)
{
    char buf[BUFFER_SIZE] = {0,};
    long retval = syscall(668, name, strlen(name)+1, BUFFER_SIZE, buf);
	printf ("(%d) [LOAD] inputs {name: %s}\nreturns {%s, state: %s}", th_id, name, STATUS(retval), buf);
}

static void lego_test_state_delete(const char * name, size_t th_id)
{
	long retval = syscall(669, name, strlen(name)+1);
	printf ("(%d) [DELETE] inputs {name: %s}\nreturns {%s}", th_id, name, STATUS(retval));
}

static void lego_test_state_check(const char * name, size_t th_id)
{
	long retval = syscall(670, name, strlen(name)+1);
	printf ("(%d) [CHECK] inputs {name: %s}\nreturns {%s}", th_id, name, STATUS(retval));
}

static void *state_user_thread(size_t id)
{
	char * name = "Bob";
	char char_id[2] = ".";
	char_id[0] = id + '0';
	char state[BUFFER_SIZE] = "Bob is handled by ";
	strcat(state, char_id);
	lego_test_state_save(name, &state, id);
	lego_test_state_load(name, id);

}


#define TH_NUM 10
int main(void)
{
	int ret;
	pthread_t th[TH_NUM];
	unsigned int args[TH_NUM];

	size_t i;

	printf("Creating %d threads\n", TH_NUM);
	for(i=0;i<TH_NUM;i++) {
		ret = pthread_create( &th[i], NULL , state_user_thread , i);
		if (ret) {
			printf("pthread_create failed at id %ld\n", i);
			exit(-1);
		}
	}

	printf("Waiting %d threads to finish\n", TH_NUM);

	for(i=0;i<TH_NUM;i++) {
		pthread_join(th[i], NULL);
	}

	printf("Test exits successfully\n");

}

