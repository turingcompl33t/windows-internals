## Characters and Strings

Windows introduces several complexities when working with strings in the name of a wider degree of language support.

### Code Pages

A _code page_ is a character encoding, which is to say that it is a mapping between a set of characters (printable and control) and unique numbers.

### Multi-Byte Character Set (MBCS) vs Unicode

In Visual Studio projects, project configuration options allow one to specify the character set for the project as one of the following values:

- `MBCS`
- `Unicode`

Selecting one of these two options has the effect of defining or undefining the `UNICODE` (used by the Windows API) and `_UNICODE` (used by the C/C++ runtime) macros.

In Windows, MBCS ALWAYS refers to a double-byte character set (DBCS) - dynamic character sets involving more than two bytes are not supported.

MSDN explicitly states that new applications should avoid MBCS and should instead opt for Unicode.

### Character Encodings

In classic C, the `char` data type represents a single ASCII character. The size of a `char` is 1 byte (8 bits) which was perfectly compatible with the ASCII encoding because ASCII only utilized seven (7) bits to encode all of the characters that it could represent. 

As time went on, however, encodings needed to be able to support a larger space of possible characters. Enter Unicode.

**Common Unicode Encodings**

Unicode offers a variety of encodings. Below, "UTF" stands for _Unicode Transformation Format_. According to the Unicode documentation, a UTF is "an algorithmic mapping from every Unicode code point (except surrogate code points) to a unique byte sequence."

- UTF-8: this is the primary encoding for the web; the encoding is dynamic, meaning that the number of bytes used to represent a single character varies by character; it utilizes from 1-4 bytes for each character; the encoding is backwards compatible with ASCII, meaning that for most English-language text each character will only be represented by a single byte 
- UTF-16: utilizes 2 bytes per character; may use up to 4 bytes for certain, rare characters
- UTF-32: utilizes 4 bytes per character; static encoding, meaning that 4 bytes are always used, regardless of the encoding for the particular character

While UTF-8 may appear the clear winner here, it does present a critical issue: the dynamic nature of the UTF-8 encoding prevents random access into strings. Because one does not know ahead of time whether each character in the string is represented by 1, 2, 3, or 4 bytes, the string must be scanned sequentially in order to locate a particular character. In contrast, UTF-16 is random access under all but the most rare circumstances.

**American National Standards Institute (ANSI) / Windows-1252**

ANSI is a private, nonprofit standards body with specific ties to the United States.

As the term is commonly applied to character encodings and code pages, it is a misnomer. It is incorrectly used to refer to the Windows-1252 (Windows "Western") code page which was originally based upon an ANSI draft that later became an ISO standard.

**American Standard Code for Information Interchange (ASCII)**

TODO

**UTF-7**

TODO

**UTF-8**

What is UTF-8? According to the Unicode documentation:

```
UTF-8 is the byte-oriented encoding form of Unicode.
```

**UTF-16**

What is UTF-16? According to the Unicode documentation:

```
UTF-16 uses a single 16-bit code unit to encode the most common 63K characters, and a pair of 16-bit code units, called surrogates, to encode the 1M less commonly used characters in Unicode
```

**UTF-32**

What is UTF-32? According to the Unicode documentation:

```
 Any Unicode character can be represented as a single 32-bit unit in UTF-32. This single 4 code unit corresponds to the Unicode scalar value, which is the abstract number associated with a Unicode character.
```

**UTF-16, UTF-32, and Endianness**

Because the code units utilized by UTF-16 and UTF-32 are composed of multiple bytes (2 bytes for UTF-16, 4 bytes for UTF-32) the endianness of the representation must be taken into account. For this reason, both UTF-16 and UTF-32 come in three distinct flavors:

- UTF-16/UTF-32: the default version; big-endian byte ordering is assumed, BUT a byte order mark (BOM) may be prepended to the byte stream to explicitly indicate the byte ordering
- UTF-16-LE/UTF-32-LE: explicit little-endian byte ordering
- UTF-16-BE/UTF-32-BE: explicit big-endian byte ordering

### Windows Types and Character Encodings

The Windows kernel utilizes UTF-16 internally.

The table below outlines the common types used when representing characters on Windows.

| Common Type(s)  | ASCII Type(s)      | UNICODE Type(s)         |
|-----------------|--------------------|-------------------------|
| TCHAR           | char, CHAR         | wchar_t, WCHAR          |
| LPTSTR, PTSTR   | char*, CHAR*, PSTR | wchar_t*, WCHAR*, PWSTR |
| LPCTSTR, PCTSTR | const char*, PCSTR | const wchar_t*, PCWSTR  |

### Safe String Functions

There are (in general) two sets of safe string functions available on Windows.

The first set of safe string functions are those added to the C/C++ runtime libraries by Microsoft. Examples of safe string functions provided by the runtime libraries include:

- `strcpy_s()`
- `wcscpy_s()`

The `_s` prefix denotes that the function is "safe."

The other set of safe string functions are those provided directly by the Windows API. These functions are available in the `<strsafe>` header. Examples include:

- `StringCchCopy()`
- `StringCchCat()`

Here, `Cch` stands for "count of characters", which itself raises an important point: all buffer sizes and lengths associated with both these API sets are expressed in **characters** rather than BYTES. This is obviously a critical distinction. 

The `<strsafe>` header also defines analogous functions that work in terms of bytes rather than character counts. These functions are identified by the `StringCb` prefix. 

### String Conversions

As in the case of the safe string functions, two options are available when converting between different string representations:

- Windows API functions
- C/C++ Runtime Library functions

The Windows API provides the following functions:

- `WideCharToMultiByte()`: Unicode (UTF-16) to a "character string"
- `MultiByteToWideChar()`: "character string" to Unicode (UTF-16)

The C runtime library provides the following functions:

- `wcstombs_s()`: wide character string to multi-byte string, safe
- `_wcstombs_s_l()`: wide character string to multi-byte string, safe, locale specified
- `mbstowcs_s()`: multi-byte string to wide character string, safe
- `_mbstowcs_s_l()`: multi-byte string to wide characters string safe, locale specified

Furthermore, the C++ standard library defines similar functions within the `std` namespace.

### References

- _Windows System Programming, 4th Edition_ Pages 34-37
- _Windows 10 System Programming_ Chapter 1
- [MSDN: String Conversions](https://docs.microsoft.com/en-us/windows/win32/menurc/strsafe-ovw?redirectedfrom=MSDN)