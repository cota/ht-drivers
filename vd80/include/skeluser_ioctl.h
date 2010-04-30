/* =========================================================== 	*/
/* Define the IOCTL calls you will implement         		*/
#ifndef _SKEL_USER_IOCTL_H_
#define _SKEL_USER_IOCTL_H_

/*
 * Note. You can change any macro _name_ in this file *except* the following:
 * - SKELUSER_IOCTL_MAGIC
 * - SkelDrvrSPECIFIC_IOCTL_CALLS
 * - SkelUserIoctlFIRST
 * - SkelUserIoctlLAST
 */

/* =========================================================== */
/* Your Ioctl calls are defined here                           */

#define SKELUSER_IOCTL_MAGIC 'V'

#define SKELU_IO(nr)		  _IO(SKELUSER_IOCTL_MAGIC, nr)
#define SKELU_IOR(nr,sz)	 _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define SKELU_IOW(nr,sz)	 _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define SKELU_IOWR(nr,sz)	_IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)

#define SkelDrvrSPECIFIC_IOCTL_CALLS Vd80DrvrSPECIFIC_IOCTL_CALLS

/* ============================= */
/* Define Standard IOCTL numbers */

#define Vd80IoctlFIRST                                  SKELU_IO(Vd80NumFIRST)
#define Vd80IoctlSET_CLOCK                              SKELU_IOW(Vd80NumSET_CLOCK,uint32_t)
#define Vd80IoctlSET_CLOCK_DIVIDE_MODE                  SKELU_IOW(Vd80NumSET_CLOCK_DIVIDE_MODE,uint32_t)
#define Vd80IoctlSET_CLOCK_DIVISOR                      SKELU_IOW(Vd80NumSET_CLOCK_DIVISOR,uint32_t)
#define Vd80IoctlSET_CLOCK_EDGE                         SKELU_IOW(Vd80NumSET_CLOCK_EDGE,uint32_t)
#define Vd80IoctlSET_CLOCK_TERMINATION                  SKELU_IOW(Vd80NumSET_CLOCK_TERMINATION,uint32_t)
#define Vd80IoctlGET_CLOCK                              SKELU_IOR(Vd80NumGET_CLOCK,uint32_t)
#define Vd80IoctlGET_CLOCK_DIVIDE_MODE                  SKELU_IOR(Vd80NumGET_CLOCK_DIVIDE_MODE,uint32_t)
#define Vd80IoctlGET_CLOCK_DIVISOR                      SKELU_IOR(Vd80NumGET_CLOCK_DIVISOR,uint32_t)
#define Vd80IoctlGET_CLOCK_EDGE                         SKELU_IOR(Vd80NumGET_CLOCK_EDGE,uint32_t)
#define Vd80IoctlGET_CLOCK_TERMINATION                  SKELU_IOR(Vd80NumGET_CLOCK_TERMINATION,uint32_t)
#define Vd80IoctlSET_TRIGGER                            SKELU_IOW(Vd80NumSET_TRIGGER,uint32_t)
#define Vd80IoctlSET_TRIGGER_EDGE                       SKELU_IOW(Vd80NumSET_TRIGGER_EDGE,uint32_t)
#define Vd80IoctlSET_TRIGGER_TERMINATION                SKELU_IOW(Vd80NumSET_TRIGGER_TERMINATION,uint32_t)
#define Vd80IoctlGET_TRIGGER                            SKELU_IOR(Vd80NumGET_TRIGGER,uint32_t)
#define Vd80IoctlGET_TRIGGER_EDGE                       SKELU_IOR(Vd80NumGET_TRIGGER_EDGE,uint32_t)
#define Vd80IoctlGET_TRIGGER_TERMINATION                SKELU_IOR(Vd80NumGET_TRIGGER_TERMINATION,uint32_t)
#define Vd80IoctlSET_ANALOGUE_TRIGGER                   SKELU_IOW(Vd80NumSET_ANALOGUE_TRIGGER,Vd80DrvrAnalogTrig)
#define Vd80IoctlGET_ANALOGUE_TRIGGER                   SKELU_IOWR(Vd80NumGET_ANALOGUE_TRIGGER,Vd80DrvrAnalogTrig)
#define Vd80IoctlGET_STATE                              SKELU_IOR(Vd80NumGET_STATE,uint32_t)
#define Vd80IoctlSET_COMMAND                            SKELU_IOW(Vd80NumSET_COMMAND,uint32_t)
#define Vd80IoctlREAD_ADC                               SKELU_IOWR(Vd80NumREAD_ADC,uint32_t)
#define Vd80IoctlREAD_SAMPLE                            SKELU_IOWR(Vd80NumREAD_SAMPLE,Vd80SampleBuf)
#define Vd80IoctlGET_POSTSAMPLES                        SKELU_IOR(Vd80NumGET_POSTSAMPLES,uint32_t)
#define Vd80IoctlSET_POSTSAMPLES                        SKELU_IOW(Vd80NumSET_POSTSAMPLES,uint32_t)
#define Vd80IoctlGET_TRIGGER_CONFIG                     SKELU_IOWR(Vd80NumGET_TRIGGER_CONFIG,Vd80DrvrTrigConfig)
#define Vd80IoctlSET_TRIGGER_CONFIG                     SKELU_IOW(Vd80NumSET_TRIGGER_CONFIG,Vd80DrvrTrigConfig)
#define Vd80IoctlLAST                                   SKELU_IO(Vd80NumLAST)

#define SkelUserIoctlFIRST Vd80IoctlFIRST
#define SkelUserIoctlLAST Vd80IoctlLAST

#endif /* _SKEL_USER_IOCTL_H_ */
