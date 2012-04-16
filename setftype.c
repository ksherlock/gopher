#pragma optimize 79

#include <GSOS.h>

#include <stdlib.h>
#include <gno/gno.h>

extern int parse_ftype(const char *cp, Word size, Word *ftype, Word *atype);

int setfiletype(const char *filename)
{
    int pd;
    int i;
    
    Word ftype;
    Word atype;
    int rv;
    
    FileInfoRecGS info;
    
    // find the extension in the filename.
    
    pd = -1;
    for (i = 0; ; ++i)
    {
        char c;
        
        c = filename[i];
        if (c == 0) break;
        if (c == '.') pd = i;
    } 
    
    // pd == position of final .
    // i == strlen
    
    if (pd == -1) return 0;
    if (pd + 1 >= i) return 0;
    pd++; // skip past it...
    
    if (!parse_ftype(filename + pd, i - pd, &ftype, &atype)) 
        return 0;
        
    info.pCount = 4;
    info.pathname = (GSString255Ptr)__C2GSMALLOC(filename);
    info.access = 0xe3;
    info.auxType = atype;
    info.fileType = ftype;
    
    //GetFileInfoGS(&info);
    //if (_toolErr) return 0;
    
    SetFileInfoGS(&info);
    rv = _toolErr;
    if (_toolErr)

    free(info.pathname);
    return rv ? 0 : 1;
}
