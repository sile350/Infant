#ifndef IMAGEBUTTON_H
#define IMAGEBUTTON_H

#include <QLabel>

class ImageButton final : public QLabel {
    Q_OBJECT
public:
    explicit ImageButton(QWidget *parent = nullptr);
    void setImagePath(const QString &path);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif
