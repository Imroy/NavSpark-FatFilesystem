#include "FatFileSystem.h"
#include "HardwareSerial.h"
#include <stdio.h>
#include <string.h>
#include "Arduino.h" 

FatFileSystem::FatFileSystem()
{
  mmcInit = false;  
  memset(&sdFile, 0, sizeof(sdFile));
  memset(&fileInfo, 0, sizeof(fileInfo));
  
}

FatFileSystem::~FatFileSystem()
{
  
}
 
FRESULT FatFileSystem::initialize()
{
  if (mmcInit)
    return FR_OK;

  FRESULT res = f_mount(0, &fatFs);
  close_file();
  return res;
}

void FatFileSystem::close_file()
{
  if(!sdFile.fs)
  {
    return;
  }  
  f_close(&sdFile);
  memset(&sdFile, 0, sizeof(sdFile));
}

FRESULT FatFileSystem::make_dir(const XCHAR *path)
{
  FRESULT res = f_mkdir(path);
  return res; 
}

FRESULT FatFileSystem::open_file(const XCHAR *path, BYTE mode)
{
  if(sdFile.fs)
  {
    f_close(&sdFile);
    memset(&sdFile, 0, sizeof(sdFile));
  }
  
  strcpy(file_name, path);
  FRESULT res = f_open(&sdFile, file_name, mode);
  if(res==FR_OK)
  {
    f_stat(path, &fileInfo);
  }

  return res; 
}

UINT FatFileSystem::write_file(const BYTE* buf_p, UINT len)
{
  FRESULT res = f_write(&sdFile, buf_p, len, &len);
  res = f_sync(&sdFile);
  return len;
}

FRESULT FatFileSystem::file_read (void* buf_p, UINT len, UINT* len_p)
{
  return f_read(&sdFile, buf_p, len, len_p);
}
