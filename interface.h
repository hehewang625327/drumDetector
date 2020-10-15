#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

bool StartBulkSystem();    							//初始化行为分析系统
bool PushBulkEvent(const char *videoFilePath, const char *picFilePath);  //开始传输数据

bool StopBulkSystem();							   //停止行为分析系统


#ifdef __cplusplus
}
#endif


#endif

