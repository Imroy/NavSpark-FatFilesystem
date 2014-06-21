#include "FatFileSystem.h"
#include "HardwareSerial.h"
#include <stdio.h>
#include <string.h>
#include "Arduino.h" 

FatFileSystem::FatFileSystem() :
  _mmcInit(false)
{
  memset(&_sdFile, 0, sizeof(_sdFile));
  memset(&_fileInfo, 0, sizeof(_fileInfo));
}

FatFileSystem::~FatFileSystem() {
}
 
FRESULT FatFileSystem::initialize() {
  if (_mmcInit)
    return FR_OK;

  FRESULT res = f_mount(0, &_fatFs);
  close_file();
  return res;
}

void FatFileSystem::close_file() {
  if (!_sdFile.fs)
    return;

  f_close(&_sdFile);
  memset(&_sdFile, 0, sizeof(_sdFile));
}

FRESULT FatFileSystem::make_dir(const XCHAR *path) {
  FRESULT res = f_mkdir(path);
  return res; 
}

FRESULT FatFileSystem::open_file(const XCHAR *path, BYTE mode) {
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

UINT FatFileSystem::write_file(const BYTE* buf_p, UINT len) {
  FRESULT res = f_write(&_sdFile, buf_p, len, &len);
  res = f_sync(&_sdFile);
  return len;
}

FRESULT FatFileSystem::file_read(void* buf_p, UINT len, UINT* len_p) {
  return f_read(&_sdFile, buf_p, len, len_p);
}
