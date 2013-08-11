#pragma optimize 79

#include <GSOS.h>

#include <stdlib.h>
#include <gno/gno.h>

int setfileattr(const char *filename, FileInfoRecGS *info, unsigned flags)
{
    Word rv;
    FileInfoRecGS tmp;

    if (!info) return 0;
    if (!flags) return 1;

    tmp.pCount = 7;
    tmp.pathname = (GSString255Ptr)__C2GSMALLOC(filename);
    if (!tmp.pathname) return 0;

    GetFileInfoGS(&tmp);
    rv = _toolErr;
    if (!_toolErr)
    {
        if (flags & ATTR_ACCESS) tmp.access = info->access;
        if (flags & ATTR_FILETYPE) tmp.fileType = info->fileType;
        if (flags & ATTR_AUXTYPE) tmp.auxType = info->auxType;
        if (flags & ATTR_CREATETIME) tmp.createDateTime = info->createDateTime;
        if (flags & ATTR_MODTIME) tmp.modDateTime = info->modDateTime;

        SetFileInfoGS(&tmp);
        rv = _toolErr;
    }

    free (tmp.pathname);

    return rv ? 0 : 1;
}