//
// Created by Administrator on 2017/5/13.
//
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <android/log.h>

using namespace cv;
using namespace std;

extern "C" {

#define TAG "myDemo-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

//将整幅图中的红色分量都提取出来
Mat GetRedComponetBySplit(Mat srcImg)
{
    Mat imgROI;
    vector<Mat>channels;
    split(srcImg, channels);
    Mat blueComponet = channels.at(0);
    Mat greenComponet = channels.at(1) ;
    blueComponet = Mat::zeros(srcImg.size(), CV_8UC1);//Mat相当于指针，会对chnnels.at(0)重新赋值
    greenComponet = Mat::zeros(srcImg.size(), CV_8UC1);
    merge(channels, imgROI);//仅仅保留红色分量，其他分量赋值为0
    return imgROI;
}


//该方法可能产生误检点，需要使用形态学后处理
Mat GetRedComponet(Mat srcImg)
{
    //如果直接对srcImg处理会改变main()函数中的实参
    Mat dstImg = srcImg.clone();
    Mat_<Vec3b>::iterator it = dstImg.begin<Vec3b>();
    Mat_<Vec3b>::iterator itend = dstImg.end<Vec3b>();
    for(; it != itend; it++)
    {
        if((*it)[2] > 190)//对红色分量做阈值处理
        {
            (*it)[0] = 0;
            (*it)[1] = 0;
            (*it)[2] = 255;//红色分量保持不变
        }

        else
        {
            (*it)[0] = 0;
            (*it)[1] = 0;
            (*it)[2] = 0;
        }
    }
//    imshow("红色分量图 by 阈值法", dstImg);
//    waitKey(0);
    return dstImg;

}

JNIEXPORT jintArray JNICALL Java_com_jt28_a6735_android_1opencv_1ndk_OpenCVHelper_gray
        (JNIEnv *env, jclass obj, jintArray buf, int w, int h) {

    jint *cbuf;
    cbuf = env->GetIntArrayElements(buf, JNI_FALSE);
    if (cbuf == NULL) {
        return 0;
    }
    //jintArray 转 Mat
    Mat imgOriginal(h, w, CV_8UC4, (unsigned char *) cbuf);

    cv::Mat hsvImg;				// HSV Image
    cv::Mat threshImg;			// Thresh Image

    std::vector<cv::Vec3f> v3fCircles;		// 3 element vector of floats, this will be the pass by reference output of HoughCircles()

    char charCheckForEscKey = 0;

    int lowH = 2;							// Set Hue
    int highH = 22;

    int lowS = 98;							// Set Saturation
    int highS = 171;

    int lowV = 196;							// Set Value
    int highV = 255;

    Mat g_srcImage,dstImage;
    std::vector<Mat> g_vChannels;

    g_srcImage = imgOriginal;
    //分离通道
    split(g_srcImage,g_vChannels);
    Mat imageBlueChannel = g_vChannels.at(0);
    Mat imageGreenChannel = g_vChannels.at(1);
    Mat imageRedChannel = g_vChannels.at(2);

    double imageBlueChannelAvg=0;
    double imageGreenChannelAvg=0;
    double imageRedChannelAvg=0;

    //求各通道的平均值
    imageBlueChannelAvg = mean(imageBlueChannel)[0];
    imageGreenChannelAvg = mean(imageGreenChannel)[0];
    imageRedChannelAvg = mean(imageRedChannel)[0];

    //求出个通道所占增益
    double K = (imageRedChannelAvg+imageGreenChannelAvg+imageRedChannelAvg)/3;
    double Kb = K/imageBlueChannelAvg;
    double Kg = K/imageGreenChannelAvg;
    double Kr = K/imageRedChannelAvg;

    //更新白平衡后的各通道BGR值
    addWeighted(imageBlueChannel,Kb,0,0,0,imageBlueChannel);
    addWeighted(imageGreenChannel,Kg,0,0,0,imageGreenChannel);
    addWeighted(imageRedChannel,Kr,0,0,0,imageRedChannel);

    merge(g_vChannels,dstImage);//图像各通道合并

    imgOriginal = dstImage;

    cv::cvtColor(imgOriginal, hsvImg, CV_BGR2HSV);						// Convert Original Image to HSV Thresh Image

    cv::inRange(hsvImg, cv::Scalar(lowH, lowS, lowV), cv::Scalar(highH, highS, highV), threshImg);

    cv::GaussianBlur(threshImg, threshImg, cv::Size(3, 3), 0);			//Blur Effect
    cv::dilate(threshImg, threshImg, 0);								// Dilate Filter Effect
    cv::erode(threshImg, threshImg, 0);									// Erode Filter Effect

    // fill circles vector with all circles in processed image
    cv::HoughCircles(threshImg,v3fCircles,CV_HOUGH_GRADIENT,2,threshImg.rows / 4,100,50,10,800);  // algorithm for detecting circles

    for (int i = 0; i < v3fCircles.size(); i++) {						// for each circle

//        std::cout << "Ball position X = "<< v3fCircles[i][0]			// x position of center point of circle
//                  <<",\tY = "<< v3fCircles[i][1]								// y position of center point of circle
//                  <<",\tRadius = "<< v3fCircles[i][2]<< "\n";					// radius of circle

        // draw small green circle at center of object detected
        cv::circle(imgOriginal,												// draw on original image
                   cv::Point((int)v3fCircles[i][0], (int)v3fCircles[i][1]),		// center point of circle
                   3,																// radius of circle in pixels
                   cv::Scalar(0, 255, 0),											// draw green
                   CV_FILLED);														// thickness

        // draw red circle around object detected
        cv::circle(imgOriginal,												// draw on original image
                   cv::Point((int)v3fCircles[i][0], (int)v3fCircles[i][1]),		// center point of circle
                   (int)v3fCircles[i][2],											// radius of circle in pixels
                   cv::Scalar(0, 0, 255),											// draw red
                   3);																// thickness
    }

    //mat 转换 jint
    jint* ptr = imgOriginal.ptr<jint>(0);
    int size = w * h;
    jintArray result = env->NewIntArray(size);
    env->SetIntArrayRegion(result, 0, size, ptr);
    env->ReleaseIntArrayElements(buf, cbuf, 0);
    return result;
}

JNIEXPORT jintArray JNICALL Java_com_jt28_a6735_android_1opencv_1ndk_OpenCVHelper_roi_1add
        (JNIEnv *env, jclass obj, jintArray buf, jintArray buf2,int w, int h,int l_w, int l_h) {
    jint *cbuf;
    cbuf = env->GetIntArrayElements(buf, JNI_FALSE);
    if (cbuf == NULL) {
        return 0;
    }
    //jintArray 转 Mat
    Mat srcImage(h, w, CV_8UC4, (unsigned char *) cbuf);

    cbuf = env->GetIntArrayElements(buf2, JNI_FALSE);
    if (cbuf == NULL) {
        return 0;
    }
    //jintArray 转 Mat
    Mat logoImage(l_h, l_w, CV_8UC4, (unsigned char *) cbuf);

    cv::Mat imgOriginal;		// Input image
    cv::Mat hsvImg;				// HSV Image
    cv::Mat threshImg;			// Thresh Image

    std::vector<cv::Vec3f> v3fCircles;		// 3 element vector of floats, this will be the pass by reference output of HoughCircles()

    char charCheckForEscKey = 0;

    int lowH = 2;							// Set Hue
    int highH = 22;

    int lowS = 98;							// Set Saturation
    int highS = 171;

    int lowV = 196;							// Set Value
    int highV = 255;

    imgOriginal = logoImage;

    Mat g_srcImage,dstImage;
    std::vector<Mat> g_vChannels;

    g_srcImage = imgOriginal;
    //分离通道
    split(g_srcImage,g_vChannels);
    Mat imageBlueChannel = g_vChannels.at(0);
    Mat imageGreenChannel = g_vChannels.at(1);
    Mat imageRedChannel = g_vChannels.at(2);

    double imageBlueChannelAvg=0;
    double imageGreenChannelAvg=0;
    double imageRedChannelAvg=0;

    //求各通道的平均值
    imageBlueChannelAvg = mean(imageBlueChannel)[0];
    imageGreenChannelAvg = mean(imageGreenChannel)[0];
    imageRedChannelAvg = mean(imageRedChannel)[0];

    //求出个通道所占增益
    double K = (imageRedChannelAvg+imageGreenChannelAvg+imageRedChannelAvg)/3;
    double Kb = K/imageBlueChannelAvg;
    double Kg = K/imageGreenChannelAvg;
    double Kr = K/imageRedChannelAvg;

    //更新白平衡后的各通道BGR值
    addWeighted(imageBlueChannel,Kb,0,0,0,imageBlueChannel);
    addWeighted(imageGreenChannel,Kg,0,0,0,imageGreenChannel);
    addWeighted(imageRedChannel,Kr,0,0,0,imageRedChannel);

    merge(g_vChannels,dstImage);//图像各通道合并

    imgOriginal = dstImage;

    cv::cvtColor(imgOriginal, hsvImg, CV_BGR2HSV);						// Convert Original Image to HSV Thresh Image

    cv::inRange(hsvImg, cv::Scalar(lowH, lowS, lowV), cv::Scalar(highH, highS, highV), threshImg);

    cv::GaussianBlur(threshImg, threshImg, cv::Size(3, 3), 0);			//Blur Effect
    cv::dilate(threshImg, threshImg, 0);								// Dilate Filter Effect
    cv::erode(threshImg, threshImg, 0);									// Erode Filter Effect

    // fill circles vector with all circles in processed image
    cv::HoughCircles(threshImg,v3fCircles,CV_HOUGH_GRADIENT,2,threshImg.rows / 4,100,50,10,800);  // algorithm for detecting circles

    for (int i = 0; i < v3fCircles.size(); i++) {						// for each circle

//        std::cout << "Ball position X = "<< v3fCircles[i][0]			// x position of center point of circle
//                  <<",\tY = "<< v3fCircles[i][1]								// y position of center point of circle
//                  <<",\tRadius = "<< v3fCircles[i][2]<< "\n";					// radius of circle

        // draw small green circle at center of object detected
        cv::circle(imgOriginal,												// draw on original image
                   cv::Point((int)v3fCircles[i][0], (int)v3fCircles[i][1]),		// center point of circle
                   3,																// radius of circle in pixels
                   cv::Scalar(0, 255, 0),											// draw green
                   CV_FILLED);														// thickness

        // draw red circle around object detected
        cv::circle(imgOriginal,												// draw on original image
                   cv::Point((int)v3fCircles[i][0], (int)v3fCircles[i][1]),		// center point of circle
                   (int)v3fCircles[i][2],											// radius of circle in pixels
                   cv::Scalar(0, 0, 255),											// draw red
                   3);																// thickness
    }

    //【4】显示结果
    //mat 转换 jint
    jint* ptr = threshImg.ptr<jint>(0);
    int size = w * h;
    jintArray result = env->NewIntArray(size);
    env->SetIntArrayRegion(result, 0, size, ptr);
    env->ReleaseIntArrayElements(buf, cbuf, 0);
    return result;
}

}
