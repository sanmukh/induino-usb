#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <linux/slab.h>


#define DRIVER_AUTHOR "Sanmukh, i.sanmukh@gmail.com"
#define DRIVER_DESC "Induino Linux Driver"

#define VENDOR_ID 0x0403
#define PRODUCT_ID 0x6001

#define INDUINO_MINOR_BASE 96

/*table of devices that work with this driver.*/
struct usb_device_id id_table[]={
	{USB_DEVICE(VENDOR_ID,PRODUCT_ID)},
	{},
};


static struct usb_driver induino_driver;

MODULE_DEVICE_TABLE(usb,id_table);


struct induino_usb{
	struct usb_device* udev;
	struct usb_endpoint_descriptor *bulk_in_endpoint;
	struct usb_endpoint_descriptor *bulk_out_endpoint;
	int minor;
	int bulk_in_size;
	int bulk_out_size;
};


static void induino_bulk_complete(struct urb* urb){

	
	if(urb->status == 0){
		printk("Data successfully transferred\n");
		printk("Data transferred is\n %s length: %d",(char*)urb->transfer_buffer,urb->actual_length);
	}
	else{
		printk("Error in transmitting data. Error code: %d",urb->status);
	}
}

//The read function
static ssize_t induino_read(struct file* file,char __user *user_buf,size_t count, loff_t* ppos){
		printk("Reading of device file unsupported\n");
		return -1;
	}

//The write function
static ssize_t induino_write(struct file* file,const char __user *user_buf, size_t count, loff_t* ppos){
	ssize_t retval=-1;
	int submitret = -1;
	int subminor;
	struct usb_interface* interface;
	struct induino_usb* dev;	
	struct urb* out_urb;
	char* buf;

	subminor = MINOR(file->f_dentry->d_inode->i_rdev);

	interface = usb_find_interface(&induino_driver,subminor);

	if(!interface){
		printk("Could not find the interface\n");
		return -1;
	}	

	dev = usb_get_intfdata(interface);
	if(count > dev->bulk_out_size){
		printk("Too much input data\n");
		return retval;
	}	

	if(user_buf){
		out_urb = usb_alloc_urb(0,GFP_KERNEL);
		if(!out_urb){
			retval = -ENOMEM;
			goto error;
		}
		
		buf = kmalloc(count,GFP_KERNEL);
		if(!buf){
			retval = -ENOMEM;
			goto error;
		}
		if(copy_from_user(buf,user_buf,count)){
			retval = -EFAULT;
			goto error;
		}

		usb_fill_bulk_urb(out_urb,dev->udev,usb_sndbulkpipe(dev->udev,dev->bulk_out_endpoint->bEndpointAddress),buf,count,induino_bulk_complete,dev);
		submitret = usb_submit_urb(out_urb,GFP_KERNEL);
		if(submitret){
			printk("Error in submitting the urb: %d\n",submitret);
			retval = -1;
		}

		retval = count;	
	}
	else{
		printk("User buffer is empty\n");
		
	}
	
	return retval;
error:
	return -1;
}

//The open function.
static ssize_t induino_open(struct inode* inode, struct file* file){
	printk("Opening the device file\n");
	return 0;
}

//The close function.
static ssize_t induino_release(struct inode* inode, struct file* file){
	printk("Closing the device file\n");
	return 0;
}

//The file operations structure
static struct file_operations induino_fops={
	.owner = THIS_MODULE,
	.write = induino_write,
	.open  = induino_open,
	.read  = induino_read,
	.release = induino_release,
};


static struct usb_class_driver induino_class = {
	.name = "usbinduino%d",
	.fops = &induino_fops,
	.minor_base = INDUINO_MINOR_BASE,

};

//Called when device is inserted.
static int induino_probe(struct usb_interface *interface, const struct usb_device_id* id){
	struct usb_device *udev = interface_to_usbdev(interface);
	struct induino_usb * dev = NULL;
	int retval =-ENOMEM;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i=0;
	int in_status=0;
	int out_status=0;	

	printk("Induino board inserted\n");

	dev = kmalloc(sizeof(struct induino_usb),GFP_KERNEL);

	if(dev == NULL){
		printk("Out of memory\n");
		goto error;

	}	
		
	memset(dev,0x00,sizeof(*dev));
	dev->udev = usb_get_dev(udev);
	

	//find out the bulk endpoint and store it in our local struct
	if(interface && interface->cur_altsetting)
		iface_desc = interface->cur_altsetting;
	else{
		printk("ERROR: No interface set as current interface.\n");
		goto error;
	}

	printk("Number of Control endpoints found is %d\n",iface_desc->desc.bNumEndpoints);

	for(i=0; i< iface_desc->desc.bNumEndpoints; i++){
		endpoint = &iface_desc->endpoint[i].desc;
				
	
		if(((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN ) && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)){
			dev->bulk_in_endpoint = endpoint;
			in_status=1;
		}
		if(((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT ) && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)){
			dev->bulk_out_endpoint = endpoint;
			out_status=1;
		}
	}
	if(!in_status){
		printk("Could not find any bulk-in endpoint\n");
		goto error;
	}
	if(!out_status){
		printk("Could not find the bulk-out endpoint\n");
		goto error;
	}

	
	dev->bulk_in_size = le16_to_cpu(dev->bulk_in_endpoint->wMaxPacketSize);
	dev->bulk_out_size = le16_to_cpu(dev->bulk_out_endpoint->wMaxPacketSize);

	if(dev->bulk_in_size == 0 || dev->bulk_out_size == 0 ){
		printk("Invalid max packet size for the endpoints\n");
		goto error;
	}	
	
	//register the driver
	retval = usb_register_dev(interface,&induino_class);
	
	if(retval){
		printk("Could not register the induino class\n");	
		goto error;	
	}
	//save the minor number 	
	dev->minor = interface->minor;

	printk("Minor number is %d\n", dev->minor);
	//save the local struct
	usb_set_intfdata( interface, dev );
	
	return 0;
		

error:
	kfree(dev);
	return retval;

}


//Called when device is removed
static void induino_disconnect(struct usb_interface * interface){
	
	struct induino_usb *dev;
	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface,NULL);

	printk("Induino board removed\n");
	usb_put_dev(dev->udev);

	usb_deregister_dev(interface,&induino_class);

	kfree(dev);


}


static struct usb_driver induino_driver={
	.name = "usbinduino",
	.probe = induino_probe,
	.disconnect = induino_disconnect,
	.id_table = id_table,
};

static int __init usb_induino_init(void){
	
	int retval = 0;
	retval = usb_register(&induino_driver);
	if(retval)
		printk("usb_register failed. Error number %d",retval);
	printk("Initialized the induino module\n");
	return retval;
}

static void __exit usb_induino_exit(void){
	printk("Induino module exited.");
	usb_deregister(&induino_driver);
}

module_init(usb_induino_init);
module_exit(usb_induino_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
