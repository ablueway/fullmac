/******************************************************
* drv_l2_usbd_hid_keyboard_descriptor_tbl.c
*
* Purpose: usb controller L2 descriptor table data, only for drv_l2_usbd.c
*
* Author: Eugene Hsu
*
* Date: 2014/09/23
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
*******************************************************/
#include <string.h>
#include "drv_l1_usbd.h"
#include "drv_l2_usbd.h"

/* Device descriptor */
ALIGN4 INT8U HID_Device_Descriptor_TBL[] =
{
     0x12,                      //bLength: 0x12 byte
 	 0x01,				        //bDescriptorType	: Device
	 0x00, 0x02,				//bcdUSB			: version 2.00
	 0x00, 						//bDeviceClass
	 0x00, 						//bDeviceSubClass
	 0x00,						//bDeviceProtocol
     0x40,  					//bMaxPacketSize0	
	 0x3F, 0x1B,				//idVendor
	 0xff, 0x26,	   			//idProduct for HID keyboard
	 0x00, 0x01,				//bcdDevice
	 0x01,						//iManufacturer
	 0x02,						//iProduct
	 0x01,						//iSerialNumber
	 0x01,						//bNumConfigurations
};

/* Qualifier descriptor */
const INT8U HID_Qualifier_Descriptor_TBL[] =
{
	0x0A,                   //bLength: 0x0A byte
	0x06,                   //bDescriptorType: DEVICE_QUALIFIER
	0x00, 0x02,             //bcdUSB: version 200 // 0x00,0x02 
	0x00,                   //bDeviceClass: 
	0x00,                   //bDeviceSubClass:
	0x00,                   //bDeviceProtocol: 
	0x40,                   //bMaxPacketSize0: maximum packet size for endpoint zero
	0x01,                   //bNumConfigurations: 1 configuration
	0x00					//bReserved
};

/* Config descriptor */
ALIGN4 INT8U HID_Config_Descriptor_TBL[] =
{
    0x09,                   		//bLength: 0x09 byte
    0x02,                   		//bDescriptorType: CONFIGURATION
    0x22, 0x00,                 	//wTotalLength:
    0x01,					   		//bNumInterfaces: 1 interfaces
    0x01,                  			//bConfigurationValue: configuration 1
    0x00,                   		//iConfiguration: index of string
    0xC0,                   		//bmAttributes: bus powered, Not Support Remote-Wakeup
    0x32,                   		//MaxPower: 100 mA 
/* Interface Descriptor */
	0x09,							//bLength: 0x09 byte
	0x04,  	  						//bDescriptorType: INTERFACE
	0x00,							//bInterfaceNumber: interface 0
	0x00,							//bAlternateSetting: alternate setting 0
	0x01,							//bNumEndpoints: 1 endpoints, EP3
	0x03,							//bInterfaceClass: HID class
	0x00,							//bInterfaceSubClass: No subclass
	0x01,							//bInterfaceProtocol: Keyboard
	0x00,							//iInterface: No string descriptor	 
/* HID descriptor */
	0x09,							//Size of descriptor
	0x21,							//HID descriptor type
	0x11,0x01,						//HID class Specification (v 1.11)
	0x00,							//Hardware traget country
	0x01,							//Number of HID descriptors to follow
	0x22,							//Report descriptor type
	0x3F,0x00,						//Total length of Report descriptor
/* Endpoint 3 (0x07 byte) HID class */
    0x07,                   		//bLength: 0x07 byte
    0x05,                   		//bDescriptorType: ENDPOINT
    0x83,                   		//bEndpointAddress: In endpoint 3
    0x03,                   		//bmAttributes: Interrupt
    0x08,0x00,             			//wMaxPacketSize: 8 byte
    0x04,                   		//bInterval: ignored
};

/* Index 0 string descriptor */
const INT8U HID_String0_Descriptor[] =
{
	 0x04,		//bLength
	 0x03,		//bDescriptorType
	 0x09, 0x04,//bString
};

