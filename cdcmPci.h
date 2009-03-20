/**
 * @file cdcmPci.h
 *
 * @brief PCI definitions
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _CDCM_PCI_H_INCLUDE_
#define _CDCM_PCI_H_INCLUDE_

#define CDCM_MAX_PCI_ISRS 4

struct cdcm_pci_isr {
  int irq;
  void *cookie;
  void (*isr)(void *);
};

#endif /* _CDCM_PCI_H_INCLUDE */
