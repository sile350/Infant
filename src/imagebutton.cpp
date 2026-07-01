#include "imagebutton.h"

#include <QMouseEvent>
#include <QPixmap>

ImageButton::ImageButton(QWidget *parent) : QLabel(parent) {
    setScaledContents(true);
    setStyleSheet(QStringLiteral("background: transparent; border: none;"));
}

void ImageButton::setImagePath(const QString &path) {
    setPixmap(QPixmap(path));
}

void ImageButton::mousePressEvent(QMouseEvent *event) {
    emit clicked();
    QLabel::mousePressEvent(event);
}
