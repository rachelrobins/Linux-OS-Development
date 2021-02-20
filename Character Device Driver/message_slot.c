
// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/slab.h>


MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"


typedef struct chanelNode {
    unsigned long chanel_id;
    char msg[BUF_LEN];
    int len;
    struct chanelNode * nextChanel;
}chanel;


typedef struct listOfChanels {
    chanel  * topChanel;

}chanelList;

char oldmsg[BUF_LEN];
chanelList * minors[256];



static int addChanel(unsigned long chanelId, chanelList * lst ) {
    chanel *  chan;
    if(lst==NULL){
        return -1;
    }
    chan =NULL;
    chan=(chanel *)kmalloc(sizeof(chanel),GFP_KERNEL);
    chan->chanel_id=chanelId;
    chan->len =0;
    chan->nextChanel=lst->topChanel;
    lst->topChanel=chan;

    return 0;
}
static chanel * getChanel(unsigned long chanelId, chanelList * lst ) {
    chanel * tmp;
    if(lst==NULL){
        return NULL;
    }
    tmp=lst->topChanel;
    while (tmp != NULL)
    {
        if(tmp->chanel_id==chanelId){
            return tmp;
        }
        tmp=tmp->nextChanel;
    }
    return NULL;
}
static int free_chanels(chanelList * lst)
{
    chanel *tmp;
    if(lst==NULL){
        return -1;
    }
    printk( "IN FREE CHANELS !!!!!!!");

    while (lst->topChanel != NULL)
    {
        printk( "IN THE begining of LOOP !!!!!!!");
        tmp = lst->topChanel;
        printk( "IN THE middle of LOOP !!!!!!!");
        lst->topChanel = lst->topChanel->nextChanel;
        printk( "BEFORE FREE !!!!!!!");
        kfree(tmp);
        printk( "AFTER FREE !!!!!!!");

    }
    return 0;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{

    int minor;
    minor =iminor(inode);
    if(minors[minor]==NULL){
        printk("Invoking device_open(%p)\n", file);

        minors[minor] = (chanelList*)kmalloc(sizeof(chanelList), GFP_KERNEL);
        minors[minor]->topChanel=NULL;
        if (minors[minor] == NULL)
        {
            return -EINVAL;
        }


    }


    return SUCCESS;
}
static int device_release( struct inode* inode,
                           struct file*  file)
{
    printk("Invoking device_release(%p,%p)\n", inode, file);
    return SUCCESS;
}

//---------------------------------------------------------------


//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file *file,
                           char __user *buffer,
size_t length,
        loff_t *offset)
{
int minor;
chanel * chan;
int i;
printk( "reading");
if(file->private_data==NULL){
printk( "no private data ");
return -EINVAL;
}
minor = iminor(file_inode(file));
chan=getChanel( (unsigned long)file->private_data, minors[minor]);
if(chan==NULL){
printk( "channel is null ");
return -EINVAL;
}
if(chan->len==0){
printk( "chanel len is 0 ");
return -EWOULDBLOCK;
}
if(length<chan->len){
printk( "length is smaller than chan len ");
return -ENOSPC;
}

for (i = 0; i < length && i < chan->len; ++i)
{
if (put_user(chan->msg[i], &buffer[i]))
{
printk( "put user failed ");
return -EFAULT;
}
}

return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
size_t             length,
        loff_t*            offset)
{
chanel * chan;
int minor;
int i,j;
printk( "writingg ");
if(file->private_data==NULL){
printk( "NO PRIVATE DATA.!! ");
return -EINVAL;
}
minor = iminor(file_inode(file));
if (BUF_LEN<length || length==0){
return -EMSGSIZE;
}
chan=getChanel( (unsigned long)file->private_data, minors[minor]);
if(chan==NULL){
printk( "NOCHANNEL!! ");
return -EINVAL;
}
for (j=0;j<BUF_LEN;j++){
oldmsg[j]=chan->msg[j];
}
for (i = 0; i < length ; ++i)
{
if (get_user(chan->msg[i], &buffer[i]))
{
for (j=0;j<BUF_LEN;j++){
chan->msg[j]=oldmsg[j];
}
printk( "GET USER FAILED !");
return -EFAULT;
}

}
if(length!=128){
chan->msg[length]='\0';
}
chan->len=length;
return length;

}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    int minor;
    int i;
    // Switch according to the ioctl called

    // Get the parameter given to ioctl by the process
    if(ioctl_command_id!=MSG_SLOT_CHANNEL || ioctl_param ==0){
        return -EINVAL;
    }
    file->private_data = (void *)ioctl_param;
    minor = iminor(file_inode(file));

    if(getChanel(ioctl_param,minors[minor])==NULL){

        i=addChanel(ioctl_param,minors[minor]);
        if(i==-1){
            return -EINVAL;
        }
    }
    else{  printk( "Chanel EXISTS");  }





    return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .owner          = THIS_MODULE,
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,


        };

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init  simple_init(void)
{
    int i;
    int rc = -1;
    // init dev struct
    printk( "start. ");

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );
    printk( "registered. ");
    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ERR  "registraion failed for  %d\n",
                MAJOR_NUM );
        return rc;
    }
    for(i=0;i<256;i++){
        minors[i]=NULL;
    }

    printk( "Registeration is successful. ");
    printk( "If you want to talk to the device driver,\n" );
    printk( "you have to create a device file:\n" );
    printk( "You can echo/cat to/from the device file.\n" );
    printk( "Dont forget to rm the device file and "
            "rmmod when you're done\n" );

    return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    int i;
    for(i=0;i<256;i++){
        if (minors[i]!=NULL) {
            printk(  "IN MINOR   %d\n",
                     i );
            free_chanels(minors[i]);
            kfree(minors[i]);
        }
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================






