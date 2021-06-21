#include "ArcFaceDetection.h"
#include <qDebug>
#include <QMessageBox>
#include <QPainter>
#include <thread>
#include <mutex>
#include <QFileDialog>
#include <QRegExp>
#include <windows.h>
#include <QVBoxLayout>
#include "config.h"
#include <QDateTime>


#define SafeFree(p) { if ((p)) free(p); (p) = NULL; }
#define SafeArrayDelete(p) { if ((p)) delete [] (p); (p) = NULL; } 
#define SafeDelete(p) { if ((p)) delete (p); (p) = NULL; } 

std::mutex g_mtx;

ArcFaceDetection::ArcFaceDetection(QWidget *parent)
	: QMainWindow(parent)
{
	setupUi(this);

	initView();
	initData();
	setUiStyle();
	signalConnectSlot();
}

ArcFaceDetection::~ArcFaceDetection()
{
	
}


void ArcFaceDetection::initView()
{
	//设置取值范围[0,1]，保留两位小数
	QRegExp rx("\\b(0(\\.[0-9][0-9]\\d{0,0})?)|(1(\\.[0][0]\\d{0,0})?)\\b");
	QValidator *validator = new QRegExpValidator(rx);
	compareThreshold->setValidator(validator);
	compareThreshold->setText(COMPARE_THRESHOLD);
	fqRegisterThreshold->setValidator(validator);
	fqRegisterThreshold->setText(FQ_REGISTER_THRESHOLD);
	fqRecognitionMaskThreshold->setValidator(validator);
	fqRecognitionMaskThreshold->setText(FQ_RECOGNITION_MASK_THRESHOLD);
	fqRecognitionNoMaskThreshold->setValidator(validator);
	fqRecognitionNoMaskThreshold->setText(FQ_RECOGNITION_NO_MASK_THRESHOLD);
	irLiveThreshold->setValidator(validator);
	irLiveThreshold->setText(IR_LIVE_THRESHOLD);
	rgbLiveThreshold->setValidator(validator);
	rgbLiveThreshold->setText(RGB_LIVE_THRESHOLD);

	//双目摄像头对齐调整
	QIntValidator *alignmentValidator = new QIntValidator(-200, 200, this);
	cameraHorizontalAlignment->setValidator(alignmentValidator);
	cameraHorizontalAlignment->setText(BINOCULAR_CAMERA_OFFSET_LEFT_RIGHT);
	cameraVerticalAlignment->setValidator(alignmentValidator);
	cameraVerticalAlignment->setText(BINOCULAR_CAMERA_OFFSET_TOP_BOTTOM);
}

void ArcFaceDetection::initData()
{
	m_isPressed = true;
	m_lastFaceId = -1;
	
	m_isCompareSuccessed = false;
	m_nIndex = 1;
	m_isOpenCamera = false;
	m_recognitionImageSucceed = false;
	m_dataValid = false;
	m_strCompareInfo = "";
	m_rgbLivenessInfo = "";
	m_irLivenessInfo = "";

	//摄像头索引不一定是0和1，根据具体情况调整
	m_rgbCameraId = 0;
	m_irCameraId = 1;
	m_compareModel = ASF_LIFE_PHOTO;
	m_controlLiveness = 1;
	m_controlImageQulity = 0;

	m_isCapture = false;
	m_cameraRegisterImagePath = "";
	m_isRegisterThreadRunning = false;

	m_recognitionImageFeature.featureSize = FACE_FEATURE_SIZE;
	m_recognitionImageFeature.feature = (MByte *)malloc(m_recognitionImageFeature.featureSize * sizeof(MByte));

	m_videoFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT) * FACENUM);
	m_videoFaceInfo.faceID = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	m_videoFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	m_videoFaceInfo.faceDataInfoList = (ASF_FaceDataInfo*)malloc(sizeof(ASF_FaceDataInfo) * FACENUM);
	for (int i = 0; i < FACENUM; i++) {
		m_videoFaceInfo.faceDataInfoList[i].data = malloc(FACE_DATA_SIZE);
	}

	m_recognitionFaceInfo.faceDataInfo.data = malloc(FACE_DATA_SIZE);

	m_featuresVec.clear();

	m_cameraNameList.clear();
	listDevices(m_cameraNameList);

	//m_textCodec = QTextCodec::codecForName("GBK");

	char appId[64];
	char sdkKey[64];
	char activeKey[20];
	readSetting(appId, sdkKey, activeKey);
	if (1 == m_controlLiveness) {
		m_isLive = false;
	} else {
		m_isLive = true;
	}

	MRESULT res = m_imageEngine.ActiveSDK(appId, sdkKey, activeKey);
	if (MOK == res)
	{
		editOutputLog(QStringLiteral("SDK激活成功！"));

		MInt32 imageMask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_IMAGEQUALITY | ASF_LIVENESS | ASF_IR_LIVENESS |
			ASF_FACELANDMARK | ASF_FACESHELTER | ASF_MASKDETECT | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE;
		res = m_imageEngine.InitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_ALL_OUT, imageMask);
		editOutputLog(QStringLiteral("IMAGE模式引擎初始化：") + QString::number(res));

		MInt32 videoFtMask = ASF_FACE_DETECT | ASF_FACELANDMARK;
		res = m_videoFtEngine.InitEngine(ASF_DETECT_MODE_VIDEO, ASF_OP_0_ONLY, videoFtMask);
		editOutputLog(QStringLiteral("VIDEO模式Ft线程引擎初始化：") + QString::number(res));

		MInt32 imageFrMask = ASF_FACERECOGNITION | ASF_MASKDETECT | ASF_IMAGEQUALITY;
		res = m_imageFrEngine.InitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_ONLY, imageFrMask);
		editOutputLog(QStringLiteral("IMAGE模式Fr线程引擎初始化：") + QString::number(res));

		MInt32 imageFlMask = ASF_LIVENESS | ASF_IR_LIVENESS | ASF_UPDATE_FACEDATA;
		res = m_imageFlEngine.InitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_ONLY, imageFlMask);
		editOutputLog(QStringLiteral("IMAGE模式Fl线程引擎初始化：") + QString::number(res));
	}	
	else
	{
		editOutputLog(QStringLiteral("SDK激活失败：") + QString::number(res));
	}
}

