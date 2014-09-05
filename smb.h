#ifndef __SMB_H__
#define __SMB_H__

#include <stdint.h>

#define SMB2_MAGIC 0x424d53fe

// commands
enum {
  SMB2_NEGOTIATE = 0x0000,
  SMB2_SESSION_SETUP = 0x0001,
  SMB2_LOGOFF = 0x0002,
  SMB2_TREE_CONNECT = 0x0003,
  SMB2_TREE_DISCONNECT = 0x0004,
  SMB2_CREATE = 0x0005,
  SMB2_CLOSE = 0x0006,
  SMB2_FLUSH = 0x0007,
  SMB2_READ = 0x0008,
  SMB2_WRITE = 0x0009,
  SMB2_LOCK = 0x000A,
  SMB2_IOCTL = 0x000B,
  SMB2_CANCEL = 0x000C,
  SMB2_ECHO = 0x000D,
  SMB2_QUERY_DIRECTORY = 0x000E,
  SMB2_CHANGE_NOTIFY = 0x000F,
  SMB2_QUERY_INFO = 0x0010,
  SMB2_SET_INFO = 0x0011,
  SMB2_OPLOCK_BREAK = 0x0012
};

// flags
enum {
  SMB2_FLAGS_SERVER_TO_REDIR = 0x00000001,
  SMB2_FLAGS_ASYNC_COMMAND = 0x00000002,
  SMB2_FLAGS_RELATED_OPERATIONS = 0x00000004,
  SMB2_FLAGS_SIGNED = 0x00000008
  // integer overflow
  /*
  SMB2_FLAGS_DFS_OPERATIONS = 0x10000000,
  SMB2_FLAGS_REPLAY_OPERATION = 0x20000000
  */
};

#define SMB2_FLAGS_DFS_OPERATIONS 0x10000000
#define SMB2_FLAGS_REPLAY_OPERATION 0x20000000

// symlink error response flags.
enum {

  SYMLINK_FLAG_RELATIVE = 0x00000001
};

// negotiate flags
enum {
  SMB2_NEGOTIATE_SIGNING_ENABLED = 0x0001,
  SMB2_NEGOTIATE_SIGNING_REQUIRED = 0x0002,

  SMB2_GLOBAL_CAP_DFS = 0x00000001,
  SMB2_GLOBAL_CAP_LEASING = 0x00000002,
  SMB2_GLOBAL_CAP_LARGE_MTU = 0x00000004,
  SMB2_GLOBAL_CAP_MULTI_CHANNEL = 0x00000008,
  SMB2_GLOBAL_CAP_PERSISTENT_HANDLES = 0x00000010,
  SMB2_GLOBAL_CAP_DIRECTORY_LEASING = 0x00000020,
  SMB2_GLOBAL_CAP_ENCRYPTION = 0x00000040
};

// session setup flags
enum {
  SMB2_SESSION_FLAG_BINDING = 0x01
};

// session setup response flags
enum {
  SMB2_SESSION_FLAG_IS_GUEST = 0x0001,
  SMB2_SESSION_FLAG_IS_NULL = 0x0002,
  SMB2_SESSION_FLAG_ENCRYPT_DATA = 0x0004
};

// tree connect flags
enum {
  SMB2_SHARE_TYPE_DISK = 0x01,
  SMB2_SHARE_TYPE_PIPE = 0x02,
  SMB2_SHARE_TYPE_PRINT = 0x03
};

// tree connect share flags.
enum {
  SMB2_SHAREFLAG_MANUAL_CACHING = 0x0000,
  SMB2_SHAREFLAG_AUTO_CACHING = 0x0010,
  SMB2_SHAREFLAG_VDO_CACHING = 0x0020,
  SMB2_SHAREFLAG_NO_CACHING = 0x0030,
  SMB2_SHAREFLAG_DFS = 0x0001,
  SMB2_SHAREFLAG_DFS_ROOT = 0x0002,
  SMB2_SHAREFLAG_RESTRICT_EXCLUSIVE_OPENS = 0x0100,
  SMB2_SHAREFLAG_FORCE_SHARED_DELETE = 0x0200,
  SMB2_SHAREFLAG_ALLOW_NAMESPACE_CACHING = 0x0400,
  SMB2_SHAREFLAG_ACCESS_BASED_DIRECTORY_ENUM = 0x0800,
  SMB2_SHAREFLAG_FORCE_LEVELII_OPLOCK = 0x1000,
  SMB2_SHAREFLAG_ENABLE_HASH_V1 = 0x2000,
  SMB2_SHAREFLAG_ENABLE_HASH_V2 = 0x4000,
  SMB2_SHAREFLAG_ENCRYPT_DATA = 0x8000
};

