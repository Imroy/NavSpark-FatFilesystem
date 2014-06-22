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
    _result = f_mount(NULL, "", 0);
  }

  void FileSystem::initialize() {
    if (_mmcInit)
      return;

    _result = f_mount(&_fatFs, "", 0);
    if (_result == FR_OK)
      _mmcInit = true;
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



  File::File(void) {
    memset(&_File, 0, sizeof(_File));
  }

  File::File(const TCHAR *path, BYTE mode) {
    memset(&_File, 0, sizeof(_File));
    _result = f_open(&_File, path, mode);
  }

  File::~File() {
    _result = FR_OK;
    if (_File.fs)
      _result = f_close(&_File);
  }

  bool File::exists(const TCHAR *path) {
    FileInfo info(path);
    if (info.result() == FR_OK)
      return true;

    return false;
  }

  DWORD File::size(const TCHAR* path) {
    FileInfo info(path);
    if (info.result() == FR_OK)
      return info.size();
    return 0;
  }

  void File::open(const TCHAR *path, BYTE mode) {
    if (_File.fs) {
      _result = f_close(&_File);
      if (_result != FR_OK)
	return;
      memset(&_File, 0, sizeof(_File));
    }

    _result = f_open(&_File, path, mode);
  }

  void File::close(void) {
    _result = FR_OK;
    if (_File.fs) {
      _result = f_close(&_File);
      memset(&_File, 0, sizeof(_File));
    }
  }

  bool File::eof(void) {
    return f_eof(&_File);
  }

  DWORD File::size(void) {
    return f_size(&_File);
  }

  void File::lseek(DWORD ofs) {
    _result = f_lseek(&_File, ofs);
  }

  DWORD File::tell(void) {
    return f_tell(&_File);
  }

  UINT File::write(const BYTE* buf_p, UINT len) {
    _result = f_write(&_File, buf_p, len, &len);
    if (_result != FR_OK)
      return len;

    _result = f_sync(&_File);

    return len;
  }

  void File::read(void* buf_p, UINT len, UINT* len_p) {
    _result = f_read(&_File, buf_p, len, len_p);
  }

  void File::truncate(void) {
    _result = f_truncate(&_File);
  }

  void File::sync(void) {
    _result = f_sync(&_File);
  }



  FileInfo::FileInfo() {
    memset(&_fileInfo, 0, sizeof(_fileInfo));
  }

  FileInfo::FileInfo(const TCHAR* path) {
    memset(&_fileInfo, 0, sizeof(_fileInfo));
    _result = f_stat(path, &_fileInfo);
  }



  Directory::Directory(void) {
    memset(&_Dir, 0, sizeof(_Dir));
  }

  Directory::Directory(const TCHAR *path) {
    memset(&_Dir, 0, sizeof(_Dir));
    _result = f_opendir(&_Dir, path);
  }

  Directory::~Directory() {
    _result = FR_OK;
    if (_Dir.fs)
      _result = f_closedir(&_Dir);
  }

  void Directory::open(const TCHAR *path) {
    if (_Dir.fs) {
      _result = f_closedir(&_Dir);
      if (_result != FR_OK)
	return;
      memset(&_Dir, 0, sizeof(_Dir));
    }

    _result = f_opendir(&_Dir, path);
  }

  void Directory::close(void) {
    _result = FR_OK;
    if (_Dir.fs) {
      _result = f_closedir(&_Dir);
      memset(&_Dir, 0, sizeof(_Dir));
    }
  }

  FileInfo* Directory::next_entry(void) {
    FileInfo *fi = new FileInfo;
    _result = f_readdir(&_Dir, &(fi->_fileInfo));
    if (_result == FR_OK)
      return fi;

    delete fi;
    return NULL;
  }


}; // namespace FAT
