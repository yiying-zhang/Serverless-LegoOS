#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include <string.h>

static void lego_uname(void)
{
	struct utsname foo;

	uname(&foo);
	printf("uname(): \n"
	       "\t sysname: %s\n"
	       "\t nodename: %s\n"
	       "\t release: %s\n"
	       "\t version: %s\n"
	       "\t machine: %s\n",
	       foo.sysname, foo.nodename, foo.release, foo.version, foo.machine);
}

static void lego_getrlimit(void)
{
	struct rlimit l;

	getrlimit(RLIMIT_STACK, &l);
	printf("getrlimit(): RLIMIT_STACK, cur: %lld, max: %lld\n",
		l.rlim_cur, l.rlim_max);
}

static void lego_time(void)
{
	struct timeval tv;
	time_t t;

	gettimeofday(&tv, NULL);
	printf("gettimeofday(): tv_sec: %lld, tv_usec: %lld\n",
		tv.tv_sec, tv.tv_usec);

	t = time(NULL);	
	printf("time(NULL): %lld\n", t);
}

static void lego_set_tid_address(void)
{
	pid_t tgid;
	int dummy;

	tgid = syscall(218, &dummy);
	printf("set_tid_address(): return tgid: %u\n", tgid);
}

static void lego_test_state_save(const char * name, const char * state)
{
    printf("state_save inputs {name: %s, state %s}\n", name, state);
    long retval = syscall(667, name, strlen(name)+1, strlen(state)+1, state);
    printf ("state_save returns: %ld\n", retval);
}

#define BUFFER_SIZE 1024
static void lego_test_state_load(const char * name)
{
	printf("state_load inputs {name: %s}\n", name);
    char buf[BUFFER_SIZE] = {0,};
    long retval = syscall(668, name, strlen(name)+1, BUFFER_SIZE, buf);
    printf ("state_load returns: %ld, {state: %s}\n", retval, buf);
}

static void lego_test_state_delete(const char * name)
{
	printf("state_delete inputs {name: %s}\n", name);
	long retval = syscall(669, name, strlen(name)+1);
	char * status = retval > 0 ? "Failure" : "Success";
	printf ("state_delete returns: %s\n", status);
}

static void lego_test_state_check(const char * name)
{
	printf("state_check inputs {name: %s}\n", name);
	long retval = syscall(670, name, strlen(name)+1);
	char * status = retval > 0 ? "Failure" : "Success";
	printf ("state_check returns: %s\n", status);
}

static void lego_test_state_sequential(void){
	printf("--- state sequential test ---");
	char * name1 = "Bob's function233";
    char * state1 = "Bob went to 0xFB1DBA5 and ordered a cup of d2330241.";
    char * state1_m = "Bob left 0xFB1DBA5.";
	char * name2 = "Bob's function666";
	char * state2 = "Bob says 666";

	lego_test_state_save(name1, state1);
	lego_test_state_save(name1, state1_m);
	lego_test_state_load(name1);
	lego_test_state_check(name2);
	lego_test_state_save(name2, state2);
	lego_test_state_delete(name1);
	lego_test_state_load(name2);
	lego_test_state_load(name1);
	lego_test_state_check(name1);
	lego_test_state_delete(name1);

}

int main(void)
{
	printf("pid: %d\n", getpid());
//	lego_time();

//	lego_uname();
//	lego_getrlimit();
//	lego_set_tid_address();
//
//	lego_time();

//    lego_test_state_save();
//    lego_test_state_load();
	lego_test_state_sequential();
}

