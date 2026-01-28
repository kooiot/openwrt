/******************************************************************************
	项目名称	：  
	文件		：  esamlib.h
	描述		：  ESAM驱动
	版本		：  0.1
	作者		：  
	创建日期	：  
******************************************************************************/

#ifndef _ESAMLIB_H
#define _ESAMLIB_H

struct encryption_reg_req;
//define ioctl command 
#define ESAM_IOC_MAGIC 0xE4
 
#define RESET_ESAM	_IO(ESAM_IOC_MAGIC,  0)		//设置ESAM复位

#define ESAM_IOC_ADD 'E'
#define ESAM_GET_DATA _IOW(ESAM_IOC_ADD, 0x01, struct encryption_reg_req) 
#define ESAM_SET_DATA _IOW(ESAM_IOC_ADD, 0x02, struct encryption_reg_req)

struct encryption_reg_req {
	int buflen; 
	char *bufdata; 
};

#endif  /* _ESAMLIB_H */
