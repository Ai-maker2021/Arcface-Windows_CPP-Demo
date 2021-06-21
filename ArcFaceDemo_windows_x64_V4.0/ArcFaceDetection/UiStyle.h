#pragma once

#include <QFile>
#include <QApplication>

class CommonHelper
{
public:
	static void setStyle(const QString &style) {
		QFile styleFile(style);
		if (styleFile.open(QFile::ReadOnly))
		{
			qDebug("open successed!");
			styleFile.open(QFile::ReadOnly);
			qApp->setStyleSheet(styleFile.readAll());
			styleFile.close();
		}
		else
		{
			qDebug("open failed!");
		}
	}
};
