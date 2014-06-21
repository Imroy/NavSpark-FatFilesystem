#include "FatFileSystem.h"
#include "HardwareSerial.h"
#include <stdio.h>
#include <string.h>
#include "Arduino.h" 

namespace FAT {

  FileSystem::FileSystem() :
    _mmcInit(false)
  {
    memset(&_sdFile, 0, sizeof(_sdFile));
    memset(&_fileInfo, 0, sizeof(_fileInfo));
  }

  FileSystem::~FileSystem() {
  }

  FRESULT FileSystem::initialize() {
    if (_mmcInit)
      return FR_OK;

    FRESULT res = f_mount(0, &_fatFs);
    close_file();
    return res;
  }

  void FileSystem::close_file() {
    if (!_sdFile.fs)
      return;

    f_close(&_sdFile);
    memset(&_sdFile, 0, sizeof(_sdFile));
  }

  FRESULT FileSystem::make_dir(const XCHAR *path) {
    FRESULT res = f_mkdir(path);
    return res; 
  }

  FRESULT FileSystem::open_file(const XCHAR *path, BYTE mode) {
    if (_sdFile.fs) {
      f_close(&_sdFile);
      memset(&_sdFile, 0, sizeof(_sdFile));
    }
  
    strcpy(_file_name, path);
    FRESULT res = f_open(&_sdFile, _file_name, mode);
    if (res==FR_OK) {
      f_stat(path, &_fileInfo);
    }

    return res; 
  }

  UINT FileSystem::write_file(const BYTE* buf_p, UINT len) {
    FRESULT res = f_write(&_sdFile, buf_p, len, &len);
    res = f_sync(&_sdFile);
    return len;
  }

  FRESULT FileSystem::file_read(void* buf_p, UINT len, UINT* len_p) {
    return f_read(&_sdFile, buf_p, len, len_p);
  }

}; // namespace FAT
