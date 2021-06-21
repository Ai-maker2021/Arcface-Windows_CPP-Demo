#pragma once
#include <QDialog>
#include <QMouseEvent>
#include <QLabel>

#define IR_PREVIEW_SIZE_WIDTH 320
#define IR_PREVIEW_SIZE_HEIGHT 240

class IrPreviewDialog : public QDialog
{
public:
	IrPreviewDialog(QDialog *parent = 0);
	~IrPreviewDialog();


public:
	bool m_bPressed;
	QPoint m_point;

	void mouseReleaseEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

	QLabel* pIrPreviewLabel;
	QPixmap m_pixIrImage;

protected:
	void paintEvent(QPaintEvent *event);
};

