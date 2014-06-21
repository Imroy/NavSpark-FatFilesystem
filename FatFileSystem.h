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
    XCHAR _file_name[256];
    FILINFO _fileInfo;

  public:
    File(const XCHAR *path, BYTE mode);
    ~File();

    FRESULT close(void);

    UINT write(const BYTE* buf_p, UINT len);
    DWORD get_size() { return _fileInfo.fsize; }
    FRESULT read (void*, UINT, UINT*);

  }; // class File

}; // namespace FAT