// tree connect capabilities
enum {
  SMB2_SHARE_CAP_DFS = 0x0008,
  SMB2_SHARE_CAP_CONTINUOUS_AVAILABILITY = 0x0010,
  SMB2_SHARE_CAP_SCALEOUT = 0x0020,
  SMB2_SHARE_CAP_CLUSTER = 0x0040,
  SMB2_SHARE_CAP_ASYMMETRIC = 0x0080
};

// create - oplock
enum {
  SMB2_OPLOCK_LEVEL_NONE = 0x00,
  SMB2_OPLOCK_LEVEL_II = 0x01,
  SMB2_OPLOCK_LEVEL_EXCLUSIVE = 0x08,
  SMB2_OPLOCK_LEVEL_BATCH = 0x09,
  SMB2_OPLOCK_LEVEL_LEAVE = 0xff
};

// create - impersonation
enum {
  IMPERSONATION_ANONYMOUS = 0x00,
  IMPERSONATION_IDENTIFICATION = 0x01,
  IMPERSONATION_IMPERSONATION = 0x02,
  IMPERSONATION_DELEGATE = 0x03
};




// create - share access
enum {
  FILE_SHARE_READ = 0x01,
  FILE_SHARE_WRITE = 0x02,
  FILE_SHARE_DELETE = 0x04
};




// create - disposition
enum {
  FILE_SUPERSEDE = 0x0000,
  FILE_OPEN = 0x0001,
  FILE_CREATE = 0x0002,
  FILE_OPEN_IF = 0x0003,
  FILE_OVERWRITE = 0x0004,
  FILE_OVERWRITE_IF = 0x0005
};

// create - options
enum {
  FILE_DIRECTORY_FILE = 0x0001,
  FILE_WRITE_THROUGH = 0x0002,
  FILE_SEQUENTIAL_ONLY = 0x0004,
  FILE_NO_INTERMEDIATE_BUFFERING = 0x0008,
  FILE_NON_DIRECTORY_FILE = 0x0040,
  FILE_NO_EA_KNOWLEDGE = 0x0200,
  FILE_RANDOM_ACCESS = 0x0800,
  FILE_DELETE_ON_CLOSE = 0x1000,
  FILE_OPEN_FOR_BACKUP_INTENT = 0x4000,
  FILE_NO_COMPRESSION = 0x8000
};

#define FILE_OPEN_REPARSE_POINT 0x00200000
#define FILE_OPEN_NO_RECALL 0x00400000



// create - access
// 2.2.13.1.1
enum {
  FILE_READ_DATA = 0x0001,
  FILE_WRITE_DATA = 0x0002,
  FILE_APPEND_DATA = 0x0004,
  FILE_READ_EA = 0x0008,
  FILE_WRITE_EA = 0x0010,
  FILE_DELETE_CHILD = 0x0040,
  FILE_EXECUTE = 0x0020,
  FILE_READ_ATTRIBUTES = 0x0080,
  FILE_WRITE_ATTRIBUTES = 0x0100
};

#define DELETE 0x00010000
#define READ_CONTROL 0x00020000
#define WRITE_DAC 0x00040000
#define WRITE_OWNER 0x00080000
//#define SYNCHRONIZE 0x00100000 // not used.
#define ACCESS_SYSTEM_SECURITY 0x01000000
#define MAXIMUM_ALLOWED 0x02000000
#define GENERIC_ALL 0x10000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_WRITE 0x40000000
#define GENERIC_READ 0x80000000

// 2.2.13.1.2
enum {
  FILE_LIST_DIRECTORY = 0x0001,
  FILE_ADD_FILE = 0x0002,
  FILE_ADD_SUBDIRECTORY = 0x0004,
  FILE_TRAVERSE = 0x0020
};

// create file attributes - MS-FSCC

