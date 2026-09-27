#include "IconProvider.h"

#include <QColor>

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
    int separator = id.indexOf('/');
    QColor color('#' + id.left(separator));
    if (!color.isValid())
        color = Qt::black;
    QString path = id.mid(separator + 1);

    QImage source(":/" + path);
    if (source.isNull())
        return QImage();

    bool sprite = path.startsWith("image/button/");
    if (sprite)
    {
        // checked frame
        int frame_width = source.width() / 4;
        source = source.copy(frame_width * 3, 0, frame_width, source.height());
    }
    source = source.convertToFormat(QImage::Format_ARGB32);

    QImage icon(source.size(), QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); y++)
    {
        const QRgb *src = reinterpret_cast<const QRgb*>(source.constScanLine(y));
        QRgb *dst = reinterpret_cast<QRgb*>(icon.scanLine(y));
        for (int x = 0; x < source.width(); x++)
        {
            int alpha = sprite ? (255 - qGray(src[x])) * qAlpha(src[x]) / 255 : qAlpha(src[x]);
            dst[x] = qRgba(color.red(), color.green(), color.blue(), alpha * color.alpha() / 255);
        }
    }

    if (size)
        *size = icon.size();
    if (requested_size.isValid() && requested_size != icon.size())
        return icon.scaled(requested_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return icon;
}
