#pragma once

#include "diskio.h" 
#include "ff.h" 

class FatFileSystem {
protected:
  bool _mmcInit;
  FATFS _fatFs;
  FIL _sdFile;
  XCHAR _file_name[256];
  FILINFO _fileInfo;

public:
  FatFileSystem();
  virtual ~FatFileSystem();
  
  FRESULT initialize();
  FRESULT make_dir(const XCHAR *path);
  FRESULT open_file(const XCHAR *path, BYTE mode);
  void close_file();
  UINT write_file(const BYTE* buf_p, UINT len);
  DWORD get_file_size() { return _fileInfo.fsize; }
  FRESULT file_read (void*, UINT, UINT*);      

};