enum {
  FILE_ATTRIBUTE_ARCHIVE = 0x0020,
  FILE_ATTRIBUTE_COMPRESSED = 0x0800,
  FILE_ATTRIBUTE_DIRECTORY = 0x0010,
  FILE_ATTRIBUTE_ENCRYPTED = 0x4000,
  FILE_ATTRIBUTE_HIDDEN = 0x0002,
  FILE_ATTRIBUTE_NORMAL = 0x0080,
  FILE_ATTRIBUTE_NOT_CONTENT_INDEXED = 0x2000,
  FILE_ATTRIBUTE_OFFLINE = 0x1000,
  FILE_ATTRIBUTE_READONLY = 0x0001,
  FILE_ATTRIBUTE_REPARSE_POINT = 0x0400,
  FILE_ATTRIBUTE_SPARSE_FILE = 0x0200,
  FILE_ATTRIBUTE_SYSTEM = 0x0004,
  FILE_ATTRIBUTE_TEMPORARY = 0x0100,
  FILE_ATTRIBUTE_INTEGRITY_STREAM = 0x8000
};
#define FILE_ATTRIBUTE_NO_SCRUB_DATA 0x00020000




// create response - flags
enum {
  SMB2_CREATE_FLAG_REPARSEPOINT  = 0x01
};

// create response - action
enum {
  FILE_SUPERSEDED =  0x0000,
  FILE_OPENED =  0x0001,
  FILE_CREATED =  0x0002,
  FILE_OVERWRITTEN =  0x0003
};

// close flags.
enum { 
  SMB2_CLOSE_FLAG_POSTQUERY_ATTRIB = 0x0001
};

// read/write flags
enum {
  SMB2_READFLAG_READ_UNBUFFERED = 0x01,
  SMB2_WRITEFLAG_WRITE_THROUGH = 0x01,
  SMB2_WRITEFLAG_WRITE_UNBUFFERED = 0x02
};

enum {
  SMB2_CHANNEL_NONE = 0x0000,
  SMB2_CHANNEL_RDMA_V1 = 0x0001,
  SMB2_CHANNEL_RDMA_V1_INVALIDATE = 0x0002
};

typedef struct smb2_header_sync {

  uint32_t protocol_id;
  uint16_t structure_size;
  uint16_t credit_charge;
  uint32_t status;
  uint16_t command;
  uint16_t credit; // _request / _response
  uint32_t flags;
  uint32_t next_command;
  uint32_t message_id[2]; // uint64_t
  uint32_t reserved;
  uint32_t tree_id;
  uint32_t session_id[2]; // uint64_t
  uint8_t signature[16];

} smb2_header_sync;

typedef struct smb2_header_async {

  uint32_t protocol_id;
  uint16_t structure_size;
  uint16_t credit_charge;
  uint32_t status;
  uint16_t command;
  uint16_t credit; // _request / _response
  uint32_t flags;
  uint32_t next_command;
  uint32_t message_id[2]; // uint64_t
  uint32_t async_id[2];
  uint32_t session_id[2]; // uint64_t
  uint8_t signature[16];

} smb2_header_async;


typedef struct smb2_error_response {
  uint16_t structure_size;
  uint16_t reserved;
  uint32_t bytecount;
  //uint8_t error_data[1]; // variable.
} smb2_error_response;

typedef struct smb2_negotiate_request {
  uint16_t structure_size;
  uint16_t dialect_count;
  uint16_t security_mode;
  uint16_t reserved;
  uint32_t capabilities;
  uint8_t client_guid[16];
  uint32_t client_start_time[2];
  //uint16_t dialects[1]; // variable sized.

} smb2_negotiate_request;



typedef struct smb2_negotiate_response {
  uint16_t structure_size;
  uint16_t security_mode;
  uint16_t dialect_revision;
  uint16_t reserved;
  uint8_t server_guid[16];
  uint32_t capabilities;
  uint32_t max_transact_size;
  uint32_t max_read_size;
  uint32_t max_write_size;
  uint32_t system_time[2];
  uint32_t server_start_time[2];
  uint16_t security_buffer_offset;
  uint16_t security_buffer_length;
  uint32_t reserved2;
  //uint8_t buffer[1]; // variable
} smb2_negotiate_response;


