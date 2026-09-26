#ifndef TABICONPROVIDER_H
#define TABICONPROVIDER_H

#include <QQuickImageProvider>

// "image://tabicon/<sprite name>/<color>": icon of a tab button sprite in any color
//
// The checked frame of the sprites (4th frame of image/button/<sprite name>.png) is a dark icon
// on a white background; its darkness becomes the alpha of the returned icon, so the checked tab
// can be drawn on the theme's panel color.
class TabIconProvider : public QQuickImageProvider
{
public:
    TabIconProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requested_size) override;
};

#endif // TABICONPROVIDER_H
