#include "ArcFaceDetection.h"
#include <QtWidgets/QApplication>
#include "UiStyle.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	CommonHelper::setStyle(":/Resources/style.qss");

	ArcFaceDetection w;
	w.show();
	return a.exec();
}
