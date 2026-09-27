#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QQuickImageProvider>

// "image://icon/<rrggbb>/<resource path>": a resource icon drawn in one color
//
// Plain icons (image/icon/) keep their shape (alpha) and get the color. The tab button sprites
// (image/button/, 4 frames side by side) use their checked frame, a dark icon on a white
// background; its darkness becomes the alpha of the returned icon.
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requested_size) override;
};

#endif // ICONPROVIDER_H
