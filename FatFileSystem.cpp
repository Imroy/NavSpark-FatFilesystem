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

    _result = f_mount(&_fatFs, "", 0);
  }

  void FileSystem::make_dir(const TCHAR *path) {
    _result = f_mkdir(path);
  }

  void FileSystem::unlink(const TCHAR* path) {
    _result = f_unlink(path);
  }

  void FileSystem::rename(const TCHAR* oldpath, const TCHAR* newpath) {
    _result = f_rename(oldpath, newpath);
  }



  File::File(const TCHAR *path, BYTE mode) {
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

  bool File::eof(void) {
    return f_eof(&_sdFile);
  }

  DWORD File::size(void) {
    return f_size(&_sdFile);
  }

  void File::lseek(DWORD ofs) {
    _result = f_lseek(&_sdFile, ofs);
  }

  DWORD File::tell(void) {
    return f_tell(&_sdFile);
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

  FileInfo::FileInfo() {
    memset(&_fileInfo, 0, sizeof(_fileInfo));
  }

  FileInfo::FileInfo(const TCHAR* path) {
    memset(&_fileInfo, 0, sizeof(_fileInfo));
    _result = f_stat(path, &_fileInfo);
  }

  Directory::Directory(const TCHAR *path) {
    memset(&_Dir, 0, sizeof(_Dir));
    _result = f_opendir(&_Dir, path);
  }

  Directory::~Directory() {
    _result = f_closedir(&_Dir);
    memset(&_Dir, 0, sizeof(_Dir));
  }

  void Directory::close(void) {
    _result = f_closedir(&_Dir);
    memset(&_Dir, 0, sizeof(_Dir));
  }

  FileInfo* Directory::next_entry(void) {
    FileInfo *fi = new FileInfo;
    _result = f_readdir(&_Dir, &(fi->_fileInfo));
    if (_result == FR_OK)
      return fi;

    return NULL;
  }


}; // namespace FAT
