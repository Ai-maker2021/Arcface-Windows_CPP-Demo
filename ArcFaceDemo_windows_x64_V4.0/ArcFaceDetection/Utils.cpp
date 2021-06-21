#include "Utils.h"
#include <strmif.h>
#include <initguid.h>
#include <QImage>
#include <QPixmap>
#include <QSettings>
#include <QStringList>
#include <QFileDialog>
#include <QDirIterator>
#include "config.h"

#pragma comment(lib, "setupapi.lib")

#define VI_MAX_CAMERAS 20
DEFINE_GUID(CLSID_SystemDeviceEnum, 0x62be5d10, 0x60eb, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);
DEFINE_GUID(CLSID_VideoInputDeviceCategory, 0x860bb310, 0x5d01, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);
DEFINE_GUID(IID_ICreateDevEnum, 0x29840822, 0x5b84, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);

Utils::Utils()
{
}


Utils::~Utils()
{
}

//列出摄像头
int Utils::listDevices(vector<string>& list)
{
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	int deviceCounter = 0;
	CoInitialize(NULL);

	HRESULT hr = CoCreateInstance(
		CLSID_SystemDeviceEnum,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum,
		reinterpret_cast<void**>(&pDevEnum)
	);

	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
		if (hr == S_OK) {

			IMoniker *pMoniker = NULL;
			while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
			{
				IPropertyBag *pPropBag;
				hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
					(void**)(&pPropBag));

				if (FAILED(hr)) {
					pMoniker->Release();
					continue; // Skip this one, maybe the next one will work.
				}

				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"Description", &varName, 0);
				if (FAILED(hr))
				{
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				}

				if (SUCCEEDED(hr))
				{
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);
					int count = 0;
					char tmp[255] = { 0 };
					while (varName.bstrVal[count] != 0x00 && count < 255)
					{
						tmp[count] = (char)varName.bstrVal[count];
						count++;
					}
					list.push_back(tmp);
				}

				pPropBag->Release();
				pPropBag = NULL;
				pMoniker->Release();
				pMoniker = NULL;

				deviceCounter++;
			}

			pDevEnum->Release();
			pDevEnum = NULL;
			pEnum->Release();
			pEnum = NULL;
		}
	}
	return deviceCounter;
}


