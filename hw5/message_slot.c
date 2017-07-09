/* Declare what kind of code we want from the header files
   Defining __KERNEL__ and MODULE allows us to access kernel-level 
   code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */

/* ***** Example w/ minimal error handling - for ease of reading ***** */

//Our custom definitions of IOCTL operations
#include "message_slot.h"

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>


MODULE_LICENSE("GPL");

#define NUM_OF_INT_BUF 4
#define LEN_OF_INT_BUF 128
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

struct chardev_info{
    spinlock_t lock;
};

typedef struct message_slot{
    char buffers[NUM_OF_INT_BUF][LEN_OF_INT_BUF];
    int ch_num;
    int slot_index;
}message_slot_t;

/////////////////////LINKED LIST////////////////////////

typedef struct node {
    struct message_slot data;
    struct node * next;
}node_t;


void push(node_t ** head, message_slot_t data) {
    node_t * curr;

    curr = kmalloc(sizeof(node_t),GFP_KERNEL);
    if (NULL == curr)
    {
        //TODO error
        printk("kmalloc failed in line : %d",__LINE__);
    }
    curr->data = data;
    curr->next = *head;
    *head = curr;
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

void print_list(node_t * head) {
    node_t * curr = head;
    if (curr == NULL)
    {
        printk("the list is EMPTY\n");
        return;
    }
    while (curr != NULL) {
        int i=0;
        printk("the slot_index is : %d\n",curr->data.slot_index );
        for (i = 0; i < NUM_OF_INT_BUF; ++i)
        {
            printk("message slot num: %d the message: \"%s\"\n",i, (curr->data).buffers[i]);
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
    retval = (*head)->data.slot_index;
    kfree(*head);
    *head = next_node;
    //TODO remove return
    return retval;
}

void destroy_list(node_t ** head){
    while (pop(head) != -1);
}

int remove_by_index(node_t ** head, int n) {
    int retval = -1;
    node_t * curr = *head;
    node_t * temp_node = NULL;
    
    if (curr == NULL)
    {
        return retval;
    }

    if (curr->data.slot_index == n) {
        return pop(head);
    }

    for (; NULL != curr->next; curr = curr->next) {
        if (curr->next->data.slot_index == n) {
            temp_node = curr->next;
            retval = 0;
            curr->next = temp_node->next;
            kfree(temp_node);
            break;
        }
    }
    return retval;
}
/////////////////////LINKED LIST////////////////////////

static node_t* head = NULL;

static int dev_open_flag = 0; /* used to prevent concurent access into the same device */
static struct chardev_info device_info;
static unsigned long flags; // for spinlock
//static char Message[BUF_LEN]; /* The message the device will give when asked */
//static int encryption_flag = 0; /*Do we need to encrypt?*/

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    printk("device_open (%p)\n", file);

    /* 
     * We don't want to talk to two processes at the same time 
     */
    spin_lock_irqsave(&device_info.lock, flags);
    if (dev_open_flag){
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }

    if (NULL == get_node_by_slot_index(head,file->f_inode->i_ino))
    {
        message_slot_t msg;
        int i;
        for (i = 0; i < NUM_OF_INT_BUF; ++i)
        {
            msg.buffers[i][0] = '\0';
        }
        msg.ch_num = -1;
        msg.slot_index = file->f_inode->i_ino;

        push(&head,msg);
        printk("device_opened (%p)\n", file);
    }
    else
    {
        printk("the device is already open(%p)\n", file);
    }
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;

}

static int device_release(struct inode *inode, struct file *file)
{

    printk("device_release(%p,%p)\n", inode, file);

    /* ready for our next caller */
    // spin_lock_irqsave(&device_info.lock, flags);
    

    // if(-1 == remove_by_index(&head, file->f_inode->i_ino)){
    //     printk("device already release \n");
    //     spin_unlock_irqrestore(&device_info.lock, flags);
    //     return -1;
    // }
    // spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

/* a process which has already opened 
   the device file attempts to read from it */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
               char __user * buffer, /* buffer to be filled with data */
               size_t length,  /* length of the buffer     */
               loff_t * offset)
{
    /* read doesnt really do anything (for now) */
    int i = -1;
    node_t* curr_node;
    if (NULL == buffer)
    {
        return -EINVAL;
    }
    spin_lock_irqsave(&device_info.lock, flags);
    
    curr_node = get_node_by_slot_index(head,file->f_inode->i_ino);
    printk("device_read(%p,%d)\n", file, length);
    if (NULL == curr_node)
    {
        //TODO errno
        printk("the file not exist in the list");
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -1;
    }
    if (-1 == curr_node->data.ch_num)
    {
        printk("the message channel index isn't init\n");
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -1;
    }


    for (i = 0; i < MIN(LEN_OF_INT_BUF,length); i++)
    {
        put_user(curr_node->data.buffers[curr_node->data.ch_num][i], buffer + i);
    }
    spin_unlock_irqrestore(&device_info.lock, flags);

    /* return the number of input characters used */
    return i;
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file,
            const char __user * buffer, size_t length, loff_t * offset)
{
    int i = -1;
    node_t* curr_node;
    if (NULL == buffer)
    {
        return -EINVAL;
    }
    spin_lock_irqsave(&device_info.lock, flags);
    curr_node = get_node_by_slot_index(head,file->f_inode->i_ino);
    printk("device_write(%p,%d)\n", file, length);
    
    if (NULL == curr_node)
    {
        //TODO
        printk("the file not exist in the list\n");
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -1;
    }
    if (-1 == curr_node->data.ch_num)
    {
        printk("the message channel index isn't init\n");
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -1;
    }

    for (i = 0; i < LEN_OF_INT_BUF; i++)
    {
        if (i < length)
        {
            get_user(curr_node->data.buffers[curr_node->data.ch_num][i], buffer + i);
        }
        else
        {
            curr_node->data.buffers[curr_node->data.ch_num][i] = 0;   
        }
    }
    spin_unlock_irqrestore(&device_info.lock, flags);
    /* return the number of input characters used */
    return i;
}

//----------------------------------------------------------------------------
static long device_ioctl( //struct inode*  inode,
                  struct file*   file,
                  unsigned int   ioctl_num,/* The number of the ioctl */
                  unsigned long  ioctl_param) /* The parameter to it */
{
    node_t* curr_node;
    /* Switch according to the ioctl called */
    if(IOCTL_SET_CH == ioctl_num && ioctl_param < 4) 
    {
        spin_lock_irqsave(&device_info.lock, flags);
        curr_node = get_node_by_slot_index(head,file->f_inode->i_ino);
        if (NULL != curr_node)
        {
            curr_node->data.ch_num = ioctl_param;
        	printk("message_slot, ioctl: setting ch_num to : %ld\n", ioctl_param);
            spin_unlock_irqrestore(&device_info.lock, flags);
            return SUCCESS;
        }
        else
        {
            //TODO
            printk("the file not exist in the list");
            spin_unlock_irqrestore(&device_info.lock, flags);
            return -1;
        }
    }

    ///////////
    if (IOCTL_SET_CH == ioctl_num)
    {
        printk("message_slot, %s: something bad occuored in line : %d\n",__FUNCTION__,__LINE__);
    }
    if (ioctl_param < 4)
    {
        printk("message_slot, %s: something bad occuored in line : %d\n",__FUNCTION__,__LINE__);
    }
    ///////////
    printk("message_slot, %s: something bad occuored in line : %d\n",__FUNCTION__,__LINE__);
    return -EINVAL;
}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */

struct file_operations Fops = {
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl= device_ioctl,
    .open = device_open,
    .release = device_release,  
};

/* Called when module is loaded. 
 * Initialize the module - Register the character device */
static int __init simple_init(void)
{
	unsigned int rc = 0;
    /* init dev struct*/
    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);    

    /* Register a character device. Get newly assigned major num */
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

    /* 
     * Negative values signify an error 
     */
    if (rc < 0) {
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", MAJOR_NUM);
        return -1;
    }

    printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    printk("You can echo/cat to/from the device file.\n");
    printk("Dont forget to rm the device file and rmmod when you're done\n");

    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit simple_cleanup(void)
{
    /* 
     * Unregister the device 
     * should always succeed (didnt used to in older kernel versions)
     */

    spin_lock_irqsave(&device_info.lock, flags);
    destroy_list(&head);
    spin_unlock_irqrestore(&device_info.lock, flags);
    
    //TODO spin_lock_init remove
    
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

    printk("UnRegisteration is a success. The major device number was %d.\n", MAJOR_NUM);
}

module_init(simple_init);
module_exit(simple_cleanup);
