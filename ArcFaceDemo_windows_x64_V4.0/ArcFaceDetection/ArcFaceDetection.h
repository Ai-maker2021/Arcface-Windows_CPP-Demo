#pragma once
#include <QtWidgets/QMainWindow>
#include "ui_ArcFaceDetection.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "Utils.h"
#include "FaceEngine.h"
#include "IrPreviewDialog.h"
#include <QCloseEvent>
//#include <QTextCodec>



class ArcFaceDetection : public QMainWindow, public Ui_ArcFaceDetectionClass
	, public Utils
{
	Q_OBJECT

public:
	ArcFaceDetection(QWidget *parent = Q_NULLPTR);
	~ArcFaceDetection();

//private:
//	Ui::ArcFaceDetectionClass ui;

private:
	int m_rgbCameraId;
	int m_irCameraId;
	int m_controlLiveness;
	ASF_CompareModel m_compareModel;
	int m_controlImageQulity;

	vector<string> m_cameraNameList;
	cv::Mat m_rgbFrame;
	cv::VideoCapture m_rgbCapture;
	cv::Mat m_irFrame;
	cv::VideoCapture m_irCapture;

	bool m_isOpenCamera;

	IrPreviewDialog* irPreviewDialog;
	QStringList picPathList;

	FaceEngine m_imageEngine;
	FaceEngine m_videoFtEngine;
	FaceEngine m_imageFrEngine;
	FaceEngine m_imageFlEngine;

	int m_nIndex;
	std::vector<ASF_FaceFeature> m_featuresVec;

	ASF_SingleFaceInfo m_recognitionFaceInfo;

	ASF_FaceFeature m_recognitionImageFeature;
	bool m_recognitionImageSucceed;

	ASF_MultiFaceInfo m_videoFaceInfo;
	bool m_dataValid;
	QString m_strCompareInfo;
	QString m_rgbLivenessInfo;
	QString m_irLivenessInfo;

	QPixmap m_pixRgbImage;			//视频模式下预览数据
	QPixmap m_pixStaticRgbImage;	//静态图预览数据

	//QTextCodec *m_textCodec;

	int m_lastFaceId;
	bool m_isLive;
	bool m_isCompareSuccessed;

	QString m_cameraRegisterImagePath;	//拍照路径
	bool m_isCapture;					//是否为拍照注册
	bool m_isRegisterThreadRunning;		//注册线程是否正在执行

private:
	void initView();
	void initData();
	void setUiStyle();
	void signalConnectSlot();
	void releaseData();
	void openCamera();
	void closeCamera();
	void editOutputLog(QString str);
	void detectionRecognitionImage(IplImage* recognitionImage, ASF_SingleFaceInfo& faceInfo, QString& strInfo);
	void scaleFaceRect(MRECT srcFaceRect, MRECT* dstFaceRect, double nScale);
	void previewIrFaceRect(MRECT srcFaceRect, MRECT* dstFaceRect);
	FqThreshold getFqThreshold();

	//线程函数
	void ftPreviewData();
	void frPreviewData();
	void livenessPreviewData();
	void showRegisterThumbnail();

protected:
	bool m_isPressed;
	QPoint m_movePoint;
	void paintEvent(QPaintEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void closeEvent(QCloseEvent *event);

private slots:
	void operationCamera();				//启动摄像头
	bool saveCameraRegisterImage();		//拍照并保存
	void cameraRegisterSingleFace();	//拍照注册单张人脸
	void registerSingleFace();			//注册单张人脸
	void registerFaceDatebase();		//注册人脸库
	void clearFaceDatebase();			//清除人脸库
	void selectRecognitionImage();		//选择识别照
	void faceCompare();					//人脸比对

	void switchCamera();				//切换摄像头
	void controlLiveness();				//开关活体检测
	void controlImageQuality();			//开关图像质量检测
	void selectCompareModel();			//选择比对模式



private slots:
	void drawListWigetItem(int nIndex, QPixmap pix);		//绘制人脸库缩略图

signals:
	void curListWigetItem(int nIndex, QPixmap pix);			//人脸库单元项信号
	
};



