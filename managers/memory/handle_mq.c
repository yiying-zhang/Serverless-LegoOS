#include <lego/list.h>
#include <lego/msg_q.h>
#include <lego/stat.h>
#include <lego/slab.h>
#include <lego/uaccess.h>
#include <lego/files.h>
#include <lego/syscalls.h>
#include <lego/comp_common.h>
#include <lego/kernel.h>
#include <memory/thread_pool.h>

unsigned int append(char* msg_data, unsigned int msg_size, struct list_head* name_nid_dict){
	struct mc_msg_queue* msg;

	msg = kmalloc(sizeof(struct mc_msg_queue), GFP_KERNEL);
	if(!msg){
		printk("handle_mq append cannot allocate memory\n");
		return MSG_RET_FAIL;
	}

	msg->msg_data = kmalloc(sizeof(char)*(msg_size+1), GFP_KERNEL);
	if(!msg->msg_data){
		kfree(msg);
		printk("handle_mq append cannot allocate memory for message\n");
		return MSG_RET_FAIL;
	}

	strcpy(msg->msg_data, msg_data);
	msg->msg_size = msg_size;

	list_add_tail(&msg->list, name_nid_dict);	

	return MSG_RET_SUCCESS;
}



/* the msg data passed in should copy a new memory here, msg_data should point to a continous memory
 */
unsigned int pop(char* msg_data, int* msg_size, struct list_head* name_nid_dict){
	struct list_head* next = name_nid_dict->next;
	
	if(!next){
		printk("should not receive from an empty message queue\n");
		return MSG_RET_FAIL;
	}
	
	struct mc_msg_queue* item = list_entry(next, struct mc_msg_queue, list);

	if(!item->msg_data || !msg_data){
		printk("message entry is null\n");
		return MSG_RET_FAIL;
	}

	strcpy(msg_data, item->msg_data);
	*msg_size = item->msg_size;

	list_del(&item->list);

	kfree(item->msg_data);	
	kfree(item);
	name_nid_dict = next;

	return MSG_RET_SUCCESS;
}

void free_all(struct list_head* name_nid_dict){

	struct list_head* cur=name_nid_dict->next;
	while(cur != name_nid_dict){		
		struct mc_msg_queue* item = list_entry(cur, struct mc_msg_queue, list);

		struct list_head* tmp = cur->next;
		list_del(cur);

		cur = tmp;
		
		if(!item || !item->msg_data){
			printk("null message during freed!\n");
			continue;
		}		

		kfree(item->msg_data);
		kfree(item);
	}
}

/*
 * api for the message queue data structure in mComponent kernel side
 */ 
unsigned int mc_mq_open(char* mq_name, unsigned int max_size)
{

/*
 * what if we already got a message queue with that name in the name map?	
 */
	unsigned long flags;
	spin_lock_irqsave(&map_lock, flags);

	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){

		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}

	if(target != NULL){
		spin_unlock_irqrestore(&map_lock, flags);
		return MSG_MQ_OPEN_ERROR_ALREADY_EXIST;
	}
	
	spin_unlock_irqrestore(&map_lock, flags);

	struct name_mq_map* tmp;

	tmp = kmalloc(sizeof(struct name_mq_map), GFP_KERNEL);
	if(!tmp){
		printk("allocated wrong\n");
		return MSG_RET_FAIL;
	}

	strcpy(tmp->mq_name, mq_name);
	tmp->max_size = max_size;
	tmp->mq = kmalloc(sizeof(struct list_head), GFP_KERNEL);

	if(!tmp->mq){
		printk("message queue kmalloc failed in mc_mq_open\n");
		return MSG_RET_FAIL;
	}

	INIT_LIST_HEAD(tmp->mq);	

	/* init the lock */
	spin_lock_init(&tmp->mq_lock);
	spin_lock_irqsave(&map_lock, flags);

	list_add_tail(&tmp->list, &addr_map);	

	spin_unlock_irqrestore(&map_lock, flags);

	kfree(tmp);
	return MSG_RET_SUCCESS;
}

