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

	QPixmap m_pixRgbImage;			//��Ƶģʽ��Ԥ������
	QPixmap m_pixStaticRgbImage;	//��̬ͼԤ������

	//QTextCodec *m_textCodec;

	int m_lastFaceId;
	bool m_isLive;
	bool m_isCompareSuccessed;

	QString m_cameraRegisterImagePath;	//����·��
	bool m_isCapture;					//�Ƿ�Ϊ����ע��
	bool m_isRegisterThreadRunning;		//ע���߳��Ƿ�����ִ��

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

	//�̺߳���
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
	void operationCamera();				//��������ͷ
	bool saveCameraRegisterImage();		//���ղ�����
	void cameraRegisterSingleFace();	//����ע�ᵥ������
	void registerSingleFace();			//ע�ᵥ������
	void registerFaceDatebase();		//ע��������
	void clearFaceDatebase();			//���������
	void selectRecognitionImage();		//ѡ��ʶ����
	void faceCompare();					//�����ȶ�

	void switchCamera();				//�л�����ͷ
	void controlLiveness();				//���ػ�����
	void controlImageQuality();			//����ͼ���������
	void selectCompareModel();			//ѡ��ȶ�ģʽ



private slots:
	void drawListWigetItem(int nIndex, QPixmap pix);		//��������������ͼ

signals:
	void curListWigetItem(int nIndex, QPixmap pix);			//�����ⵥԪ���ź�
	
};



