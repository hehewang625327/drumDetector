#include <list>
#include <string>
#include <iostream>
#include <unistd.h>
#include <QDebug>
#include <fstream>
#include <QDateTime>
#include <stdint.h>
#include "interface.h"

#ifdef linux
#define OPENCV
#define GPU
#endif

#define PROB 0.9
#define FRAMECNT 300

#include "yolo_v2_class.hpp"
#include "opencv2/opencv.hpp"
#include <opencv2/highgui/highgui.hpp>

std::list<cv::Mat> framesDrums;
cv::Size size=cv::Size(640,360);
bool drumsAppear=false;
int cnt=0;
int drumsAppearCnt=0;
bool netSendFlag=false;

void draw_boxes(cv::Mat mat_img, std::vector<bbox_t> result_vec, std::vector<std::string> obj_names){
    int const colors[6][3] = { { 1,0,1 },{ 0,0,1 },{ 0,1,1 },{ 0,1,0 },{ 1,1,0 },{ 1,0,0 } };
    for (auto &i : result_vec) {
        cv::Scalar color = obj_id_to_color(i.obj_id);
        cv::rectangle(mat_img, cv::Rect(i.x, i.y, i.w, i.h), color, 2);
        if (obj_names.size() > i.obj_id) {
            std::string obj_name = obj_names[i.obj_id];
//            QString q_prob=QString::number(i.prob,'f',3);
//            std::string prob=q_prob.toStdString();
//            std::string name_prob=obj_name+" "+prob;
            if (i.track_id > 0) obj_name += " - " + std::to_string(i.track_id);
            cv::Size const text_size = getTextSize(obj_name, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
//            cv::Size const text_size = getTextSize(name_prob, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
            int const max_width = (text_size.width > i.w + 2) ? text_size.width : (i.w + 2);
            cv::rectangle(mat_img, cv::Point2f(std::max((int)i.x - 1, 0), std::max((int)i.y - 30, 0)),
                cv::Point2f(std::min((int)i.x + max_width, mat_img.cols - 1), std::min((int)i.y, mat_img.rows - 1)),
                color, CV_FILLED, 8, 0);
            putText(mat_img, obj_name, cv::Point2f(i.x, i.y - 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
//            putText(mat_img, name_prob, cv::Point2f(i.x, i.y - 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
        }
    }
}

std::vector<std::string> objects_names_from_file(std::string const filename) {
    std::ifstream file(filename);
    std::vector<std::string> file_lines;
    if (!file.is_open()) return file_lines;
    for (std::string line; getline(file, line);) file_lines.push_back(line);
    std::cout << "object names loaded!\n";
    return file_lines;
}

void drumsConfirm(const std::vector<bbox_t>& result_vec,std::vector<bbox_t>& result){
    //std::vector<bbox_t> result_kalman;
    //track_kalman_t track_kalman;
    if(result_vec.size()>0){
        for(std::size_t i=0;i<result_vec.size();i++){
            if(result_vec[i].prob>PROB){
                result.push_back(result_vec[i]);

            }
        }
//    result_kalman=track_kalman.correct(result);
    }
//    else{
//        result=track_kalman.predict();
//    }
}

void videoWriter(std::string & videoSavePath,std::string &picSavePath,std::string savePath){
    QDateTime time=QDateTime::currentDateTime();
    std::string baseName=time.toString("yyyy-MM-dd_hh-mm-ss").toStdString();

    picSavePath=savePath+baseName+".png";
    cv::imwrite(picSavePath,framesDrums.front());  //图片保存

    videoSavePath=savePath+baseName+".avi";   //视频保存
    cv::VideoWriter writer;
    cv::Mat frame;
    writer.open(videoSavePath,CV_FOURCC('D','I','V','X'),25.0,size);
    std::list<cv::Mat>::iterator iter;
//    int writerFrame=0;
    for(iter=framesDrums.begin();iter!=framesDrums.end();iter++){
       writer<<*iter;
//       writerFrame++;
//       std::cout<<"writer frame: "<<writerFrame<<std::endl;
    }
    std::cout<<"video writer!\n";
    writer.release();
    framesDrums.clear();
//    writerFrame=0;

}

void frameCapture(Detector detector,std::vector<std::string> obj_names,const std::string videoSrc,const std::string savePath){
    cv::VideoCapture cap(videoSrc);
    if(!cap.isOpened()){
        std::cout<<"video open failed!"<<std::endl;
    }

    while(true){
        cv::Mat frame;
        cap>>frame;
        cv::resize(frame,frame,size);//视频帧进行缩放
        if(frame.empty()){
            std::cout<<"video frame capture failed!\n";
            continue;
        }

        std::vector<bbox_t> result_vec=detector.detect(frame);
        std::vector<bbox_t> result;
        drumsConfirm(result_vec,result);
        draw_boxes(frame,result,obj_names);
        std::string videoSavePath;
        std::string uuid;
        std::string picSavePath;
        if(result.size()>0){
            drumsAppearCnt++;
        }
        if(drumsAppearCnt>5){
            drumsAppear=true;
        }
        if(drumsAppear){
            if(cnt<FRAMECNT){
                framesDrums.push_back(frame);
                cnt++;
//                std::cout<<"cnt:"<<cnt<<std::endl;
            }
        }

        if(framesDrums.size()==FRAMECNT){
            videoWriter(videoSavePath,picSavePath,savePath);
            if(PushBulkEvent(videoSavePath.c_str(),picSavePath.c_str())){
                std::cout<<"event sends successfully!\n";
            }
            netSendFlag=true;
            drumsAppearCnt=0;
            drumsAppear=false;
            cnt=0;
        }

        cv::imshow("frame",frame);
        frame.release();
        cv::waitKey(3);
    }
    cap.release();
}

int main(int argc,char* argv[]){
    std::string  names_file,cfg_file, weights_file,rtsp_or_video_addr;
    std::string confFilePath=argv[1];   //配置文件  存放names,cfg,weights,视频源
    std::string savePath=argv[2];       //视频，图片的保存地址
    std::ifstream file;
    file.open(confFilePath,std::ios::in);
    file>>names_file;
    file>>cfg_file;
    file>>weights_file;
    file>>rtsp_or_video_addr;
    Detector detector(cfg_file, weights_file);//初始化检测器
    detector.nms=0.02;   //0.42
    std::vector<std::string> obj_names = objects_names_from_file(names_file);
    //std::string baConfigPath=Tool::FixFilePath(configPath);
    StartBulkSystem();

    frameCapture(detector,obj_names,rtsp_or_video_addr,savePath);

    StopBulkSystem();
    std::cout<<"video recording finished!\n";
    return 0;
}