typedef struct smb2_session_setup_request {
  uint16_t structure_size;
  uint8_t flags;
  uint8_t security_mode;
  uint32_t capabilities;
  uint32_t channel;
  uint16_t security_buffer_offset;
  uint16_t security_buffer_length;
  uint32_t previous_session_id[2];
  //uint8t_t buffer[1]; // variable

} smb2_session_setup_request;


typedef struct smb2_session_setup_response {
  uint16_t structure_size;
  uint16_t session_flags;
  uint16_t security_buffer_offset;
  uint16_t security_buffer_length;
  //uint8t_t buffer[1]; // variable
} smb2_session_setup_response;


typedef struct smb2_tree_connect_request {
  uint16_t structure_size;
  uint16_t reserved;
  uint16_t path_offset;
  uint16_t path_length;
  // uint8_t buffer[0];
} smb2_tree_connect_request;

typedef struct smb2_tree_connect_response {
  uint16_t structure_size;
  uint8_t share_type;
  uint8_t reserved;
  uint32_t share_flags;
  uint32_t capabilities;
  uint32_t maximal_access;
} smb2_tree_connect_response;

typedef struct smb2_tree_disconnect_request {
  uint16_t structure_size;
  uint16_t reserved;
} smb2_tree_disconnect_request;

typedef struct smb2_tree_disconnect_response {
  uint16_t structure_size;
  uint16_t reserved;
} smb2_tree_disconnect_response;

typedef struct smb2_logoff_request {
  uint16_t structure_size;
  uint16_t reserved;
} smb2_logoff_request;

typedef struct smb2_logoff_response {
  uint16_t structure_size;
  uint16_t reserved;
} smb2_logoff_response;

typedef struct smb2_create_request {
  uint16_t structure_size;
  uint8_t security_flags;
  uint8_t request_oplock_level;
  uint32_t impersonation_level;
  uint32_t smb_create_flags[2];
  uint32_t reserved[2];
  uint32_t desired_access;
  uint32_t file_attributes;
  uint32_t share_access;
  uint32_t create_disposition;
  uint32_t create_options;
  uint16_t name_offset;
  uint16_t name_length;
  uint32_t create_contexts_offset;
  uint32_t create_contexts_length;
  //uint8_t buffer[0];
} smb2_create_request;

typedef struct smb2_create_response {
  uint16_t structure_size;
  uint8_t oplock_level;
  uint8_t flags;
  uint32_t create_action;
  uint32_t creation_time[2];
  uint32_t last_access_time[2];
  uint32_t last_write_time[2];
  uint32_t change_time[2];
  uint32_t allocation_size[2];
  uint32_t end_of_file[2];
  uint32_t file_attributes;
  uint32_t reserved2;
  uint32_t file_id[4];
  uint32_t create_contexts_offset;
  uint32_t create_contexts_length;
  // uint8_t buffer[0];
} smb2_create_response;

typedef struct smb2_close_request {
  uint16_t structure_size;
  uint16_t flags;
  uint32_t reserved;
  uint32_t file_id[4];
} smb2_close_request;

typedef struct smb2_close_response {
  uint16_t structure_size;
  uint16_t flags;
  uint32_t reserved;
  uint32_t creation_time[2];
  uint32_t last_access_time[2];
  uint32_t last_write_time[2];
  uint32_t change_time[2];
  uint32_t allocation_size[2];
  uint32_t end_of_file[2];
  uint32_t file_attributes;
} smb2_close_response;

typedef struct smb2_flush_request {
  uint16_t structure_size;
  uint16_t reserved;
  uint32_t reserved2;
  uint32_t file_id[4];
} smb2_flush_request;

typedef struct smb2_flush_response {
  uint16_t structure_size;
  uint16_t reserved;
} smb2_flush_response;

typedef struct smb2_read_request {
  uint16_t structure_size;
  uint8_t padding;
  uint8_t flags;
  uint32_t length;
  uint32_t offset[2];
  uint32_t file_id[4];
  uint32_t minimum_count;
  uint32_t channel;
  uint32_t remaining_bytes;
  uint16_t read_channel_info_offset;
  uint16_t read_channel_info_length;
  // uint8_t buffer[0];
} smb2_read_request;

typedef struct smb2_read_response {
  uint16_t structure_size;
  uint8_t data_offset;
  uint8_t reserved;
  uint32_t data_length;
  uint32_t data_remaining;
  uint32_t reserved2;
  // uint8_t buffer[0];
} smb2_read_response;

#endif
