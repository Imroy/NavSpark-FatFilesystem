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
    memset(&_fileInfo, 0, sizeof(_fileInfo));

    strcpy(_file_name, path);
    FRESULT res = f_open(&_sdFile, _file_name, mode);
    if (res==FR_OK) {
      f_stat(path, &_fileInfo);
    }
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

}; // namespace FAT
