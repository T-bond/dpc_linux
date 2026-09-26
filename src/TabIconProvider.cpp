#include "TabIconProvider.h"

#include <QColor>

TabIconProvider::TabIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage TabIconProvider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
    int separator = id.lastIndexOf('/');
    QString sprite_name = id.left(separator);
    QColor color(id.mid(separator + 1));
    if (!color.isValid())
        color = Qt::black;

    QImage sprite(QString(":/image/button/%1.png").arg(sprite_name));
    if (sprite.isNull())
        return QImage();

    // checked frame
    int frame_width = sprite.width() / 4;
    QImage frame = sprite.copy(frame_width * 3, 0, frame_width, sprite.height())
                         .convertToFormat(QImage::Format_ARGB32);

    QImage icon(frame.size(), QImage::Format_ARGB32);
    for (int y = 0; y < frame.height(); y++)
    {
        const QRgb *src = reinterpret_cast<const QRgb*>(frame.constScanLine(y));
        QRgb *dst = reinterpret_cast<QRgb*>(icon.scanLine(y));
        for (int x = 0; x < frame.width(); x++)
        {
            int alpha = (255 - qGray(src[x])) * qAlpha(src[x]) / 255;
            dst[x] = qRgba(color.red(), color.green(), color.blue(), alpha * color.alpha() / 255);
        }
    }

    if (size)
        *size = icon.size();
    if (requested_size.isValid() && requested_size != icon.size())
        return icon.scaled(requested_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return icon;
}