QImage Utils::cvMatToQImage(const cv::Mat& mat)
{
	// 8-bits unsigned, NO. OF CHANNELS = 1  
	if (mat.type() == CV_8UC1) {
		QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
		// Set the color table (used to translate colour indexes to qRgb values)  
		image.setColorCount(256);
		for (int i = 0; i < 256; i++)
			image.setColor(i, qRgb(i, i, i));

		// Copy input Mat  
		uchar *pSrc = mat.data;
		for (int row = 0; row < mat.rows; row++) {
			uchar *pDest = image.scanLine(row);
			memcpy(pDest, pSrc, mat.cols);
			pSrc += mat.step;
		}
		return image;
	}
	// 8-bits unsigned, NO. OF CHANNELS = 3  
	else if (mat.type() == CV_8UC3) {
		// Copy input Mat  
		const uchar *pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat  
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	else if (mat.type() == CV_8UC4) {
		//qDebug() << "CV_8UC4";
		// Copy input Mat  
		const uchar *pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat  
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
		return image.copy();
	}
	else {
		//qDebug() << "ERROR: Mat could not be converted to QImage.";
		return QImage();
	}
}

void Utils::readSetting(char appId[], char sdkKey[], char activeKey[])
{
	QSettings settings(SETTING_FILE,QSettings::IniFormat);

	settings.beginGroup(ACTIVE_SETTING);
	QString strAppId = settings.value("appId").toString();
	strcpy_s(appId, 64, strAppId.toStdString().c_str());
	QString strSdkKey = settings.value("sdkKey").toString();
	strcpy_s(sdkKey, 64, strSdkKey.toStdString().c_str());
	QString strActiveKey = settings.value("activeKey").toString();
	strcpy_s(activeKey, 20, strActiveKey.toStdString().c_str());
	settings.endGroup();
}

void Utils::writeSettingIni(QString appId, QString sdkKey, QString activeKey)
{
	QSettings settings(SETTING_FILE, QSettings::IniFormat);
	settings.beginGroup(ACTIVE_SETTING);
	settings.setValue("appId", appId);
	settings.setValue("sdkKey", sdkKey);
	settings.setValue("activeKey", activeKey);
	settings.endGroup();
}


QStringList Utils::getDestFolderImages()
{
	QStringList imageList;
	QFileInfo fileInfo;
	QString filePath;
	QString suffixName;

	//选择文件夹路径
	QString folderPath = QFileDialog::getExistingDirectory(nullptr, "choose src Directory", "F:\\");
	if (folderPath.isEmpty())
	{
		return imageList;
	}

	QDir dir(folderPath);
	//仅获取指定路径下的所有文件，不递归查找
	QFileInfoList allFilePath = dir.entryInfoList();
	for (int i = 0; i < allFilePath.size(); ++i)
	{
		fileInfo = allFilePath.at(i);
		suffixName = fileInfo.suffix();
		if (suffixName == "jpg" || suffixName == "JPG" || suffixName == "png" ||
			suffixName == "PNG" || suffixName == "bmp" || suffixName == "BMP")
		{
			filePath = fileInfo.filePath();
			imageList << filePath;
		}
	}
	return imageList;
}

QStringList Utils::getFolderAllImages()
{
	QStringList imageList;
	QFileInfo fileinfo;
	QString filePath;
	QString suffixName;

	//选择文件夹路径
	QString folderPath = QFileDialog::getExistingDirectory(nullptr, "choose src Directory", NULL);
	if (folderPath.isEmpty())
	{
		return imageList;
	}

	//遍历文件夹下所有文件
	QDirIterator allFilePath(folderPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (allFilePath.hasNext())
	{
		filePath = allFilePath.next();
		fileinfo = QFileInfo(filePath);
		suffixName = fileinfo.suffix();
		if (suffixName == "jpg" || suffixName == "JPG" || suffixName == "png" ||
			suffixName == "PNG" || suffixName == "bmp" || suffixName == "BMP")
		{
			imageList << filePath;
		}
	}
	return imageList;
}

bool Utils::isDirExist(QString fullPath)
{
	QDir dir(fullPath);
	if (dir.exists())
	{
		return true;
	}
	else
	{
		bool flag = dir.mkdir(fullPath); //只创建一级子目录，保证上级目录要存在
		//bool flag = dir.mkpath(fullPath); //创建多级目录
		return flag;
	}
}


//替换中文乱码问题
char* Utils::utf8ToGbk(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len <= 0)
	{
		return "";
	}

	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr)
		delete[] wstr;
	return str;
}


void getStringSize(HDC hDC, const char* str, int* w, int* h)
{
	SIZE size;
	GetTextExtentPoint32A(hDC, str, strlen(str), &size);
	if (w != 0) *w = size.cx;
	if (h != 0) *h = size.cy;
}

void Utils::putTextZH(IplImage* dst, const char* str, cv::Point org, cv::Scalar color, int fontSize,
	const char *fn /*= "Arial"*/, bool italic /*= false*/, bool underline /*= false*/)
{
	CV_Assert(dst->imageData != 0 && (dst->nChannels == 1 || dst->nChannels == 3));

	int x, y, r, b;
	if (org.x > dst->width || org.y > dst->height) return;
	x = org.x < 0 ? -org.x : 0;
	y = org.y < 0 ? -org.y : 0;

	LOGFONTA lf;
	lf.lfHeight = -fontSize;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_BLACK;		//黑体
	lf.lfItalic = italic;		//斜体
	lf.lfUnderline = underline; //下划线
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfPitchAndFamily = 0;
	strcpy_s(lf.lfFaceName, fn);

	HFONT hf = CreateFontIndirectA(&lf);
	HDC hDC = CreateCompatibleDC(0);
	HFONT hOldFont = (HFONT)SelectObject(hDC, hf);

	int strBaseW = 0, strBaseH = 0;
	int singleRow = 0;
	char buf[1 << 12];
	strcpy_s(buf, str);
	char *bufT[1 << 12];  // 这个用于分隔字符串后剩余的字符，可能会超出。
						  //处理多行
	{
		int nnh = 0;
		int cw, ch;

		const char* ln = strtok_s(buf, "\n", bufT);
		while (ln != 0)
		{
			getStringSize(hDC, ln, &cw, &ch);
			strBaseW = max(strBaseW, cw);
			strBaseH = max(strBaseH, ch);

			ln = strtok_s(0, "\n", bufT);
			nnh++;
		}
		singleRow = strBaseH;
		strBaseH *= nnh;
	}

	if (org.x + strBaseW < 0 || org.y + strBaseH < 0)
	{
		SelectObject(hDC, hOldFont);
		DeleteObject(hf);
		DeleteObject(hDC);
		return;
	}

	r = org.x + strBaseW > dst->width ? dst->width - org.x - 1 : strBaseW - 1;
	b = org.y + strBaseH > dst->height ? dst->height - org.y - 1 : strBaseH - 1;
	org.x = org.x < 0 ? 0 : org.x;
	org.y = org.y < 0 ? 0 : org.y;

	BITMAPINFO bmp = { 0 };
	BITMAPINFOHEADER& bih = bmp.bmiHeader;
	int strDrawLineStep = strBaseW * 3 % 4 == 0 ? strBaseW * 3 : (strBaseW * 3 + 4 - ((strBaseW * 3) % 4));

	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = strBaseW;
	bih.biHeight = strBaseH;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = strBaseH * strDrawLineStep;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	void* pDibData = 0;
	HBITMAP hBmp = CreateDIBSection(hDC, &bmp, DIB_RGB_COLORS, &pDibData, 0, 0);

	if (pDibData == 0)
	{
		return;
	}

	CV_Assert(pDibData != 0);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);

	//color.val[2], color.val[1], color.val[0]
	SetTextColor(hDC, RGB(255, 255, 255));
	SetBkColor(hDC, 0);
	//SetStretchBltMode(hDC, COLORONCOLOR);

	strcpy_s(buf, str);
	const char* ln = strtok_s(buf, "\n", bufT);
	int outTextY = 0;
	while (ln != 0)
	{
		TextOutA(hDC, 0, outTextY, ln, strlen(ln));
		outTextY += singleRow;
		ln = strtok_s(0, "\n", bufT);
	}
	uchar* dstData = (uchar*)dst->imageData;
	int dstStep = dst->widthStep / sizeof(dstData[0]);
	unsigned char* pImg = (unsigned char*)dst->imageData + org.x * dst->nChannels + org.y * dstStep;
	unsigned char* pStr = (unsigned char*)pDibData + x * 3;
	for (int tty = y; tty <= b; ++tty)
	{
		unsigned char* subImg = pImg + (tty - y) * dstStep;
		unsigned char* subStr = pStr + (strBaseH - tty - 1) * strDrawLineStep;
		for (int ttx = x; ttx <= r; ++ttx)
		{
			for (int n = 0; n < dst->nChannels; ++n) {
				double vtxt = subStr[n] / 255.0;
				int cvv = vtxt * color.val[n] + (1 - vtxt) * subImg[n];
				subImg[n] = cvv > 255 ? 255 : (cvv < 0 ? 0 : cvv);
			}

			subStr += 3;
			subImg += dst->nChannels;
		}
	}

	SelectObject(hDC, hOldBmp);
	SelectObject(hDC, hOldFont);
	DeleteObject(hf);
	DeleteObject(hBmp);
	DeleteDC(hDC);
}


