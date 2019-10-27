### Registry Data Types

- REG_BINARY: binary data in any form
- REG_DWORD: 32-bit number
- REG_QWORD: 64-bit number
- REG_DWORD_LITTLE_ENDIAN: a 32-bit number in little-endian format, the default storage format, thus equivalent to REQ_DWORD
- REG_QWORD_LITTLE_ENDIAN: a 64-bit number in little-endian-format, the default storage format, thus equivalent to the REG_QWORD
- REG_DWORD_BIG_ENDIAN: a 32-bit number in big-endian format
- REG_EXPAND_SZ: null terminated string with unexpanded references to environment variables e.g. %PATH%
    - May be unicode or ANSI string depending on the functions used to manipulate 
- REG_LINK: unicode symbolic link
- REG_MULTI_SZ: array of null-terminated strings, each of which is terminated by two null characters
- REG_NONE: no defined value type
- REG_RESOURCE_LIST: device driver resource list
- REG_SZ: null-terminated string
    - May be unicode or ANSI depending on the functions used to manipulate
