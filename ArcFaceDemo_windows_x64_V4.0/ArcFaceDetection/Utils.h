#pragma once
#include <vector>
#include <string>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <QString>

using namespace std;

class QImage;
class QPixmap;
class QStringList;

class Utils
{
public:
	Utils();
	~Utils();


public:
	int listDevices(vector<string>& list);
	QImage cvMatToQImage(const cv::Mat& mat);
	void readSetting(char appId[], char sdkKey[], char activeKey[]);
	void writeSettingIni(QString appId, QString sdkKey, QString activeKey);
	QStringList getDestFolderImages();			//��ǰ�ļ��в��ݹ����
	QStringList getFolderAllImages();			//��ǰ�ļ��еݹ����
	bool isDirExist(QString fullPath);			//�ж�Ŀ¼�Ƿ���ڣ��������򴴽�

	//����ת��
	char* utf8ToGbk(const char* utf8);
	//����opencv cvPutText����������������(���������QT�г�ͻ)
	void putTextZH(IplImage* dst, const char* str, cv::Point org, cv::Scalar color, int fontSize,
		const char *fn = "Arial", bool italic = false, bool underline = false);

};