/* Index 1 string descriptor */
const INT8U HID_String1_Descriptor[] =
{
     0x1A,		//bLength
	 0x03,		//bDescriptorType
	 'G', 0x00,	//bString
	 'E', 0x00,
	 'N', 0x00,
	 'E', 0x00,
	 'R', 0x00,
	 'A', 0x00,
	 'L', 0x00,
	 'P', 0x00,
	 'L', 0x00,
	 'U', 0x00,
	 'S', 0x00,
	 ' ', 0x00,
};

/* Index 2 string descriptor */
const INT8U HID_String2_Descriptor[] =
{
     0x40,      //bLength
	 0x03,		//bDescriptorType
	 'G', 0x00,	//bString
	 'e', 0x00,
	 'n', 0x00,
	 'e', 0x00,
 	 'r', 0x00,
 	 'i', 0x00, 	 
 	 'c', 0x00, 	 
  	 ' ', 0x00, 	 
   	 'U', 0x00, 	 
  	 'S', 0x00, 	    	 
  	 'B', 0x00, 	    	 
	 ' ', 0x00,
   	 'H', 0x00,
   	 'I', 0x00,
   	 'D', 0x00,
   	 ' ', 0x00,
   	 'K', 0x00,
   	 'e', 0x00,
   	 'y', 0x00,
   	 'b', 0x00,
   	 'o', 0x00,
   	 'a', 0x00,
   	 'r', 0x00,
   	 'd', 0x00,
   	 ' ', 0x00,
  	 'D', 0x00, 	 
  	 'e', 0x00, 	   	   	 	 
  	 'v', 0x00, 	   	   	 	 
  	 'i', 0x00, 	   	   	 	   	 
  	 'c', 0x00, 	   	   	 	   	 
  	 'e', 0x00, 	   	   	 	   	    	   	  	   	 	   	 
};

/* Total length 0x3F bytes */
const INT8U HID_Key_Report_Descriptor[] =	
{	
	0x05,0x01,		//Usage Page(Generic Desktop)   
	0x09,0x06,		//Usage(Keyboard)   
	0xA1,0x01,		//Collection(Application)   
	0x05,0x07,		//Usage Page(Key Codes)   
	0x19,0xE0,		//Usage Minimum(224)   
	0x29,0xE7,		//Usage Maximum(231)   
	0x15,0x00,		//Logical Minimum(0)   
	0x25,0x01,		//Logical Maximum(1)   
	0x75,0x01,		//Report Size(1)   
	0x95,0x08,     	//Report Count(8)   
	0x81,0x02,		//Input(Data, Variable, Absolute)   
	0x95,0x01,		//Report Count(1)   
	0x75,0x08,		//Report Size(8)   
	0x81,0x01,		//Input(Constant)   
	0x95,0x05,		//Report Count(5)   
	0x75,0x01,		//Report Size(1)   
	0x05,0x08,		//Usage Page(LEDs)   
	0x19,0x01,		//Usage Minimum(1)   
	0x29,0x05,		//Usage Maximum(5)   
	0x91,0x02,		//Output(Data, Variable, Absolute)   
	0x95,0x01,		//Report Count(1)   
	0x75,0x03,		//Report Size(3)   
	0x91,0x01,		//Output(Constant)   
	0x95,0x06,		//Report Count(6)   
	0x75,0x08,		//Report Size(8)   
	0x15,0x00,		//Logical Minimum(0)   
	0x25,0x65,		//Logical Maximum(101)   
	0x05,0x07,		//Usage Page(Key Codes)   
	0x19,0x00,		//Usage Minimum(0)   
	0x29,0x65,		//Usage Maximum(101)   
	0x81,0x00,		//Input(Data, Array)   
	0xC0			//End Collection 
};

const INT8U HID_Descriptor[] =
{
	0x09,							//bLength: 0x09 byte
	0x21,							//bDescriptorType: HID_DESCRIPTOR
	0x11,0x01,						//bcdHID: version 1.11
	0x00,							//bCountryCode: Not Supported  
	0x01,							//bNumDescriptor: 1 descriptor  
	0x22,							//bDescriptorType: REPORT_DESCRIPTOR
	0x3F,0x00,						//wDescriptorLen:
};