void ArcFaceDetection::setUiStyle()
{
	setWindowIcon(QIcon(":/Resources/favicon.ico"));
	//setWindowTitle("Arcsoft-ArcFaceDemo c++");
}


void ArcFaceDetection::signalConnectSlot()
{
	connect(btnOperationCamera, &QPushButton::clicked, this, &ArcFaceDetection::operationCamera);
	connect(btnRegister, &QPushButton::clicked, this, &ArcFaceDetection::registerFaceDatebase);
	connect(btnOneRegister, &QPushButton::clicked, this, &ArcFaceDetection::registerSingleFace);
	connect(btnSelectImage, &QPushButton::clicked, this, &ArcFaceDetection::selectRecognitionImage);
	connect(btnClear, &QPushButton::clicked, this, &ArcFaceDetection::clearFaceDatebase);
	connect(btnFaceCompare, &QPushButton::clicked, this, &ArcFaceDetection::faceCompare);
	connect(this, SIGNAL(curListWigetItem(int, QPixmap)), this, SLOT(drawListWigetItem(int, QPixmap)));
	connect(btnSwitchCamera, &QPushButton::clicked, this, &ArcFaceDetection::switchCamera);
	connect(btnControlLiveness, &QPushButton::clicked, this, &ArcFaceDetection::controlLiveness);
	connect(btnControlImageQuality, &QPushButton::clicked, this, &ArcFaceDetection::controlImageQuality);
	connect(btnCompareModel, &QPushButton::clicked, this, &ArcFaceDetection::selectCompareModel);
	connect(btnCameraRegister, &QPushButton::clicked, this, &ArcFaceDetection::cameraRegisterSingleFace);

}

void ArcFaceDetection::releaseData()
{
	SafeFree(m_recognitionImageFeature.feature);
	SafeFree(m_videoFaceInfo.faceRect);
	SafeFree(m_videoFaceInfo.faceID);
	SafeFree(m_videoFaceInfo.faceOrient);
	for (int i = 0; i < FACENUM; i++){
		SafeFree(m_videoFaceInfo.faceDataInfoList[i].data);
	}
	SafeFree(m_videoFaceInfo.faceDataInfoList);
	SafeFree(m_recognitionFaceInfo.faceDataInfo.data);
}

// 摄像头模块
void ArcFaceDetection::openCamera()
{
	if (1 == m_cameraNameList.size())
	{
		if (!m_rgbCapture.isOpened())
		{
			if (m_rgbCapture.open(m_rgbCameraId))
			{
				if (!(m_rgbCapture.set(CV_CAP_PROP_FRAME_WIDTH, VIDEO_FRAME_DEFAULT_WIDTH) &&
					m_rgbCapture.set(CV_CAP_PROP_FRAME_HEIGHT, VIDEO_FRAME_DEFAULT_HEIGHT)))
				{
					QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("RGB摄像头初始化失败！"));
				}
				else
				{
					m_isOpenCamera = true;
				}
			}
			else
			{
				QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("RGB摄像头索引配置不正确！"));
			}
		}
		else
		{
			QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("RGB摄像头被占用！"));
		}
	}
	else if (2 == m_cameraNameList.size())
	{
		if (!m_rgbCapture.isOpened())
		{
			if (m_rgbCapture.open(m_rgbCameraId))
			{
				if (!(m_rgbCapture.set(CV_CAP_PROP_FRAME_WIDTH, VIDEO_FRAME_DEFAULT_WIDTH) &&
					m_rgbCapture.set(CV_CAP_PROP_FRAME_HEIGHT, VIDEO_FRAME_DEFAULT_HEIGHT)))
				{
					QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("RGB摄像头初始化失败！"));
				}
			}
			else
			{
				QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("RGB摄像头被占用！"));
			}
		}
		else
		{
			QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("RGB摄像头被占用！"));
		}

		if (!m_irCapture.isOpened())
		{
			if (m_irCapture.open(m_irCameraId))
			{
				if (!(m_irCapture.set(CV_CAP_PROP_FRAME_WIDTH, VIDEO_FRAME_DEFAULT_WIDTH) &&
					m_irCapture.set(CV_CAP_PROP_FRAME_HEIGHT, VIDEO_FRAME_DEFAULT_HEIGHT)))
				{
					QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("IR摄像头初始化失败！"));
				}
				else
				{
					m_isOpenCamera = true;
				}
			}
			else
			{
				QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("IR摄像头被占用！"));
			}
		}
		else
		{
			QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("IR摄像头被占用！"));
		}
	}
	else
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("摄像头数量不支持！"));
	}
}

void ArcFaceDetection::closeCamera()
{
	m_isOpenCamera = false;
	m_dataValid = false;
	Sleep(1000);
}

void ArcFaceDetection::operationCamera()
{
	if (btnOperationCamera->text() == QStringLiteral("启动摄像头"))
	{
		openCamera();
	}
	else
	{
		closeCamera();
	}
	
	if (m_isOpenCamera)
	{
		btnOperationCamera->setText(QStringLiteral("关闭摄像头"));

		std::thread thFt(&ArcFaceDetection::ftPreviewData, this);
		thFt.detach();

		std::thread thFr(&ArcFaceDetection::frPreviewData, this);
		thFr.detach();

		if (1 == m_controlLiveness)	//是否启用活体
		{
			std::thread thLiveness(&ArcFaceDetection::livenessPreviewData, this);
			thLiveness.detach();
		}
		
		if (2 == m_cameraNameList.size())
		{
			irPreviewDialog = new IrPreviewDialog();
			irPreviewDialog->show();
		}
	}
	else 
	{
		btnOperationCamera->setText(QStringLiteral("启动摄像头"));
		if (2 == m_cameraNameList.size() && irPreviewDialog != nullptr)
		{
			irPreviewDialog->close();
		}
	}
}

//ft预览
void ArcFaceDetection::ftPreviewData()
{
	MRESULT res = MOK;
	ASF_LandMarkInfo headLandMarkInfo = { 0 };
	MRECT* dstFaceRect = (MRECT*)malloc(sizeof(MRECT));

	//定义字体
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_COMPLEX, 0.5, 0.5, 0);
	
	while (m_isOpenCamera)
	{
		Sleep(3);
		//处理RGB摄像头数据
		m_rgbCapture >> m_rgbFrame;
		if (!m_rgbFrame.empty())
		{
			IplImage *rgbImage = NULL;
			{
				std::lock_guard<mutex> lock(g_mtx);
				IplImage iplrgbImage(m_rgbFrame);
				rgbImage = cvCloneImage(&iplrgbImage);
			}

			res = m_videoFtEngine.PreDetectFace(rgbImage, m_videoFaceInfo, true);
			if (MOK == res)
			{
				//口罩版本特有的额头区域检测
				MRESULT resHead = m_videoFtEngine.FaceProcessMaskHead(m_videoFaceInfo, rgbImage, headLandMarkInfo);
				if (res == MOK && headLandMarkInfo.num > 0)
				{
					//画额头区域框
					cvRectangle(rgbImage, cvPoint(headLandMarkInfo.point[0].x, headLandMarkInfo.point[0].y),
						cvPoint(headLandMarkInfo.point[2].x, headLandMarkInfo.point[2].y), cvScalar(255, 0, 0), 2);
				}

				//画人脸框
				cvRectangle(rgbImage, cvPoint(m_videoFaceInfo.faceRect[0].left, m_videoFaceInfo.faceRect[0].top),
					cvPoint(m_videoFaceInfo.faceRect[0].right, m_videoFaceInfo.faceRect[0].bottom), 
					(m_isCompareSuccessed && m_isLive) ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255), 2);

				//画字
				cvPutText(rgbImage, m_strCompareInfo.toStdString().c_str(),
					cvPoint(m_videoFaceInfo.faceRect[0].left, m_videoFaceInfo.faceRect[0].top - 10),
					&font, (m_isCompareSuccessed && m_isLive) ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255));

				cvPutText(rgbImage, m_rgbLivenessInfo.toStdString().c_str(),
					cvPoint(m_videoFaceInfo.faceRect[0].left, m_videoFaceInfo.faceRect[0].top + 20),
					&font, (m_isCompareSuccessed && m_isLive) ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255));

				m_dataValid = true;

			} else {
				m_dataValid = false;
			}

			m_pixRgbImage = QPixmap::fromImage(cvMatToQImage(cv::cvarrToMat(rgbImage)));
			cvReleaseImage(&rgbImage);
		}

		//处理红外摄像头数据
		if (2 == m_cameraNameList.size())
		{
			m_irCapture >> m_irFrame;
			if (!m_irFrame.empty())
			{
				IplImage *destIrImage = NULL;
				{
					std::lock_guard<mutex> lock(g_mtx);
					IplImage iplIrImage(m_irFrame);

					//图像缩放
					CvSize destImgSize;
					destImgSize.width = iplIrImage.width / 2;
					destImgSize.height = iplIrImage.height / 2;
					destIrImage = cvCreateImage(destImgSize, iplIrImage.depth, iplIrImage.nChannels);
					cvResize(&iplIrImage, destIrImage, CV_INTER_AREA);
				}

				if (MOK == res)
				{
					//画框
					scaleFaceRect(m_videoFaceInfo.faceRect[0], dstFaceRect, 1.0);
					previewIrFaceRect(*dstFaceRect, dstFaceRect);	//预览人脸框边界值保护
					scaleFaceRect(*dstFaceRect, dstFaceRect, 0.5);

					cvRectangle(destIrImage, cvPoint(dstFaceRect->left, dstFaceRect->top),
						cvPoint(dstFaceRect->right, dstFaceRect->bottom), 
						(m_isCompareSuccessed && m_isLive) ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255), 1);

					//画字
					cvPutText(destIrImage, m_irLivenessInfo.toStdString().c_str(),
						cvPoint(dstFaceRect->left, dstFaceRect->top - 10), &font, 
						(m_isCompareSuccessed && m_isLive) ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255));
				}

				irPreviewDialog->m_pixIrImage = QPixmap::fromImage(cvMatToQImage(cv::cvarrToMat(destIrImage)));
				cvReleaseImage(&destIrImage);
			}
		}
	}

	SafeFree(dstFaceRect);
	labelPreview->clear();
	m_rgbCapture.release();
	if (2 == m_cameraNameList.size())
	{
		m_irCapture.release();
	}
}

