/* ===================================================== */
/* Copy from Vd80 hardware memory to user virtual memory */
/* ===================================================== */

int Vd80CopyToUser(Vd80SampleBuf *sbuf, SkelDrvrModuleContext *mcon) {

U32 *regs = NULL;
int samples, tval;

#ifdef __linux__
int remaining;
struct page **pages;
short *vaddr, *kaddr;
int offset, npages, j, cc;
int length, i;
#endif

   if ((regs = GetRegs(mcon)) == NULL) return 0;

#ifdef __linux__

   vaddr  = sbuf->SampleBuf;
   length = sbuf->Samples * sizeof(short);

   npages = (((unsigned long) vaddr & ~PAGE_MASK) + length + ~PAGE_MASK) >> PAGE_SHIFT;

   if ((pages = kmalloc(npages * sizeof(struct page *), GFP_KERNEL)) == NULL) return -ENOMEM;

   down_read(&current->mm->mmap_sem);
   cc = get_user_pages(current, current->mm, (unsigned long) vaddr & PAGE_MASK, npages, 1, 0, pages, NULL);
   up_read(&current->mm->mmap_sem);

   if (cc < npages) {
      for (i = 0; i < cc; i++) page_cache_release(pages[i]);
      kfree(pages);
      return -ENOMEM;
   }

   offset = (unsigned long) vaddr & (PAGE_SIZE -1);
   remaining = length / sizeof(short);
   samples = 0;

   for (i=0; i<npages; i++) {
      kaddr = (unsigned short *) kmap(pages[i]);
      for (j=offset; ((j<PAGE_SIZE) && (remaining>0)); j+=2) {
	 tval = GetReg(regs,VD80_MRWR,mcon);
#if 1
	 sbuf->SampleBuf[samples +1] = (short) (tval >> 00) & 0xFFFF;
	 sbuf->SampleBuf[samples +0] = (short) (tval >> 16) & 0xFFFF;
#else
	 kaddr[j + 1] = (short) (tval >> 00) & 0xFFFF;
	 kaddr[j + 0] = (short) (tval >> 16) & 0xFFFF;
#endif
	 samples   += 2;
	 remaining -= 2;
	 udelay(1);
      }
      offset = 0;
      SetPageDirty(pages[i]);
      page_cache_release(pages[i]);
      kunmap(pages[i]);
   }
   kfree(pages);
   return samples;

#else

   /* LynxOs direct memory access */

   for (samples=0; samples<sbuf->Samples; samples+=2) {
      tval = GetReg(regs,VD80_MRWR,mcon);
      sbuf->SampleBuf[samples + 0] = (short) (tval >> 00) & 0xFFFF;
      sbuf->SampleBuf[samples + 1] = (short) (tval >> 16) & 0xFFFF;
      usec_sleep(1);
   }

   return samples;

#endif
}
