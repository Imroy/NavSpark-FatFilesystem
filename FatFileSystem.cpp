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

  void FileSystem::initialize() {
    if (_mmcInit)
      return;

    _result = f_mount(0, &_fatFs);
  }

  void FileSystem::make_dir(const XCHAR *path) {
    _result = f_mkdir(path);
  }

  File::File(const XCHAR *path, BYTE mode) {
    memset(&_sdFile, 0, sizeof(_sdFile));
    _result = f_open(&_sdFile, path, mode);
  }

  File::~File() {
    _result = FR_OK;
    if (_sdFile.fs) {
      _result = f_close(&_sdFile);
      memset(&_sdFile, 0, sizeof(_sdFile));
    }
  }

  void File::close(void) {
    _result = FR_OK;
    if (_sdFile.fs) {
      _result = f_close(&_sdFile);
      memset(&_sdFile, 0, sizeof(_sdFile));
    }
  }

  UINT File::write(const BYTE* buf_p, UINT len) {
    _result = f_write(&_sdFile, buf_p, len, &len);
    if (_result != FR_OK)
      return len;

    _result = f_sync(&_sdFile);

    return len;
  }

  void File::read(void* buf_p, UINT len, UINT* len_p) {
    _result = f_read(&_sdFile, buf_p, len, len_p);
  }

  FileInfo::FileInfo(const XCHAR* path) {
    memset(&_fileInfo, 0, sizeof(_fileInfo));
    _result = f_stat(path, &_fileInfo);
  }

}; // namespace FAT