//特征提取、比对
void ArcFaceDetection::frPreviewData()
{
	MRESULT res = MOK;
	ASF_SingleFaceInfo singleFaceInfo = { 0 };
	ASF_FaceFeature faceFeature = { 0 };
	faceFeature.featureSize = FACE_FEATURE_SIZE;
	faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

	ASF_MultiFaceInfo multiFaceInfo = { 0 };
	multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT) * FACENUM);
	multiFaceInfo.faceID = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	multiFaceInfo.faceDataInfoList = (ASF_FaceDataInfo*)malloc(sizeof(ASF_FaceDataInfo) * FACENUM);
	for (int i = 0; i < FACENUM; i++){
		multiFaceInfo.faceDataInfoList[i].data = malloc(FACE_DATA_SIZE);
	}

	ASF_MaskInfo maskInfo = { 0 };
	
	while (m_isOpenCamera)
	{
		if (!m_dataValid)
		{
			continue;
		}

		//与上一张人脸比较若不是同一张人脸，人脸比对和活体检测数据清零
		if (m_lastFaceId != m_videoFaceInfo.faceID[0])
		{
			m_strCompareInfo = "";
			m_isCompareSuccessed = false;

			if (1 == m_controlLiveness)
			{
				m_isLive = false;
			}
			
			m_rgbLivenessInfo = "";
			m_irLivenessInfo = "";
		}

		m_lastFaceId = m_videoFaceInfo.faceID[0];

		if (m_isCompareSuccessed)
		{
			Sleep(3);
			continue;
		}

		IplImage *rgbImage = NULL;
		{
			std::lock_guard<mutex> lock(g_mtx);
			if (m_rgbFrame.empty())
			{
				continue;
			}
			IplImage iplRgbImage(m_rgbFrame);
			rgbImage = cvCloneImage(&iplRgbImage);

			multiFaceInfo.faceNum = 1;
			multiFaceInfo.faceOrient[0] = m_videoFaceInfo.faceOrient[0];
			multiFaceInfo.faceRect[0] = m_videoFaceInfo.faceRect[0];
			multiFaceInfo.faceID[0] = m_videoFaceInfo.faceID[0];
			memset(multiFaceInfo.faceDataInfoList[0].data, 0, FACE_DATA_SIZE);
			memcpy(multiFaceInfo.faceDataInfoList[0].data, m_videoFaceInfo.faceDataInfoList[0].data, FACE_DATA_SIZE);
			multiFaceInfo.faceDataInfoList[0].dataSize = m_videoFaceInfo.faceDataInfoList[0].dataSize;

			singleFaceInfo.faceOrient = multiFaceInfo.faceOrient[0];
			singleFaceInfo.faceRect = multiFaceInfo.faceRect[0];
			singleFaceInfo.faceDataInfo = multiFaceInfo.faceDataInfoList[0];
		}

		//判断口罩
		res = m_imageFrEngine.FaceProcessMask(multiFaceInfo, rgbImage, maskInfo);
		if (res != MOK || maskInfo.num < 1 || maskInfo.maskArray[0] == -1)
		{
			cvReleaseImage(&rgbImage);
			continue;
		}

		//图像质量检测 戴口罩识别阈值 0.29  不带口罩识别阈值 0.49
		if (m_controlImageQulity == 1)
		{
			res = m_imageFrEngine.PreImageQualityDetect(rgbImage, singleFaceInfo,
				ASF_RECOGNITION, maskInfo.maskArray[0], getFqThreshold());
			if (res != MOK)
			{
				cvReleaseImage(&rgbImage);
				continue;
			}
		}

		res = m_imageFrEngine.PreExtractFeature(rgbImage, singleFaceInfo, faceFeature, ASF_RECOGNITION, maskInfo.maskArray[0]);
		if (MOK != res || m_featuresVec.size() <= 0)
		{
			cvReleaseImage(&rgbImage);
			continue;
		}

		int maxIndex = 0, curIndex = 0;
		MFloat maxThreshold = 0.0;
		for (auto regisFeature : m_featuresVec)
		{
			curIndex++;
			MFloat confidenceLevel = 0.0;
			res = m_imageFrEngine.FacePairMatching(confidenceLevel, faceFeature, regisFeature, m_compareModel);
			if (MOK == res && confidenceLevel > maxThreshold)
			{
				maxThreshold = confidenceLevel;
				maxIndex = curIndex;
			}
		}

		if (maxThreshold > compareThreshold->text().toFloat())
		{
			m_isCompareSuccessed = true;
			m_strCompareInfo = QString::number(maxIndex) + QStringLiteral(": ") + QString("%1").arg(maxThreshold);
		}
		else
		{
			m_strCompareInfo = "";
		}
		cvReleaseImage(&rgbImage);
	}
	SafeFree(faceFeature.feature);
	SafeFree(multiFaceInfo.faceOrient);
	SafeFree(multiFaceInfo.faceRect);
	SafeFree(multiFaceInfo.faceID);
	for (int i = 0; i < FACENUM; i++) {
		SafeFree(multiFaceInfo.faceDataInfoList[i].data);
	}
	SafeFree(multiFaceInfo.faceDataInfoList);
}

