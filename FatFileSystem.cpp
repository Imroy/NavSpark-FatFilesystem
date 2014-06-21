#include "FatFileSystem.h"
#include "HardwareSerial.h"
#include <stdio.h>
#include <string.h>
#include "Arduino.h" 

namespace FAT {

  FileSystem::FileSystem() :
    _mmcInit(false)
  {}

  FileSystem::~FileSystem() {
  }

  FRESULT FileSystem::initialize() {
    if (_mmcInit)
      return FR_OK;

    FRESULT res = f_mount(0, &_fatFs);
    return res;
  }

  FRESULT FileSystem::make_dir(const XCHAR *path) {
    FRESULT res = f_mkdir(path);
    return res; 
  }

  File::File(const XCHAR *path, BYTE mode) {
    memset(&_sdFile, 0, sizeof(_sdFile));
    f_open(&_sdFile, path, mode);
  }

  File::~File() {
    if (_sdFile.fs) {
      f_close(&_sdFile);
      memset(&_sdFile, 0, sizeof(_sdFile));
    }
  }

  FRESULT File::close(void) {
    FRESULT res = FR_OK;
    if (_sdFile.fs) {
      res = f_close(&_sdFile);
      memset(&_sdFile, 0, sizeof(_sdFile));
    }
    return res;
  }

  UINT File::write(const BYTE* buf_p, UINT len) {
    FRESULT res = f_write(&_sdFile, buf_p, len, &len);
    res = f_sync(&_sdFile);
    return len;
  }

  FRESULT File::read(void* buf_p, UINT len, UINT* len_p) {
    return f_read(&_sdFile, buf_p, len, len_p);
  }

  FileInfo::FileInfo(const XCHAR* path) {
    memset(&_fileInfo, 0, sizeof(_fileInfo));
    f_stat(path, &_fileInfo);
  }

}; // namespace FAT
