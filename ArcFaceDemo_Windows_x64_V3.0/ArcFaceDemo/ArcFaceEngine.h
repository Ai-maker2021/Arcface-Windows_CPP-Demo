#pragma once

#include "stdafx.h"
#include "merror.h"
#include <opencv2\opencv.hpp>
#include <vector>

//#define PRO

#ifdef PRO
#include "pro/arcsoft_face_sdk.h"
#else
#include "free/arcsoft_face_sdk.h"
#endif


class ArcFaceEngine
{
public:
	ArcFaceEngine();
	~ArcFaceEngine();

	//����
	MRESULT ActiveSDK(char* appID, char* sdkKey, char* activeKey);
	//��ȡ�����ļ���Ϣ�����Ի�ȡ����Ч�ڣ�
	MRESULT GetActiveFileInfo(ASF_ActiveFileInfo& activeFileInfo);
	//��ʼ������
	MRESULT InitEngine(ASF_DetectMode mode);	
	//�ͷ�����
	MRESULT UnInitEngine();	
	//�������
	MRESULT PreDetectFace(IplImage* image, ASF_SingleFaceInfo& faceRect, bool isRGB);
	//����������ȡ
	MRESULT PreExtractFeature(IplImage* image, ASF_FaceFeature& feature, ASF_SingleFaceInfo& faceRect);
	//�����ȶ�
	MRESULT FacePairMatching(MFloat &confidenceLevel, ASF_FaceFeature feature1, ASF_FaceFeature feature2, 
		ASF_CompareModel compareModel = ASF_LIFE_PHOTO);
	//���û�����ֵ
	MRESULT SetLivenessThreshold(MFloat	rgbLiveThreshold, MFloat irLiveThreshold);
	//RGBͼ���������Լ��
	MRESULT FaceASFProcess(ASF_MultiFaceInfo detectedFaces, IplImage *img, ASF_AgeInfo &ageInfo,
		ASF_GenderInfo &genderInfo, ASF_Face3DAngle &angleInfo, ASF_LivenessInfo& liveNessInfo);
	//IR������
	MRESULT FaceASFProcess_IR(ASF_MultiFaceInfo detectedFaces, IplImage *img, ASF_LivenessInfo& irLiveNessInfo);
	//��ȡ�汾��Ϣ
	const ASF_VERSION GetVersion();
	


private:
	MHandle m_hEngine;
};

//ͼƬ�ü�
void PicCutOut(IplImage* src, IplImage* dst, int x, int y);
//��ɫ�ռ�ת��
int ColorSpaceConversion(IplImage* image, MInt32 format, ASVLOFFSCREEN& offscreen);