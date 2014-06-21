#pragma once

#include "diskio.h" 
#include "ff.h" 

namespace FAT {

  class with_result {
  protected:
    FRESULT _result;

  public:
    const FRESULT result(void) const { return _result; }

  }; // class with_results

  class FileSystem : public with_result {
  protected:
    bool _mmcInit;
    FATFS _fatFs;

  public:
    FileSystem();
    virtual ~FileSystem();

    void initialize();
    void make_dir(const XCHAR *path);
  }; // class FileSystem

  class File : public with_result {
  protected:
    FIL _sdFile;

  public:
    File(const XCHAR *path, BYTE mode);
    ~File();

    void close(void);

    void lseek(DWORD ofs);
    UINT write(const BYTE* buf_p, UINT len);
    void read (void* buf_p, UINT len, UINT* len_p);
  }; // class File

  class Directory;
  class FileInfo : public with_result {
  protected:
    FILINFO _fileInfo;

    friend class Directory;
    FileInfo();

  public:
    FileInfo(const XCHAR* path);

    const DWORD size(void) const { return _fileInfo.fsize; }
    const WORD date(void) const { return _fileInfo.fdate; }
    const WORD time(void) const { return _fileInfo.ftime; }
    const BYTE attrib(void) const { return _fileInfo.fattrib; }
    const char* name(void) const { return _fileInfo.fname; }
#if _USE_LFN
    const XCHAR* long_name(void) const { return _fileInfo.lfname; }
    const int long_name_size(void) const { return _fileInfo.lfsize; }
#endif
  };

  class Directory : public with_result {
  protected:
    DIR _Dir;

  public:
    Directory(const XCHAR *path);

    FileInfo* next_entry(void);

  }; // class Directory

}; // namespace FAT
