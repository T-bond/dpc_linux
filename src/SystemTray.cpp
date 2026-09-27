#include "SystemTray.h"

#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QJSEngine>
#include <QMenu>
#include <QSystemTrayIcon>

#include "DeviceManager.h"

SystemTray* SystemTray::s_instance = nullptr;

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    DeviceManager *device = DeviceManager::instance();

    m_menu = new QMenu();
    m_profile_menu = m_menu->addMenu(tr("Profile"));
    // not exclusive: each hardware profile has its own profile
    m_profile_group = new QActionGroup(this);
    m_profile_group->setExclusionPolicy(QActionGroup::ExclusionPolicy::None);

    m_lights_off = m_menu->addAction(tr("Lights off"));
    m_lights_off->setCheckable(true);
    m_lights_off->setChecked(device->lightsOff());
    connect(m_lights_off, &QAction::triggered, device, &DeviceManager::setLightsOff);
    connect(device, &DeviceManager::lightsOffChanged, this, [this, device]() {
        m_lights_off->setChecked(device->lightsOff());
    });

    m_menu->addSeparator();
    connect(m_menu->addAction(tr("Exit")), &QAction::triggered, this, &SystemTray::quitRequested);

    m_tray = new QSystemTrayIcon(QIcon(":/image/drevo-power-console.png"), this);
    m_tray->setToolTip(device->deviceName());
    m_tray->setContextMenu(m_menu);
    connect(device, &DeviceManager::connectionChanged, this, [this, device]() {
        m_tray->setToolTip(device->deviceName());
    });
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            emit showWindowRequested();
    });

    connect(device, &DeviceManager::profilesChanged, this, &SystemTray::updateProfiles);
    connect(device, &DeviceManager::currentProfileChanged, this, &SystemTray::updateProfiles);
    connect(device, &DeviceManager::hardwareProfilesChanged, this, &SystemTray::updateProfiles);
    connect(device, &DeviceManager::closeToTrayChanged, this, &SystemTray::updateVisible);
    updateProfiles();
    updateVisible();
}

SystemTray::~SystemTray()
{
    delete m_menu;
    s_instance = nullptr;
}

SystemTray* SystemTray::create(QQmlEngine *, QJSEngine *)
{
    // the instance is owned by main(), not by the QML engine
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

bool SystemTray::available() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void SystemTray::updateVisible()
{
    m_tray->setVisible(DeviceManager::instance()->closeToTray() && available());
}

void SystemTray::updateProfiles()
{
    DeviceManager *device = DeviceManager::instance();

    // deletes the actions, which leaves the group empty
    m_profile_menu->clear();

    // one section per hardware profile; the profile it has is checked
    const QVariantList hardware_profiles = device->hardwareProfiles();
    const QVariantList profiles = device->profiles();
    for (int i = 0; i < hardware_profiles.size(); i++)
    {
        const QVariantMap hardware_profile = hardware_profiles.at(i).toMap();
        m_profile_menu->addSection(hardware_profile.value("name").toString());

        for (const QVariant &profile : profiles)
        {
            const QVariantMap map = profile.toMap();
            if (map.value("hardwareProfile").toInt() != i)
                continue;
            const int id = map.value("id").toInt();

            QAction *action = m_profile_menu->addAction(map.value("name").toString());
            action->setCheckable(true);
            action->setChecked(id == hardware_profile.value("profile").toInt());
            m_profile_group->addAction(action);
            connect(action, &QAction::triggered, device, [device, id]() { device->setCurrentProfile(id); });
        }
    }
}
