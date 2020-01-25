

#include <lego/list.h>
#include <lego/spinlock.h>

#ifndef MSG_H
#define MSG_H

#define MAX_FILENAME_LENGTH 100

/*
 * return success value for the message queue system call
 */ 
#define MSG_RET_SUCCESS 0
#define MSG_RET_FAIL 1
/* 
 * message queue data structure
 */

struct mc_msg_queue{
	char* msg_data;
	unsigned int msg_size;
	struct list_head list;
	/* this array is the pid table 
 	struct list_head pid_list; */
};

static DEFINE_SPINLOCK(map_lock);

struct name_mq_map{
	char mq_name[MAX_FILENAME_LENGTH];
	unsigned int max_size;
	struct list_head* mq;
	struct list_head list;
	spinlock_t mq_lock;
};

unsigned int append(char* msg_data, unsigned int msg_size, struct list_head* name_nid_dict);

/* the msg data passed in should copy a new memory here, msg_data should point to a continous memory
 *  * */
unsigned int pop(char* msg_data, int* msg_size, struct list_head* name_nid_dict);
/*
 *  *  * print name_nid_dict
 *   *   */


void print(struct list_head* name_nid_dict);
void free_all(struct list_head* name_nid_dict);
static LIST_HEAD(addr_map);

unsigned int mc_mq_open(char* mq_name, unsigned int max_size);
unsigned int mc_mq_send(char *mq_name, char* msg_data, unsigned int msg_size);
unsigned int mc_mq_receive(char *mq_name, char* msg_data, unsigned int* msg_size);
unsigned int mc_mq_close(char* mq_name);
unsigned int mc_mq_free(void);
#endif
