#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <crypto/hash.h>
#include <linux/err.h>
#include <asm/uaccess.h>

#include "salausmoduuli_macros.h"

MODULE_AUTHOR("Henrikki Hoikka");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kayttojarjestelmakurssin harkkatyo: 72-salaus");

//mutex for protecting against multiple processes simultaniously opening device.
static DEFINE_MUTEX(encrypt_mutex);
#define SHA1_LENGTH 20

//buffer for message and an int to hold it's lenght
static char buffer[MSG_LEN] = {0};
static int buffer_used = 0;

static struct class *dev_class = NULL;
static struct device *dev = NULL;
static dev_t devt;

//buffers for crypted keys.
//these are calculated when encrypting/decrypting
//via ioctl to make sure we do not change buffer_state when decrypting didn't happen,
//or ruin our  message buffer with wrong key.
static struct key_buffers {
    //one extra space for '\0'
    char key_hash_enc [SHA1_LENGTH+1];
    char key_hash_dec [SHA1_LENGTH+1];
} key_buffers;

//three possible states that our buffer can be in.
//from EMPTY and ENCRYPTED you can get only to DECRYPTED
//from DECRYPTED you can get to EMPTY and ENCRYPTED
static enum buffer_state  {
    EMPTY = 0,
    ENCRYPTED = 1,
    DECRYPTED = 2,
} buffer_state;


//proto-types for fops
static int open (struct inode *inodep, struct file *filp);
static int release (struct inode *inodep, struct file *filp);
static ssize_t read (struct file *filp, char *msg, size_t size, loff_t *offset);
static ssize_t write (struct file *filp, const char *msg, size_t size, loff_t *offset);
static long ioctl (struct file *filp, unsigned int, unsigned long);


static struct file_operations fops =
{
    .open = open,
    .release = release,
    .read = read,
    .write = write,
    .unlocked_ioctl = ioctl,
};

static void change_state (const enum buffer_state new_state)
{
    if ((new_state < EMPTY) || (new_state > DECRYPTED))
        return;
    buffer_state = new_state;
}
// Naive approach by simply xor:in every char in message by char in key in a
// circular manner is probably enough for our 'encryption' purposes.
static int xorer( const char* key, const unsigned int len)
{
    int i,j;
    //don't xor if key is longer than message or < 1 .
    //if message was: 'message' and key was: 'thisislongkey',
    //key: 'thisisl' would also decrypt the message.
    if (len < 1 || len > buffer_used)
        return -1;

    for(i = 0, j = 0; i < buffer_used; ++i) {
        buffer[i] ^= key[j];
        //key is always <= to message in our buffer, so lets loop it in a circle.
        if(j >= len - 1) {
            j = 0;
            continue;
        }
        ++j;
    }
    return 0;
}


/*  let's provide sha1 hasher function. It is used for making sure that
    key used for decryption is same as the one used for encryption,
    before actual "xoring" happens and ruins our buffer and it's super important message!
    This also allows us to change buffer_state only when actual correct decryption happens
    It uses linux's crypto-api's synchronous hash.
*/
static int sha1_hasher(char *hash_buffer, const char *key_to_be_hashed)
{
    struct crypto_shash *tfm;
    struct shash_desc *sdesc;
    int size;
    int ret;
    size_t len;
    len = strlen(key_to_be_hashed);

    //initialize our transformation-struct, it's used for configuring the
    //hash-machine
    tfm = crypto_alloc_shash("sha1", 0, CRYPTO_TFM_REQ_MAY_SLEEP);

    if(IS_ERR(tfm))
        return PTR_ERR(tfm);

    // allocate space for transformation-struct aswell
    // we can get it's size with function provided for the purpose.
    size = sizeof(struct shash_desc) + crypto_shash_descsize(tfm);

    //actual allocation of description-struct
    sdesc = kmalloc(size, GFP_KERNEL);

    if(!sdesc)
        return -ENOMEM;

    sdesc->tfm = tfm;
    sdesc->flags = 0x0;

    if(IS_ERR(sdesc) )
        return PTR_ERR(sdesc);

    //this is a "short cut" for combo of folowing function calls:
    //crypto_shash_init() configures the machine with sdesc struct
    //crypto_shash_update() does the actual hashing
    //crypto_shash_final() copies the hash to buffer provided.
    ret = crypto_shash_digest(sdesc, key_to_be_hashed, len, hash_buffer);

    //add nul to end of our buffer.
    hash_buffer[SHA1_LENGTH] = '\0';
    //free allocated stuff
    crypto_free_shash(tfm);
    kfree(sdesc);
    return ret;
}