//活体检测
void ArcFaceDetection::livenessPreviewData()
{
	ASF_MultiFaceInfo multiFaceInfo = { 0 };
	multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT) * FACENUM);
	multiFaceInfo.faceID = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	multiFaceInfo.faceDataInfoList = (ASF_FaceDataInfo*)malloc(sizeof(ASF_FaceDataInfo) * (FACENUM));
	for (int i = 0; i < FACENUM; i++) {
		multiFaceInfo.faceDataInfoList[i].data = malloc(FACE_DATA_SIZE);
	}

	while (m_isOpenCamera)
	{
		if (!m_dataValid)
		{
			Sleep(3);	//无人脸避免持续循环，浪费cpu资源
			continue;
		}

		MRESULT res = MOK;
		{
			std::lock_guard<mutex> lock(g_mtx);
			multiFaceInfo.faceNum = 1;
			multiFaceInfo.faceOrient[0] = m_videoFaceInfo.faceOrient[0];
			multiFaceInfo.faceRect[0] = m_videoFaceInfo.faceRect[0];
			multiFaceInfo.faceID[0] = m_videoFaceInfo.faceID[0];
			memset(multiFaceInfo.faceDataInfoList[0].data, 0, FACE_DATA_SIZE);
			memcpy(multiFaceInfo.faceDataInfoList[0].data, m_videoFaceInfo.faceDataInfoList[0].data, FACE_DATA_SIZE);
			multiFaceInfo.faceDataInfoList[0].dataSize = m_videoFaceInfo.faceDataInfoList[0].dataSize;
		}

		// 活体检测为真时本张人脸不再处理
		if (m_isLive)
		{
			Sleep(3);
			continue;
		}

		//设置活体阈值
		m_imageFlEngine.SetLivenessThreshold(rgbLiveThreshold->text().toFloat(), irLiveThreshold->text().toFloat());

		//单目默认只做RGB活体，使用RGB活体检测结果去判断人脸比对是否通过
		if (1 == m_cameraNameList.size())
		{
			//RGB活体检测
			IplImage *rgbImage = NULL;
			{
				std::lock_guard<mutex> lock(g_mtx);
				if (m_rgbFrame.empty())
				{
					continue;
				}
				IplImage iplRgbImage(m_rgbFrame);
				rgbImage = cvCloneImage(&iplRgbImage);
			}

			ASF_LivenessInfo rgbLivenessInfo = { 0 };
			res = m_imageFlEngine.livenessProcess(multiFaceInfo, rgbImage, rgbLivenessInfo, true);
			if (MOK == res && rgbLivenessInfo.num > 0 && rgbLivenessInfo.isLive[0] == 1)
			{
				m_rgbLivenessInfo = "Live";
				m_isLive = true;
			}
			else
			{
				if (MOK != res || rgbLivenessInfo.num < 1)
				{
					m_rgbLivenessInfo = "";
				}
				else
				{
					rgbLivenessInfo.isLive[0] == 0 ? m_rgbLivenessInfo = "No Live" : 
						m_rgbLivenessInfo = "Unknown";
				}
			}
			cvReleaseImage(&rgbImage);
		}

		//双目只做IR活体，人脸比对使用IR活体去判断人脸比对是否通过
		if (2 == m_cameraNameList.size())
		{
			IplImage *irImage = NULL;
			{
				
				std::lock_guard<mutex> lock(g_mtx);
				if (m_irFrame.empty())
				{
					continue;
				}
				IplImage iplIrImage(m_irFrame);
				irImage = cvCloneImage(&iplIrImage);
			}

			//双目摄像头偏移量调整
			multiFaceInfo.faceRect[0].left -= cameraHorizontalAlignment->text().toInt();
			multiFaceInfo.faceRect[0].top -= cameraVerticalAlignment->text().toInt();
			multiFaceInfo.faceRect[0].right -= cameraHorizontalAlignment->text().toInt();
			multiFaceInfo.faceRect[0].bottom -= cameraVerticalAlignment->text().toInt();

			//做双目对齐之后需要更新FaceData
			res = m_imageFlEngine.UpdateFaceData(irImage, multiFaceInfo);
			if (MOK != res)
			{
				m_irLivenessInfo = "";
				continue;
			}

			ASF_LivenessInfo irLivenessInfo = { 0 };
			res = m_imageFlEngine.livenessProcess(multiFaceInfo, irImage, irLivenessInfo, false);
			if (MOK == res && irLivenessInfo.num > 0 && irLivenessInfo.isLive[0] == 1)
			{
				m_irLivenessInfo = "Live";
				m_isLive = true;
			}
			else
			{
				if (MOK != res || irLivenessInfo.num < 1)
				{
					m_irLivenessInfo = "";
				}
				else
				{
					irLivenessInfo.isLive[0] == 0 ? m_irLivenessInfo ="No Live" : 
						m_irLivenessInfo = "Unknown";
				}
			}
			cvReleaseImage(&irImage);
		}
	}
	SafeFree(multiFaceInfo.faceOrient);
	SafeFree(multiFaceInfo.faceRect);
	SafeFree(multiFaceInfo.faceID);
	for (int i = 0; i < FACENUM; i++) {
		SafeFree(multiFaceInfo.faceDataInfoList[i].data);
	}
  	SafeFree(multiFaceInfo.faceDataInfoList);
}



//红外预览人脸框边界值保护
void ArcFaceDetection::previewIrFaceRect(MRECT srcFaceRect, MRECT* dstFaceRect)
{
	dstFaceRect->left = srcFaceRect.left - cameraHorizontalAlignment->text().toInt();
	if (dstFaceRect->left < 0)
		dstFaceRect->left = 2;
	dstFaceRect->top = srcFaceRect.top - cameraVerticalAlignment->text().toInt();;
	if (dstFaceRect->top < 0)
		dstFaceRect->top = 2;
	dstFaceRect->right = srcFaceRect.right - cameraHorizontalAlignment->text().toInt();
	if (dstFaceRect->right > VIDEO_FRAME_DEFAULT_WIDTH)
		dstFaceRect->right = VIDEO_FRAME_DEFAULT_WIDTH - 2;
	dstFaceRect->bottom = srcFaceRect.bottom - cameraVerticalAlignment->text().toInt();;
	if (dstFaceRect->bottom > VIDEO_FRAME_DEFAULT_HEIGHT)
		dstFaceRect->bottom = VIDEO_FRAME_DEFAULT_HEIGHT - 2;
}

//拍照保存
bool ArcFaceDetection::saveCameraRegisterImage()
{
	QString strFolderPath = QDir::currentPath() + QStringLiteral(CAMERA_REGISTER_IMAGE_PATH);
	if (!isDirExist(strFolderPath))
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("文件夹创建失败！"));
		return false;
	}

	if (!m_dataValid)
	{
		return false;
	}

	cv::Mat rgbRegisterImage;
	{
		std::lock_guard<mutex> lock(g_mtx);
		rgbRegisterImage = m_rgbFrame.clone();
	} 

	m_cameraRegisterImagePath = "";
	int timeStamp = QDateTime::currentDateTime().toTime_t(); 
	m_cameraRegisterImagePath = strFolderPath + "/" + QString::number(timeStamp) + ".jpg";
	if (!cv::imwrite(string(m_cameraRegisterImagePath.toLocal8Bit()), rgbRegisterImage))
	{
		return false;
	}
	
	return true;
}

//拍照注册
void ArcFaceDetection::cameraRegisterSingleFace()
{
	if (!m_isOpenCamera)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("请先打开摄像头！"));
		return;
	}
	
	if (!saveCameraRegisterImage())
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("拍照失败，请正脸对准！"));
		return;
	}

	m_isCapture = true;

	registerSingleFace();
}

//注册单张人脸
void ArcFaceDetection::registerSingleFace()
{
	if (m_isRegisterThreadRunning)
	{
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("等待当前注册任务执行结束!"));
		return;
	}

	registerListWidget->setIconSize(QSize(LIST_WIDGET_ITEM_WIDTH, LIST_WIDGET_ITEM_HEIGHT));
	//设置QListWidget的显示模式
	registerListWidget->setViewMode(QListView::IconMode);
	//设置QListWidget中的单元项不可被拖动
	registerListWidget->setMovement(QListView::Static);
	registerListWidget->setSpacing(13);

	picPathList.clear();
	QString imgFilePath = "";
	if (m_isCapture)
	{
		imgFilePath = m_cameraRegisterImagePath;
		m_isCapture = false;
	}
	else
	{
		imgFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择图片"),
			"F:\\", tr("Image files(*.bmp *.jpg *.png);;All files (*.*)"));
	}

	if (imgFilePath.isEmpty())
	{
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择注册照!"));
		return;
	}

	picPathList << imgFilePath;	//插入链表

	if (picPathList.size() > 0)
	{
		std::thread th(&ArcFaceDetection::showRegisterThumbnail, this);
		th.detach();
	}
}

