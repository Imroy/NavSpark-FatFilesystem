#include "FatFileSystem.h"

bool withGNSSLib = FALSE;

FAT::FileSystem fatFsAgent;
uint8_t current_day = 0;
uint8_t current_hour = 255;

int led_cycle_time = -1, led_on_time = -1;
long int last_cycle;

void change_led(int ct, int ot) {
  if ((ct != led_cycle_time) || (ot != led_on_time))
    last_cycle = int((double)millis() / ct) * ct;
  led_cycle_time = ct;
  led_on_time = ot;
}

void setup() {
  const int Size = 512;
  char buf[Size] = { 0 };
  UINT len = 0;

#if defined(USE_UART1_FOR_NMEA) && (USE_UART1_FOR_NMEA==1)
  withGNSSLib = TRUE;
#endif
   // put your setup code here, to run once:
  GnssConf.setUpdateRate(STGNSS_POSITION_UPDATE_RATE_10HZ);
  GnssConf.init();
  Serial.begin(115200);
  pinMode(GPIO0_LED, OUTPUT);
  change_led(2000, 100);

  // put your setup code here, to run once:
  if (!withGNSSLib) {
    Serial1.config(STGNSS_UART_8BITS_WORD_LENGTH, STGNSS_UART_1STOP_BITS, STGNSS_UART_NOPARITY);
    Serial1.begin();
  }

  len = sprintf(buf, "withGNSSLib = %d\r\n", withGNSSLib);
  UartOutput(buf, len);

  spiMaster.config(0, 10000000, false, false); // mode 0, 5MHz, CS0 and CS1
  spiMaster.begin();
  spiMaster.slaveSelect(0); // use GPIO28

  fatFsAgent.initialize();
}

void UartOutput(char* buf, int len) {
   if (withGNSSLib)
    gnss_uart_putline(0, (U08*)buf, len);
  else
    Serial1.print(buf);
}

void UartOutput(char* buf) {
  UartOutput(buf, strlen(buf));
}

void loop() {
  // put your main code here, to run repeatedly:
  long int now = millis();
  while (now - last_cycle >= led_cycle_time)
    last_cycle += led_cycle_time;

  if (now - last_cycle < led_on_time)
    digitalWrite(GPIO0_LED, HIGH);
  else
    digitalWrite(GPIO0_LED, LOW);
}

void task_called_after_GNSS_update(void) {
  GnssInfo.update();
  if (!GnssInfo.isUpdated())
    return;

  switch (GnssInfo.fixMode()) {
  case 0:  // no fix
    change_led(2000, 100);
    break;

  case 1:  // prediction
    change_led(2000, 1000);
    break;

  case 2:  // 2D fix
    change_led(1000, 500);
    break;

  case 3:  // 3D fix
    change_led(500, 250);
    break;

  case 4:  // differential
    change_led(250, 125);
    break;
  }

  if (GnssInfo.fixMode() != 0) {
    char buf[100] = { 0 };

    TCHAR dir_name[32] = { 0 };
    sprintf(dir_name, "%04d-%02d-%02d", GnssInfo.date.year(), GnssInfo.date.month(), GnssInfo.date.day());
    if (GnssInfo.date.day() != current_day) {
      fatFsAgent.make_dir(dir_name);

      current_day = GnssInfo.date.day();
    }

    TCHAR file_name[32] = { 0 };
    sprintf(file_name, "%02d.txt", GnssInfo.time.hour());

    TCHAR file_path[32] = { 0 };
    strcpy(file_path, dir_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);

    FAT::File text_file(file_path, FA_WRITE | FA_OPEN_ALWAYS);

    FAT::FileInfo text_info(file_path);
    DWORD file_size = text_info.size();
    text_file.lseek(file_size);

    char lat[32], lon[32];
    GnssInfo.location.latitude_formatString(lat);
    GnssInfo.location.longitude_formatString(lon);
    sprintf(buf, "%02d:%02d:%02d.%02d %s %s %f m %f km/h\n",
	    GnssInfo.time.hour(), GnssInfo.time.minute(), GnssInfo.time.second(), GnssInfo.time.centisecond(),
	    lat, lon,
	    GnssInfo.altitude.meters(), GnssInfo.speed.kph());
    text_file.write((BYTE*)buf, strlen(buf));
    text_file.close();
  }
}