/*  init and exit */


//in init we register the device and create a class and device
// this way device can be seen in sysfs at /sys/class/harkka/salausmoduuli.
static int __init salaus_init(void)
{
    int major = register_chrdev(MAJOR_NUM, NAME, &fops);
    if (major < 0) {
        printk (KERN_ALERT "%s: Major number is in use\n", NAME);
        return major;
    }

    printk (KERN_INFO "%s: Device registered succesfuly with major number: %i\n",
             NAME, MAJOR_NUM);

    //register a class for our device
    dev_class = class_create(THIS_MODULE, CLASS);

    // IS_ERR macro is for checking correctness of kernel types.
    if (IS_ERR(dev_class)) {
         //release stuff back to kernel if failed
        unregister_chrdev(MAJOR_NUM, NAME);
        printk (KERN_ALERT "%s: Didn't get it's class registered\n", NAME);
        return PTR_ERR(dev_class);
    }

    printk (KERN_INFO "%s: registered with class: %s\n", NAME, CLASS);

    //Create devt and a device with class.
    devt = MKDEV(MAJOR_NUM, 0);
    dev = device_create(dev_class, NULL, devt, NULL, NAME);

    if (IS_ERR(dev)) {
      //release stuff back to kernel if failed
        class_unregister(dev_class);
        class_destroy(dev_class);
        unregister_chrdev(MAJOR_NUM, NAME);
        printk (KERN_ALERT "%s: Couldn't create a device\n", NAME);
        return PTR_ERR(dev);
     }
     //init ready!!
     //Let's but a default message in our buffer.
     strncpy(buffer, "Top secret placeholder", 255);
     buffer_used = strlen(buffer);
     //buffer is no longer empty so let's change it's state.
     change_state(DECRYPTED);

     //allocate  mutex
     mutex_init(&encrypt_mutex);
     printk (KERN_INFO "%s: has a device: %s\n", NAME, CLASS);
     return 0;
}

static void __exit salaus_exit(void)
{
    //release stuff before exiting.
    device_destroy(dev_class, devt);
    class_unregister(dev_class);
    class_destroy(dev_class);
    unregister_chrdev(MAJOR_NUM, NAME);

    //free mutex
    mutex_destroy(&encrypt_mutex);
    printk(KERN_INFO "%s: says: bye bye!.\n", NAME);
}


/*  declarations for fops  */


static int open (struct inode *inodep, struct file *filp)
{
    //make sure that this device instance is opened only once at a time.
    if ( !mutex_trylock(&encrypt_mutex) ) {

        printk(KERN_ALERT "File is already opened\n");
        return -EBUSY;
    }

    return 0;
}

static int release (struct inode *inodep, struct file *filp)
{
    //release mutex
    mutex_unlock(&encrypt_mutex);
    return 0;
}


static ssize_t read (struct file *filp, char *msg, size_t size, loff_t *offset)
{
    int count;
    //copy the message to provided buffer in userspace.
    count = copy_to_user(msg, buffer, buffer_used);
    // copy_to_user() returns the amount of failed copy attempts
    if(count != 0) {
       printk(KERN_INFO "%s: reading failed \n", NAME );
       return -EFAULT;
    }

    printk(KERN_INFO "%s: message of %i chars read \n", NAME, buffer_used );
    return buffer_used;
}