// 人脸库操作
void ArcFaceDetection::registerFaceDatebase()
{
	if (m_isRegisterThreadRunning)
	{
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("等待当前注册任务执行结束!"));
		return;
	}

	registerListWidget->setIconSize(QSize(LIST_WIDGET_ITEM_WIDTH, LIST_WIDGET_ITEM_HEIGHT));
	//设置QListWidget的显示模式
	registerListWidget->setViewMode(QListView::IconMode);
	//设置QListWidget中的单元项不可被拖动
	registerListWidget->setMovement(QListView::Static);
	registerListWidget->setSpacing(13);

	picPathList.clear();
	picPathList = getDestFolderImages();

	if (picPathList.size() > 0)
	{
		std::thread th(&ArcFaceDetection::showRegisterThumbnail, this);
		th.detach();
	}
}

void ArcFaceDetection::showRegisterThumbnail()
{
	m_isRegisterThreadRunning = true;	//注册线程开始执行

	editOutputLog(QStringLiteral("开始注册人脸库..."));
	editOutputLog(QStringLiteral("选择待注册图：") + QString::number(picPathList.size()));

	ASF_MultiFaceInfo multiFaceInfo = { 0 };
	multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32) * FACENUM);	//一张人脸
	multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT) * FACENUM);
	multiFaceInfo.faceID = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	multiFaceInfo.faceDataInfoList = (ASF_FaceDataInfo*)malloc(sizeof(ASF_FaceDataInfo) * FACENUM);
	for (int i = 0; i < FACENUM; i++) {
		multiFaceInfo.faceDataInfoList[i].data = malloc(FACE_DATA_SIZE);
	}

	for (int i = 0; i < picPathList.size(); ++i)
	{
		QString picPath = picPathList.at(i);
		std::string strPicPath = picPath.toStdString();
		strPicPath = (const char*)picPath.toLocal8Bit();	//防止中文乱码

		IplImage* registerImage = cvLoadImage(strPicPath.c_str());
		MRESULT res = m_imageEngine.PreDetectFace(registerImage, multiFaceInfo, true);
		if (MOK != res)
		{
			cvReleaseImage(&registerImage);
			continue;
		}

		ASF_SingleFaceInfo singleFaceInfo = { 0 };
		singleFaceInfo.faceOrient = multiFaceInfo.faceOrient[0];
		singleFaceInfo.faceRect = multiFaceInfo.faceRect[0];
		singleFaceInfo.faceDataInfo = multiFaceInfo.faceDataInfoList[0];

		//注册照要求不带口罩，图像质量要求相对较高，推荐阈值0.63
		//注册照为身份证件照的情况下，可以不做图像质量检测
		if (m_controlImageQulity == 1)
		{
			res = m_imageEngine.PreImageQualityDetect(registerImage, singleFaceInfo, ASF_REGISTER, 0, getFqThreshold());
			if (MOK != res) {
				cvReleaseImage(&registerImage);
				continue;
			}
		}

		ASF_FaceFeature faceFeature = { 0 };
		faceFeature.featureSize = FACE_FEATURE_SIZE;
		faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

		res = m_imageEngine.PreExtractFeature(registerImage, singleFaceInfo, faceFeature, ASF_REGISTER, 0);
		if (MOK != res)
		{
			SafeFree(faceFeature.feature);
			cvReleaseImage(&registerImage);
			continue;
		}

		//将图片数据加入到内存
		QByteArray pData;
		QFile fp(picPath);
		fp.open(QIODevice::ReadOnly);
		pData = fp.readAll();
		fp.close();
		QImage imgPix;
		imgPix.loadFromData(pData);
		QPixmap pix;
		pix = QPixmap::fromImage(imgPix);

		emit curListWigetItem(m_nIndex, pix);	//发送信号到主线程

		m_featuresVec.push_back(faceFeature);
		cvReleaseImage(&registerImage);
		m_nIndex++;
	}
	editOutputLog(QStringLiteral("注册成功：") + QString::number(m_nIndex - 1));

	SafeFree(multiFaceInfo.faceOrient);
	SafeFree(multiFaceInfo.faceRect);
	SafeFree(multiFaceInfo.faceID);
	for (int i = 0; i < FACENUM; i++) {
		SafeFree(multiFaceInfo.faceDataInfoList[i].data);
	}
	SafeFree(multiFaceInfo.faceDataInfoList);

	m_isRegisterThreadRunning = false;		//注册线程执行结束
}

void ArcFaceDetection::drawListWigetItem(int nIndex, QPixmap pix)
{
	QListWidgetItem *pItem = new QListWidgetItem(QIcon(pix.scaled(QSize(LIST_WIDGET_ITEM_WIDTH, LIST_WIDGET_ITEM_HEIGHT), Qt::KeepAspectRatio)),
		QString::number(nIndex) + QStringLiteral("号"), registerListWidget);
	//设置单元项的宽度和高度
	pItem->setSizeHint(QSize(LIST_WIDGET_ITEM_WIDTH, LIST_WIDGET_ITEM_HEIGHT + 20));
	pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
}

void ArcFaceDetection::clearFaceDatebase()
{
	if (m_isOpenCamera)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("请先关闭摄像头！"));
		return;
	}

	m_nIndex = 1;
	registerListWidget->clear();

	for (auto feature : m_featuresVec)
	{
		SafeFree(feature.feature);
	}

	m_featuresVec.clear();

	editOutputLog(QStringLiteral("清除人脸库"));
}


