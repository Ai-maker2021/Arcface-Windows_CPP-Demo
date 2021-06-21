#include "IrPreviewDialog.h"



IrPreviewDialog::IrPreviewDialog(QDialog *parent)
{
	setAttribute(Qt::WA_DeleteOnClose);		//窗口关闭，释放所有资源
	m_bPressed = true;
	this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	setFixedSize(IR_PREVIEW_SIZE_WIDTH, IR_PREVIEW_SIZE_HEIGHT);
	pIrPreviewLabel = new QLabel(this);
	pIrPreviewLabel->resize(IR_PREVIEW_SIZE_WIDTH, IR_PREVIEW_SIZE_HEIGHT);
}


IrPreviewDialog::~IrPreviewDialog()
{
}

void IrPreviewDialog::mouseMoveEvent(QMouseEvent *event)
{
	if (m_bPressed)
		move(event->pos() - m_point + pos());
}

void IrPreviewDialog::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	m_bPressed = false;
}

void IrPreviewDialog::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_bPressed = true;
		m_point = event->pos();
	}
}

void IrPreviewDialog::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	pIrPreviewLabel->setPixmap(m_pixIrImage);
}