//write will overwrite previous message when succesful!!
static ssize_t write (struct file *filp, const char *msg,
                      size_t size, loff_t *offset)
{
    int count;
    //don't overwrite message if encrypted!
    if(buffer_state == ENCRYPTED) {
        printk(KERN_ALERT "%s: Someone tried to overwrite encrypted message", NAME);
        return -EFAULT;
    }



    if(size > MSG_LEN) {
        printk(KERN_INFO "%s: %i is too long, maximum lenght for message is %i \n"
               , NAME, size, MSG_LEN);
        return -EINVAL;
    }

    //no need to copy anything if message is empty
    if (size <= 0) {
        buffer[0] = '\0';
        change_state(EMPTY);
        buffer_used = 0;
        return 0;
    }

    //copy the message from provided buffer in userland
    count = copy_from_user(buffer, msg, size);

    // copy_from_user() returns the amount of failed copy attempts
    if(count != 0) {
        printk(KERN_INFO "%s: writing failed \n", NAME );
        return -EFAULT;
    }
    //change buffers length and state
    buffer_used = size;
    change_state(DECRYPTED);
    printk(KERN_INFO "%s: %i chars were writen to device \n", NAME, buffer_used );
    return buffer_used;
}


static long ioctl(struct file *filp, unsigned int ioctl_num, unsigned long key_u)
{
    char key_k[MAX_KEY_LEN];
    int  ret = 0;
    static int key_len;
    //do not encrypt an empty message. While encrypted first char might be '\0'
    //if first char and orginal message are the same.
    if (!(*buffer) && buffer_state != ENCRYPTED)
        return -EPERM;

    //switch case for ioctl. Macros for different calls are defined in salausmoduuli_macros
    switch (ioctl_num) {

    case ENCRYPT:
        if( (key_len <= 0) || (key_len > MAX_KEY_LEN) )
            return -EINVAL;
        //no re-encrypting allowed!
        if (buffer_state != DECRYPTED) {
            printk(KERN_ALERT "%s: Someone tried to encrypt allready encrypted message.\n", NAME);
            return -EPERM;
        }
        //copy key to kernel space.
        ret = copy_from_user(key_k, (char*)key_u, key_len );
        if( ret != 0)
            return -EPERM;
        // run sha1 hash on our key and save it so we can check it when decrypting
        ret = sha1_hasher(key_buffers.key_hash_enc, key_k);
        if( ret != 0)
            return -EPERM;

        //all ok, let's encrypt the buffer!!
        ret = xorer(key_k, key_len);
        if( ret < 0)
            return -EPERM;

        // it's save to change the state
        change_state(ENCRYPTED);
        key_len = 0;
        break;

    case DECRYPT:
        if( (key_len <= 0) || (key_len > MAX_KEY_LEN) )
            return -EINVAL;
        //no re-decrypting allowed!
        if (buffer_state != ENCRYPTED) {
            printk(KERN_ALERT "%s: Someone tried to decrypt allready decrypted message.\n", NAME);
            return -EPERM;
        }
        //copy key to kernel space.
        ret = copy_from_user(key_k, (char*)key_u, key_len );
        if( ret != 0)
            return -EPERM;
        //sha1 hash the guess
        ret = sha1_hasher(key_buffers.key_hash_dec, key_k);
        if( ret != 0)
            return -EPERM;
        //compare hashvalues of key used to encryption and in our decryption "candidate".
        ret = strncmp(key_buffers.key_hash_dec,
                        key_buffers.key_hash_enc, MAX_KEY_LEN );
        if( ret != 0)
            return -EPERM;
        //correct key!!
        //we can decrypt without fear of ruining our top secret message!
        ret = xorer(key_k, key_len);
        if( ret < 0)
            return -EPERM;

        // it's save to change the state
        change_state(DECRYPTED);
        key_len = 0;
        break;

    case SET_LEN:
		key_len = key_u;
	    break;
     // default case for prevention of wrong doing.
    default:
        return -ENOTTY;
    }
    return ret;
}

//macro calls for init & exit
module_init(salaus_init);
module_exit(salaus_exit);