// 识别照操作
void ArcFaceDetection::selectRecognitionImage()
{
	if (m_isOpenCamera)
	{
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先关闭摄像头!"));
		return;
	}

	QString imgFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择图片"),
		"F:\\", tr("Image files(*.bmp *.jpg *.png);;All files (*.*)"));

	if (imgFilePath.isEmpty())
	{
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择识别照!"));
		return;
	}

	//FD、FR、process算法处理
	QString strProcessInfo;
	std::string strPicPath = string(imgFilePath.toLocal8Bit());
	IplImage* recognitionImage = cvLoadImage(strPicPath.c_str());
	detectionRecognitionImage(recognitionImage, m_recognitionFaceInfo, strProcessInfo);

	//缩放比例
	double nScale = VIDEO_FRAME_DEFAULT_WIDTH / VIDEO_FRAME_DEFAULT_HEIGHT;
	if (double(recognitionImage->width) / double(recognitionImage->height) > nScale)
	{
		nScale = VIDEO_FRAME_DEFAULT_WIDTH / double(recognitionImage->width);
	}
	else
	{
		nScale = VIDEO_FRAME_DEFAULT_HEIGHT / double(recognitionImage->height);
	}

	//对图片进行等比例缩放
	CvSize destImgSize;
	destImgSize.width = recognitionImage->width * nScale;
	destImgSize.height = recognitionImage->height * nScale;
	IplImage *destImage = cvCreateImage(destImgSize, recognitionImage->depth, recognitionImage->nChannels);
	cvResize(recognitionImage, destImage, CV_INTER_AREA);

	//对人脸框进行按比例缩放
	MRECT* destRect = (MRECT*)malloc(sizeof(MRECT));
	scaleFaceRect(m_recognitionFaceInfo.faceRect, destRect, nScale);

	//画框
	cvRectangle(destImage, cvPoint(destRect->left, destRect->top),
		cvPoint(destRect->right, destRect->bottom), CV_RGB(100, 200, 255), 2, 1, 0);

	//画字
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_COMPLEX, 0.5, 0.8, 0);
	cvPutText(destImage, strProcessInfo.toStdString().c_str(),
		cvPoint(destRect->left, destRect->top - 15), &font, cvScalar(90, 240, 120));

	//预览
	m_pixStaticRgbImage = QPixmap::fromImage(cvMatToQImage(cv::cvarrToMat(destImage)));
	
	
	SafeFree(destRect);
	cvReleaseImage(&recognitionImage);
	cvReleaseImage(&destImage);
}

void ArcFaceDetection::detectionRecognitionImage(IplImage* recognitionImage, ASF_SingleFaceInfo& faceInfo, QString& strInfo)
{
	ASF_MultiFaceInfo multiFaceInfo = { 0 };
	multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32) * FACENUM);	//一张人脸
	multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT) * FACENUM);
	multiFaceInfo.faceID = (MInt32*)malloc(sizeof(MInt32) * FACENUM);
	multiFaceInfo.faceDataInfoList = (ASF_FaceDataInfo*)malloc(sizeof(ASF_FaceDataInfo) * FACENUM);
	for (int i = 0; i < FACENUM; i++) {
		multiFaceInfo.faceDataInfoList[i].data = malloc(FACE_DATA_SIZE);
	}

	MRESULT res = m_imageEngine.PreDetectFace(recognitionImage, multiFaceInfo, true);
	if (MOK != res)
	{
		editOutputLog(QStringLiteral("未检测到人脸！"));
	}
	else
	{
		ASF_AgeInfo ageInfo = { 0 };
		ASF_GenderInfo genderInfo = { 0 };
		ASF_Face3DAngle angleInfo = { 0 };
		ASF_LivenessInfo livenessInfo = { 0 };

		//赋值传出
		faceInfo.faceOrient = multiFaceInfo.faceOrient[0];
		faceInfo.faceRect = multiFaceInfo.faceRect[0];
		memset(faceInfo.faceDataInfo.data, 0, FACE_DATA_SIZE);
		memcpy(faceInfo.faceDataInfo.data, multiFaceInfo.faceDataInfoList[0].data, FACE_DATA_SIZE);
		faceInfo.faceDataInfo.dataSize = multiFaceInfo.faceDataInfoList[0].dataSize;

		res = m_imageEngine.FaceProcess(multiFaceInfo, recognitionImage, ageInfo, genderInfo, angleInfo, livenessInfo);
		if (MOK == res)
		{
			strInfo.sprintf("%s, %s, %s", to_string(ageInfo.ageArray[0]).c_str(),
				genderInfo.genderArray[0] == 0 ? "Male" : (genderInfo.genderArray[0] == 1 ? "Female" : "unknown"),
				livenessInfo.isLive[0] == 1 ? "live" : (livenessInfo.isLive[0] == 0 ? "No Live" : "unknown"));
		}

		//FR
		ASF_MaskInfo maskInfo = { 0 };
		res = m_imageEngine.FaceProcessMask(multiFaceInfo, recognitionImage, maskInfo);
		if (res != MOK || maskInfo.num < 1 || maskInfo.maskArray[0] == -1)
		{
			editOutputLog(QStringLiteral("口罩检测失败！"));
			m_recognitionImageSucceed = FALSE;
		}
		else
		{
			strInfo += maskInfo.maskArray[0] == 1 ? ", Mask" : ", NoMask";
			//戴口罩识别阈值 0.29  不带口罩识别阈值 0.49
			if (m_controlImageQulity == 1)
			{
				res = m_imageEngine.PreImageQualityDetect(recognitionImage, faceInfo,
					ASF_RECOGNITION, maskInfo.maskArray[0], getFqThreshold());
			}

			if (res != MOK)
			{
				editOutputLog(QStringLiteral("图像质量检测失败！"));
				m_recognitionImageSucceed = FALSE;
			}
			else
			{
				res = m_imageEngine.PreExtractFeature(recognitionImage, faceInfo, m_recognitionImageFeature,
					ASF_RECOGNITION, maskInfo.maskArray[0]);
				if (MOK == res)
				{
					editOutputLog(QStringLiteral("识别照特征提取成功！"));
					m_recognitionImageSucceed = TRUE;
				}
				else
				{
					editOutputLog(QStringLiteral("识别照特征提取失败！"));
					m_recognitionImageSucceed = FALSE;
				}
			}
		}
	}
	SafeFree(multiFaceInfo.faceOrient);
	SafeFree(multiFaceInfo.faceRect);
	SafeFree(multiFaceInfo.faceID);
	for (int i = 0; i < FACENUM; i++) {
		SafeFree(multiFaceInfo.faceDataInfoList[i].data);
	}
	SafeFree(multiFaceInfo.faceDataInfoList);
}

