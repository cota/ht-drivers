/* ***************************************************************** */
/* For programs that want to do pretty printing of various IO fields */
/* Julian Lewis 26th Nov 2003. CERN AB/CO/HT Geneva                  */
/* Julian.Lewis@cern.ch                                              */
/* ***************************************************************** */

#ifndef IO_FIELDS
#define IO_FIELDS

/* IO Field names */

#define IoFieldNAME_SIZE 32
#define IoFieldCOMMENT_SIZE 64

typedef struct {
   unsigned long FieldSize;
   char          Name[IoFieldNAME_SIZE];
   unsigned long Offset;
   char          Comment[IoFieldCOMMENT_SIZE];
 } IoField;

typedef enum {
   IoFieldTypeNONE,
   IoFieldTypePLX_CONFIG,
   IoFieldTypePLX_LOCAL,
   IoFieldTypeRFM_REGS,
   IoFieldTypeVMIC_SDRAM
 } IoFieldType;

#endif