unsigned int mc_mq_close(char* mq_name){
	unsigned long flags;
	spin_lock_irqsave(&map_lock, flags);

	/* make sure the message queue exists*/
	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){

		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}

	if(target ==NULL){
		spin_unlock_irqrestore(&map_lock, flags);	
		return MSG_RET_FAIL;	
	}

	list_del(&target->list);

	/* possible leak
         * what if message queue still gots some thing, then we free then
         */
	free_all(target->mq);
	kfree(target->mq);
	kfree(target);

	spin_unlock_irqrestore(&map_lock, flags);

	return MSG_RET_SUCCESS;
}


unsigned int mc_mq_send(char *mq_name, char* msg_data, unsigned int msg_size){

	/* find out where is our mq head pointer */

	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){
		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}
	if(target != NULL){
		/* spin lock acquire here */
		unsigned long flags;

		spin_lock_irqsave(&target->mq_lock,flags);
		int res = append(msg_data, msg_size, target->mq);
		spin_unlock_irqrestore(&target->mq_lock,flags);
		return res;	
	}
	
	/* 
 	 * message queue not found, return failed macro
	 */
	return MSG_RET_FAIL;
}

unsigned int mc_mq_receive(char *mq_name, char* msg_data, unsigned int* msg_size){


	/* find out where is our mq head pointer */
	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){
		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}
	if(target ==NULL){	
		return MSG_RET_FAIL;
	}

	/* spin lock acquire here */
	unsigned long flags;

	spin_lock_irqsave(&target->mq_lock,flags);	
	int res = pop(msg_data, msg_size, target->mq);
	spin_unlock_irqrestore(&target->mq_lock,flags);

	return res;
}

/*
 * handling message queue api request in core.c
 */

void handle_mq_open_request(struct p2m_mqopen_payload* payload,
	struct thpool_buffer *tb)
{
	ssize_t* retval;
	retval = thpool_buffer_tx(tb);
	tb_set_tx_size(tb, sizeof(*retval));

	char* mq_name = payload->mq_name;
	int max_size = payload->msg_size;
	
	/* mq open */
	int res = mc_mq_open(mq_name, max_size);

/* sync to gmm
#ifdef CONFIG_GMM 
	ssize_t result = read_mq_nid_from_gmm(kname);
#endif
*/
	*retval = res;		
}

void handle_mq_send_request(struct p2m_mqsend_payload* payload,
	struct thpool_buffer *tb)
{
	ssize_t* retval;
	retval = thpool_buffer_tx(tb);
	tb_set_tx_size(tb, sizeof(*retval));

	/* debug */
	char* mq_name = payload->mq_name;
	char* mq_data = payload->msg;
	int mq_size = payload->msg_size;

	/* call mq send api from here! */
	int res = mc_mq_send(mq_name, mq_data, strlen(mq_data));

	/* return 0 means success!*/
	*retval = res;
}

void handle_mq_receive_request(struct p2m_mqrecv_payload* payload,
	struct thpool_buffer *tb)
{
 	struct p2m_mqrecv_reply_struct* retval;
	retval = thpool_buffer_tx(tb);
	
	char* msg = kmalloc(sizeof(char)*(MAX_FILENAME_LENGTH+1), GFP_KERNEL);	
	if(!msg){
		retval->ret = MSG_RET_FAIL;
	}

	int size;	
	tb_set_tx_size(tb, sizeof(*retval));
	
	if(mc_mq_receive(payload->mq_name,msg,&size == MSG_RET_SUCCESS)){
		strcpy(retval->mq_data, msg);
		retval->ret = MSG_RET_SUCCESS;
		kfree(msg);
	}
	else{
		retval->ret = MSG_RET_FAIL;
		kfree(msg);
	}

}

void handle_mq_close_request(struct p2m_mqclose_payload* payload,
	struct thpool_buffer *tb)
{
	
	ssize_t* retval;
	retval = thpool_buffer_tx(tb);

	tb_set_tx_size(tb, sizeof(*retval));

	*retval = mc_mq_close(payload->mq_name);

}