void ArcFaceDetection::scaleFaceRect(MRECT srcFaceRect, MRECT* dstFaceRect, double nScale)
{
	dstFaceRect->left = srcFaceRect.left * nScale;
	dstFaceRect->top = srcFaceRect.top * nScale;
	dstFaceRect->right = srcFaceRect.right * nScale;
	dstFaceRect->bottom = srcFaceRect.bottom * nScale;
}

//人脸比对模块
void ArcFaceDetection::faceCompare()
{
	if (!m_recognitionImageSucceed)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("人脸比对失败，请选择识别照！"));
		return;
	}

	if (m_featuresVec.size() == 0)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("请注册人脸库！"));
		return;
	}

	int maxIndex = 1, curIndex = 0;
	MFloat maxThreshold = 0.0;
	for (auto registerFeature : m_featuresVec)
	{
		curIndex++;
		MFloat confidenceLevel = 0.0;
		MRESULT pairRes = m_imageEngine.FacePairMatching(confidenceLevel, m_recognitionImageFeature, 
			registerFeature, m_compareModel);

		if (MOK == pairRes && confidenceLevel > maxThreshold)
		{
			maxThreshold = confidenceLevel;
			maxIndex = curIndex;
		}
	}

	editOutputLog(QString::number(maxIndex) + QStringLiteral("号：") + QString("%1").arg(maxThreshold));
	editOutputLog(QStringLiteral("比对结束！"));
}

//获取fq阈值
FqThreshold ArcFaceDetection::getFqThreshold()
{
	FqThreshold fqThreshold;
	fqThreshold.fqRegisterThreshold = fqRegisterThreshold->text().toFloat();
	fqThreshold.fqRecognitionMaskThreshold = fqRecognitionMaskThreshold->text().toFloat();
	fqThreshold.fqRecognitionNoMaskThreshold = fqRecognitionNoMaskThreshold->text().toFloat();
	return fqThreshold;
}

//日志输出
void ArcFaceDetection::editOutputLog(QString str)
{
	editOutLog->append(str);
}

//预览绘制
void ArcFaceDetection::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	if (m_isOpenCamera)
	{
		labelPreview->setPixmap(m_pixRgbImage);
	}
	else
	{
		labelPreview->setPixmap(m_pixStaticRgbImage);
		labelPreview->setAlignment(Qt::AlignCenter);
	}
}

//关闭窗口前的工作
void ArcFaceDetection::closeEvent(QCloseEvent *event)
{
	m_isOpenCamera = false;
	m_dataValid = false;
	Sleep(1000);	//等待线程资源释放
	if (2 == m_cameraNameList.size() && irPreviewDialog != nullptr)
	{
		irPreviewDialog->close();	//关闭程序时关掉红外摄像头子窗口
	}
}

// 无边框移动
void ArcFaceDetection::mouseMoveEvent(QMouseEvent *event)
{
	if (m_isPressed)
		move(event->pos() - m_movePoint + pos());
}

void ArcFaceDetection::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	m_isPressed = false;
}

void ArcFaceDetection::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_isPressed = true;
		m_movePoint = event->pos();
	}
}

void ArcFaceDetection::switchCamera()
{
	if (m_isOpenCamera)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("请先关闭摄像头！"));
		return;
	} 

	int cameraId = m_rgbCameraId;
	m_rgbCameraId = m_irCameraId;
	m_irCameraId = cameraId;
	editOutputLog(QStringLiteral("摄像头索引切换成功！") + "\t" + 
		"RGB Index:" + QString::number(m_rgbCameraId) + "\t" + "IR Index:" + QString::number(m_irCameraId));
	return;
}

void ArcFaceDetection::controlLiveness()
{
	if (m_isOpenCamera)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("请先关闭摄像头！"));
		return;
	}

	if (1 == m_controlLiveness)
	{
		m_controlLiveness = 0;
		m_isLive = true;
		btnControlLiveness->setText(QStringLiteral("活体已关闭"));
		editOutputLog(QStringLiteral("关闭活体检测！"));
	}
	else
	{
		m_controlLiveness = 1;
		m_isLive = false;
		btnControlLiveness->setText(QStringLiteral("活体已开启"));
		editOutputLog(QStringLiteral("开启活体检测！"));
	}
	return;
}

void ArcFaceDetection::controlImageQuality()
{
	if (m_isOpenCamera)
	{
		QMessageBox::about(NULL, QStringLiteral("提示"), QStringLiteral("请先关闭摄像头！"));
		return;
	}

	if (m_controlImageQulity == 1)
	{
		m_controlImageQulity = 0;
		btnControlImageQuality->setText(QStringLiteral("质量检测已关闭"));
		editOutputLog(QStringLiteral("图像质量检测已关闭！"));
	}
	else
	{
		m_controlImageQulity = 1;
		btnControlImageQuality->setText(QStringLiteral("质量检测已开启"));
		editOutputLog(QStringLiteral("图像质量检测已开启！"));
	}
}

void ArcFaceDetection::selectCompareModel()
{
	if (m_compareModel == ASF_LIFE_PHOTO)
	{
		m_compareModel = ASF_ID_PHOTO;
		editOutputLog(QStringLiteral("当前比对模式：") + "ASF_ID_PHOTO");
	}
	else
	{
		m_compareModel = ASF_LIFE_PHOTO;
		editOutputLog(QStringLiteral("当前比对模式：") + "ASF_LIFE_PHOTO");
	}
	return;
}