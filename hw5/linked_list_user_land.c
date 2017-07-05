#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>


#define NUM_OF_INT_BUF 4
#define LEN_OF_INT_BUF 128

/////////////////////LINKED LIST////////////////////////
typedef struct message_slot{
    char buffers[NUM_OF_INT_BUF][LEN_OF_INT_BUF];
    int slot_index;
}message_slot_t;

typedef struct node {
    struct message_slot data;
    struct node * next;
}node_t;


void push(node_t ** head, message_slot_t data) {
    // node_t * curr = head;
    // while (curr != NULL) {
    //     curr = curr->next;
    // }

    // /* now we can add a new variable */
    // curr = malloc(sizeof(node_t));
    // if (NULL == curr)
    // {
    //     //TODO error
    //     printf("kmalloc failed in line : %d",__LINE__);
    // }
    // curr->data = data;
    // curr->next = NULL;

    node_t * curr = malloc(sizeof(node_t));
    if (NULL == curr)
    {
        //TODO error
        printf("kmalloc failed in line : %d",__LINE__);
    }
    curr->data = data;
    curr->next = *head;
    printf("%d\n",__LINE__ );
    *head = curr;
    if (*head == NULL)
    {
        printf("%d\n",__LINE__ );
    }
    printf("%d\n",__LINE__ );
}

void print_list(node_t * head) {
    node_t * curr = head;
    if (curr == NULL)
    {
        printf("the list is EMPTY\n");
        return;
    }
    while (curr != NULL) {
        int i=0;
        printf("the slot_index is : %d\n",curr->data.slot_index );
        for (i = 0; i < NUM_OF_INT_BUF; ++i)
        {
            printf("message slot num : %d the message : \"%s\"\n",i, (curr->data).buffers[i]);
        }
        curr = curr->next;
    }
}

int pop(node_t ** head) {
    int retval = -1;
    node_t * next_node = NULL;

    if (*head == NULL) {
        return -1;
    }

    next_node = (*head)->next;
    retval = 0;
    free(*head);
    *head = next_node;
    //TODO remove return
    return retval;
}

int remove_by_index(node_t ** head, int n) {
    int i = 0;
    int retval = -1;
    node_t * curr = *head;
    node_t * temp_node = NULL;
    
    printf("%d\n",__LINE__ );
    if (curr == NULL)
    {
        printf("%d\n",__LINE__ );
        return retval;
    }
    if (curr->data.slot_index == n) {
        return pop(head);
    }
    printf("%d\n",__LINE__ );

    for (; NULL != curr->next; curr = curr->next) {
        if (curr->next->data.slot_index == n) {
            temp_node = curr->next;
            retval = 0;
            curr->next = temp_node->next;
            free(temp_node);
            break;
        }
    }
    return retval;
}

node_t* get_node_by_slot_index(node_t * head, int index){
    node_t * curr = head;
    for (; NULL != curr; curr = curr->next) {
        if (curr->data.slot_index == index)
        {
            return curr;
        }
    }
    return NULL;

}


int destroy_list(node_t ** head){
    while (pop(head) != -1);
    //return 1;
}


int main(int argc, char const *argv[])
{

    node_t * test_list = NULL;// = malloc(sizeof(node_t));
    //printf ("the rc is: %d \n",destroy_list(&test_list));
    // printf("%d\n",__LINE__ );
    // test_list->data.slot_index = 1;
    // test_list->next = malloc(sizeof(node_t));
    // test_list->next->data.slot_index = 2;
    // test_list->next->next = malloc(sizeof(node_t));
    // test_list->next->next->data.slot_index = 3;
    // test_list->next->next->next = malloc(sizeof(node_t));
    // test_list->next->next->next->data.slot_index = 4;
    // test_list->next->next->next->next = NULL;

    message_slot_t data;
    int i;
    for (i = 0; i < 3; ++i)
    {
        data.slot_index = 80*i;
        // data.buffers = {{"dssfdds"}, {"dsdsa"}, {"gfg"}, {"fgdsgfds"}};
        push(&test_list,data);
    }
    if (test_list == NULL)
    {
        printf("%d\n",__LINE__ );
    }
    print_list(test_list);
    printf("%d\n",__LINE__ );
    remove_by_index(&test_list, 720);
    printf("%d\n",__LINE__ );
    remove_by_index(&test_list, 800);
    printf("%d\n",__LINE__ );
    remove_by_index(&test_list, 3);
    remove_by_index(&test_list, 880);
    printf("%d\n",__LINE__ );
    node_t * mynode = get_node_by_slot_index(test_list,80);
    if (NULL != mynode)
    {
        /* code */
        printf("the num : %d\n",mynode->data.slot_index);
    }

    print_list(test_list);
    printf("%d\n",__LINE__ );
    return 0;
}