#include "sti_gnss_lib.h"
#include "FatFileSystem.h" 

BOOL withGNSSLib = FALSE;


void setup() {
  char folderName[64] = { 0 };
  const int Size = 512;
  char buf[Size] = { 0 };
  char fileName[64] = { 0 };
  UINT len = 0;
  
#if defined(USE_UART1_FOR_NMEA) && (USE_UART1_FOR_NMEA==1)
  withGNSSLib = TRUE;
#endif    
   // put your setup code here, to run once: 
  GnssConf.init();
  Serial.begin(115200);
  // put your setup code here, to run once:
  if(!withGNSSLib)
  {
    Serial1.config(STGNSS_UART_8BITS_WORD_LENGTH, STGNSS_UART_1STOP_BITS, STGNSS_UART_NOPARITY);
    Serial1.begin();
  }
  
  len = sprintf(buf, "withGNSSLib = %d\r\n", withGNSSLib);
  UartOutput(withGNSSLib, buf, len);
  
  spiMaster.config(0, 10000000, false, false); // mode 0, 5MHz, CS0 and CS1 
  spiMaster.begin(); 
  spiMaster.slaveSelect(0); // use GPIO28 
  
  FAT::FileSystem fatFsAgent;
  fatFsAgent.initialize();
  
  //create a folder
  for(int i=0; i<100; ++i)
  {  
    sprintf(folderName, "TestFolder%03d", i + 1);
    fatFsAgent.make_dir(folderName);
    if (FR_OK == fatFsAgent.result())
      break;
  }
  
  //create a text file and write some lines of message.
  strcpy(fileName, folderName);
  strcat(fileName, "/text_file.txt");
  {
    FAT::File text_file(fileName, FA_WRITE | FA_CREATE_NEW);
    for (int i = 0; i < 20; ++i) {
      sprintf(buf, "NavSpark adapter board text file example line %d\r\n", i + 1);
      len = text_file.write((BYTE*)buf, strlen(buf));
    }
    text_file.close();
  }
  
  //create a binary file and write sequential values.
  strcpy(fileName, folderName);
  strcat(fileName, "/bin_file.bin");
  {
    FAT::File bin_file(fileName, FA_WRITE | FA_CREATE_NEW);
    for (int i = 0; i < Size; ++i) {
      buf[i % Size] = (BYTE)i;
    }
    len = Size;
    len = bin_file.write((BYTE*)buf, len);
    bin_file.close();
    len = Size;
  }

  //read a text file to Serial
  strcpy(fileName, folderName);
  strcat(fileName, "/text_file.txt");
  {
    FAT::File text_file(fileName, FA_READ);

    UINT  readSize = 64;
    len = readSize;
    do {
      text_file.read(buf, len, &len);
      buf[len] = 0;  //Add string null terminator
      UartOutput(withGNSSLib, buf, len);
    } while(len == readSize);
    text_file.close();
  }
  
  //read a binary file to Serial
  strcpy(fileName, folderName);
  strcat(fileName, "/bin_file.bin");
  {
    FAT::File bin_file(fileName, FA_READ);
    FAT::FileInfo bin_info(fileName);
    DWORD fileSize = bin_info.size();
    len = sprintf(buf, "open %s, size : %ld\r\n", fileName, fileSize);
    UartOutput(withGNSSLib, buf, len);
    //Read first 8 bytes and dump them to uart.
    UINT readSize = 8;
    len = readSize;
    bin_file.read(buf, len, &len);
    bin_file.close();
  }
  
  len = sprintf(fileName, "Data : %02X %02X %02X %02X %02X %02X %02X %02X \r\n", 
      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
  UartOutput(withGNSSLib, fileName, len);
}

void UartOutput(BOOL withGNSSLib, char* buf, int len)
{
   if(withGNSSLib)
    gnss_uart_putline(0, (U08*)buf, len);  
  else
    Serial1.print(buf); 
}

void loop() {
  // put your main code here, to run repeatedly:
}

void task_called_after_GNSS_update(void) 
{

}
