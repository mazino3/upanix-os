#ifndef _DLFCN_H_
#define _DLFCN_H_

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct {
    const char *dli_fname;  /* Pathname of shared object that
                               contains address */
    void       *dli_fbase;  /* Address at which shared object
                               is loaded */
    const char *dli_sname;  /* Name of nearest symbol with address
                               lower than addr */
    void       *dli_saddr;  /* Exact address of symbol named
                               in dli_sname */
} Dl_info;


int dladdr(void *addr, Dl_info *info);

#if defined (__cplusplus)
}
#endif

#endif
