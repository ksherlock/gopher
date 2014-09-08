#ifndef __SMB_ERRORS_H__
#define __SMB_ERRORS_H__

#define STATUS_SUCCESS 0x00000000

#define STATUS_NOTIFY_CLEANUP 0x0000010B
#define STATUS_NOTIFY_ENUM_DIR 0x0000010C

#define STATUS_BUFFER_OVERFLOW 0x80000005
#define STATUS_NO_MORE_FILES 0x80000006

#define STATUS_UNSUCCESSFUL 0xC0000001
#define STATUS_INVALID_PARAMETER 0xC000000D
#define STATUS_NO_SUCH_DEVICE 0xC000000E
#define STATUS_NO_SUCH_FILE 0xC000000F
#define STATUS_END_OF_FILE 0xC0000011
#define STATUS_MORE_PROCESSING_REQUIRED 0xC0000016
#define STATUS_NO_MEMORY 0xC0000017
#define STATUS_ACCESS_DENIED 0xC0000022
#define STATUS_BUFFER_TOO_SMALL 0xC0000023
#define STATUS_OBJECT_NAME_INVALID 0xC0000033
#define STATUS_OBJECT_NAME_NOT_FOUND 0xC0000034
#define STATUS_OBJECT_NAME_COLLISION 0xC0000035
#define STATUS_FILE_CLOSED 0xC0000128
#define STATUS_NO_USER_SESSION_KEY 0xC0000202
#define STATUS_USER_SESSION_DELETED 0xC0000203
#define STATUS_FILE_NOT_AVAILABLE 0xC0000467


#endif
