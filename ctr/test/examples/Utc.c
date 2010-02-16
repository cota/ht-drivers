/**************************************************************************/
/* Place the current UTC time in the CTX1 driver                          */
/* Julian Lewis 15th Nov 2002                                             */
/**************************************************************************/

#include <time.h>

int main(int argc,char *argv[]) {

time_t tod;

   tod = time(NULL);
   printf("%u\n",(int) tod);
}
