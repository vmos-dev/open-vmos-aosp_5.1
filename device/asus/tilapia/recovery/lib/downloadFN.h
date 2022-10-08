#ifndef _DOWNLOADFN_H
#define _DOWNLOADFN_H

void SetBaudrate(int baudrate);
void SetComPort(char *comport);
void SetTraceFile(char *filepath);
void SetEraseMode(int value);
void SetHighPerformance();
void ResetModem();
int DownloadFiles(char* file);

#endif
