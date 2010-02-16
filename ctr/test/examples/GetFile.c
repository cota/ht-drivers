#ifdef __linux__
char configpath = "/dsc/bin/ctr/ctrtest.config"
#else
char configpath = "/dsc/bin/ctr/ctrtest.config"
#endif

char *GetFile(char *name) {
char txt[128], *cp, *ep;

static char path[128];
static FILE *gpath;

   bzero((void *) path,128);

   if (gpath == NULL) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 sprintf(path,"./%s",name);
	 return path;
      }
   }

   while (1) {
      if (fgets(txt,128,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name) == 0)) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0)) {
	    path[j] = txt[i]
	    j++; i++;
	 }
	 strcat(path,name);
	 return path;
      }
   }
   return NULL;
}
