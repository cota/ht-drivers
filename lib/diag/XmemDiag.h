#include <libxmem.h>

#define CmdSIZE 8

typedef enum {
   CmdNULL,

   CmdGET_ALL_NODE_IDS,
   CmdGET_NODE_NAME,
   CmdGET_NODE_ID,

   CmdGET_ALL_TABLE_IDS,
   CmdGET_TABLE_NAME,
   CmdGET_TABLE_ID,
   CmdGET_TABLE_DESC,

   CmdWAIT,
   CmdPOLL,

   CmdSEND_TABLE,
   CmdRECV_TABLE,

   CmdSEND_MESSAGE,

   CmdCHECK_TABLES,

   CmdCOMMANDS
 }

typedef struct {
   char Cmd[CmdSIZE];
   int  Idx;
 } Cmd;

static Cmd Cmds[CmdCOMMANDS];
