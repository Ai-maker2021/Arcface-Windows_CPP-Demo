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
	QStringList getDestFolderImages();			//当前文件夹不递归查找
	QStringList getFolderAllImages();			//当前文件夹递归查找
	bool isDirExist(QString fullPath);			//判断目录是否存在，不存在则创建

	//编码转换
	char* utf8ToGbk(const char* utf8);
	//代替opencv cvPutText函数中文乱码问题(特殊情况与QT有冲突)
	void putTextZH(IplImage* dst, const char* str, cv::Point org, cv::Scalar color, int fontSize,
		const char *fn = "Arial", bool italic = false, bool underline = false);

};

