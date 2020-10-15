执行要求：
1.将bulk3.conf文件放在可执行文件同级目录
2.将names文件，cfg文件,weights文件以及rtsp流的绝对路径按照
...../***.names
...../***.cfg
...../***.weights
rtsp://...........
顺序进行放置，如
/home/wang/documents/yoloDrums/drums.names
/home/wang/documents/yoloDrums/yolov4-tiny.cfg
/home/wang/documents/yoloDrums/backup_tiny/yolov4-tiny_best.weights
rtsp://admin:yltx8888@192.168.37.210:554/h264/ch1/main/av_stream
3.依赖动态库：
libtest.so
4.头文件：
interface.h

执行方式：
./drumDetectorProj-3 /home/wang/documents/cppProjects/drumDetectorProj-3/Release/drum.conf /home/wang/videoTest/


