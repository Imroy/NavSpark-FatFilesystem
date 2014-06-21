#pragma once

#include "diskio.h" 
#include "ff.h" 

namespace FAT {

  class FileSystem {
  protected:
    bool _mmcInit;
    FATFS _fatFs;

  public:
    FileSystem();
    virtual ~FileSystem();

    FRESULT initialize();
    FRESULT make_dir(const XCHAR *path);
  }; // class FileSystem

  class File {
  protected:
    FIL _sdFile;

  public:
    File(const XCHAR *path, BYTE mode);
    ~File();

    FRESULT close(void);

    UINT write(const BYTE* buf_p, UINT len);
    FRESULT read (void*, UINT, UINT*);
  }; // class File

  class FileInfo {
  protected:
    FILINFO _fileInfo;

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

}; // namespace FAT
