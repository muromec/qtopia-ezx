/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include "appearance.h"
#include "themedview.h"

#include <private/contextkeymanager_p.h>
#include <QtopiaApplication>
#include <qtopiaservices.h>
#include <QMenu>
#include <QTimer>
#include <QPhoneProfileManager>
#include <QFormLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QTranslatableSettings>
#include <QtopiaChannel>
#include <QtopiaIpcEnvelope>
#include <QPixmap>
#include <QDesktopWidget>
#include <QScrollArea>

ThemePreview::ThemePreview(const QString &name, int width, int height)
    : QObject(0), m_name(name), themedView(0),
      m_buttonIcons(true),
      m_width(width),
      m_height(height)
{
}

ThemePreview::~ThemePreview()
{
    delete themedView;
}

void ThemePreview::setXmlFilename(const QString &filename)
{
    if (filename == m_filename)
        return;
    m_filename = filename;
    reload();
}

void ThemePreview::setButton(bool icon, int buttonIndex, QSoftMenuBar::StandardLabel label)
{
    QString buttonName = "button" + QString::number(buttonIndex);
    ThemeTextItem *textItem = (ThemeTextItem *)themedView->findItem(buttonName , ThemedView::Text);
    ThemeImageItem *imageItem = (ThemeImageItem *)themedView->findItem(buttonName, ThemedView::Image);

    ContextKeyManager *mgr = ContextKeyManager::instance();
    if (!icon) {
        if (textItem) {
           textItem->setVisible(true);
           textItem->setText(mgr->standardText(label));
        }
        if (imageItem) {
            imageItem->setVisible(false);
        }
    } else {
        if (textItem) {
            textItem->setVisible(false);
        }
        if (imageItem) {
            imageItem->setVisible(true);
            imageItem->setImage(QPixmap(":icon/" + mgr->standardPixmap(label)));
        }
    }
}

void ThemePreview::reload()
{
    if (m_color.isEmpty() || m_filename.isEmpty())
        return;
    delete themedView;
    themedView = new ThemedView;
    themedView->setPalette(m_palette);
    themedView->loadSource(m_filename);
    themedView->resize(m_width, m_height);

    if (m_name == "contextbar") {
        QList<QSoftMenuBar::StandardLabel> labels;
        if (QApplication::isLeftToRight())
            labels << QSoftMenuBar::Options << QSoftMenuBar::Select << QSoftMenuBar::Back;
        else
            labels << QSoftMenuBar::Back << QSoftMenuBar::Select << QSoftMenuBar::Options;
        for (int i=0; i<labels.size(); i++)
            setButton(m_buttonIcons, i, labels[i]);
    }
}

void ThemePreview::setColor(const QString &color)
{
    if (color == m_color)
        return;
    QSettings scheme(color, QSettings::IniFormat);
    scheme.beginGroup("Colors");
    m_palette.setColor(QPalette::Normal, QPalette::Window, scheme.value("Background").toString());
    m_palette.setColor(QPalette::Normal, QPalette::Button, scheme.value("Button").toString());
    m_palette.setColor(QPalette::Normal, QPalette::Highlight, scheme.value("Highlight").toString());
    m_palette.setColor(QPalette::Normal, QPalette::Text, scheme.value("Text").toString());
    m_palette.setColor(QPalette::Normal, QPalette::Base, scheme.value("Base").toString());
    m_color = color;
    reload();
}

void ThemePreview::requestPreview()
{
    QTimer::singleShot(0, this, SLOT(doPreview()));
}

void ThemePreview::doPreview()
{
    if (!themedView)
        return;
    ThemeItem *page = themedView->findItem(m_name, ThemedView::Page);
    if (page)
        m_preview = QPixmap::grabWidget(themedView, page->rect());
    emit previewReady(m_preview.scaledToWidth((int)(m_width * 0.55), Qt::SmoothTransformation));
}

void ThemePreview::setIconLabel(bool enable)
{
    m_buttonIcons = enable;

    if (!themedView)
        return;

   reload();
}

static QSettings gConfig("Trolltech", "qpe");

// ThemeInfo ----------------------------------------------------
ThemeInfo::ThemeInfo() {}
void ThemeInfo::setThemeName(const QString &n) { name = n; }
const QString & ThemeInfo::themeName() { return name; }
void ThemeInfo::setStyleName( const QString sn ) { style = sn; }
const QString & ThemeInfo::styleName() const { return style; }
void ThemeInfo::setThemeFileName( const QString fn ) { themeFile = fn; }
const QString & ThemeInfo::themeFileName() const { return themeFile; }
void ThemeInfo::setDecorationFileName( const QString fn ) { decorationFile = fn; }
const QString & ThemeInfo::decorationFileName() const { return decorationFile; }
void ThemeInfo::setServerWidget(const QString &sw) { srvWidget = sw; }
QString ThemeInfo::serverWidget() const { return srvWidget; }
void ThemeInfo::setColorSchemes(const QStringList &cs) { colSchemes = cs; }
QStringList ThemeInfo::colorSchemes() const { return colSchemes; }
void ThemeInfo::setBackgroundImages(const QStringList &bg) { bgImages = bg; }
QStringList ThemeInfo::backgroundImages() const { return bgImages; }

// AppearanceSettings ----------------------------------------------------
AppearanceSettings::AppearanceSettings( QWidget* parent, Qt::WFlags fl )
    : QDialog(parent, fl),
    mIsStatusView(false), mIsFromActiveProfile(false)
{
    setupUi();
    readThemeSettings();
    readColorSchemeSettings();

    // Populate the first combo box (theme).
    populate();

    // Select the theme to populate the other combo boxes.
    // Note that they are not connected yet, so no preview is requested.
    themeSelected(mActiveThemeIndex);

    // Connect the combo boxes.
    connect( mThemeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(themeSelected(int)) );
    connect( mColorCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(colorSelected(int)) );
    connect( mBgCombo, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(backgroundSelected(QString)) );
    connect( mLabelCkBox, SIGNAL(toggled(bool)),
            this, SLOT(labelToggled(bool)) );

    connect( qApp, SIGNAL(appMessage(QString,QByteArray)),
        this, SLOT(receive(QString,QByteArray)) );

    // Re-select the theme to request the previews.
    themeSelected(mActiveThemeIndex);

    QSoftMenuBar::menuFor( this )->addAction
        ( QIcon( ":icon/Note" ), tr( "Add to current profile" ), this, SLOT(pushSettingStatus()) );
    QSoftMenuBar::menuFor( this )->addAction
        ( QIcon( ":image/homescreen/homescreen" ), tr( "Homescreen Settings..." ), this, SLOT(openHomescreenSettings()) );
}

AppearanceSettings::~AppearanceSettings()
{
}

void AppearanceSettings::populate()
{
    // current theme & color
    gConfig.beginGroup( "Appearance" );
    mActiveTheme = gConfig.value( "Theme", "qtopia.conf" ).toString();
    mActiveColor = gConfig.value( "Scheme", "Qtopia" ).toString();
    mActiveBackground = gConfig.value( "BackgroundImage", "" ).toString();
    gConfig.endGroup();

    // current label type
    gConfig.beginGroup( "ContextMenu" );
    mActiveLabelType = (QSoftMenuBar::LabelType)gConfig.value( "LabelType", QSoftMenuBar::TextLabel ).toInt();
    gConfig.endGroup();
    mLabelCkBox->setCheckState( mActiveLabelType == QSoftMenuBar::IconLabel ? Qt::Checked : Qt::Unchecked );
    labelToggled( mActiveLabelType == QSoftMenuBar::IconLabel );

    // current server widgets
    QSettings cfg( "Trolltech", "ServerWidgets" );
    cfg.beginGroup( "Mapping" );
    if (!cfg.childKeys().isEmpty())
        mServerWidgets = cfg.value("Default", "Phone").toString();

    // populate theme combo box
    foreach ( ThemeInfo theme, mThemes ) {
        mThemeCombo->addItem( theme.themeName() );
        if ( theme.themeFileName() == mActiveTheme ) {
            mActiveThemeIndex = mThemeCombo->count() - 1;
            mThemeCombo->setCurrentIndex(mActiveThemeIndex);
        }
    }
    mContextPreview->setIconLabel(mActiveLabelType == QSoftMenuBar::IconLabel);
}

void AppearanceSettings::accept()
{
    QPhoneProfileManager profileManager;
    QPhoneProfile activeProfile = profileManager.activeProfile();

    if ( !mIsStatusView ) { // normal appearance setting operation
        QPhoneProfile::Setting setting = activeProfile.applicationSetting("appearance");
        if ( setting != QPhoneProfile::Setting() )
            pushSettingStatus();
    } else { // status view from profiles
        // save current status to the profile
        pushSettingStatus();
    }

    applyStyle();

    QDialog::accept();
}

void AppearanceSettings::applyStyle()
{
    if (mThemeCombo->currentIndex() < 0 ||
            mThemeCombo->currentIndex() >= mThemes.count()) {
        return;
    }

    const ThemeInfo &theme = mThemes[mThemeCombo->currentIndex()];

    QString themeFile = findFile(QLatin1String("etc/themes/") + mActiveTheme);
    QSettings themeCfg(themeFile, QSettings::IniFormat);
    themeCfg.beginGroup(QLatin1String("Theme"));
    QString activeIconPath = themeCfg.value(QLatin1String("IconPath")).toString();

    // If the server widgets or IconPath changes we need to restart for
    // all changes to be visible.
    bool needRestart = mServerWidgets != theme.serverWidget()
                        || activeIconPath != theme.iconPath;

    if (needRestart) {
        int ret = QMessageBox::warning( this, tr( "Restart?" ),
                tr( "Device will be restarted for theme to be fully applied.<br>Apply Now?" ),
                QMessageBox::Yes, QMessageBox::No );
        if ( ret != QMessageBox::Yes )
            return;
    }

    bool themeChanged = ( theme.themeFileName() != mActiveTheme );
    bool colorSchemeChanged = ( mColorCombo->itemData(mColorCombo->currentIndex()).toString() != mActiveColor );

    if (themeChanged)
        writeThemeSettings(theme);
    if (colorSchemeChanged)
        writeColorSchemeSettings();
    if (themeChanged || colorSchemeChanged) {
        QtopiaChannel::send("QPE/System", "applyStyle()");
        if (themeChanged)
            QtopiaChannel::send("QPE/System", "applyStyleSplash()");
        else
            QtopiaChannel::send("QPE/System", "applyStyleNoSplash()");
    }

    if ( mBgCombo->currentText() != mActiveBackground )
        applyBackgroundImage();
    if ( ( mLabelCkBox->isChecked()
                ? QSoftMenuBar::IconLabel
                : QSoftMenuBar::TextLabel ) != mActiveLabelType ) {
        applySoftKeyLabels();
    }

    if ( !theme.serverWidget().isEmpty() ) {
        QSettings cfg( "Trolltech", "ServerWidgets" );
        cfg.beginGroup( "Mapping" );
        cfg.remove(""); //delete all entries in current grp
        cfg.setValue("Default", theme.serverWidget());

        if (needRestart) {
            QtopiaIpcEnvelope env( "QPE/System", "restart()" );
            QtopiaApplication::quit();
        }
    }
}

void AppearanceSettings::setupUi()
{
    // title string
    setWindowTitle( tr( "Appearance" ) );

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);

    QWidget *appearance = new QWidget;

    QScrollArea *appearanceWrapper = new QScrollArea;
    appearanceWrapper->setFocusPolicy(Qt::NoFocus);
    appearanceWrapper->setFrameStyle(QFrame::NoFrame);
    appearanceWrapper->setWidget(appearance);
    appearanceWrapper->setWidgetResizable(true);
    appearanceWrapper->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); 

    // create layout
    QFormLayout *formLayout = new QFormLayout(appearance);

    // create widgets
    mThemeCombo = new QComboBox( this );
    mColorCombo = new QComboBox( this );
    mBgCombo = new QComboBox( this );

    // add widgets
    formLayout->addRow( tr( "Theme" ), mThemeCombo );
    formLayout->addRow( tr( "Color" ), mColorCombo );
    formLayout->addRow( tr( "Background" ), mBgCombo );

    // label option checkbox
    mLabelCkBox = new QCheckBox( tr( "Use icons for soft keys" ), this );
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget( mLabelCkBox, 0, Qt::AlignHCenter );
    formLayout->addRow( hLayout );

    // preview
    mGroupBox = new QGroupBox(this);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setMargin(10);
    mGroupBox->setLayout(layout);
    mGroupBox->hide();

    QVBoxLayout *previewLayout = new QVBoxLayout();
    previewLayout->setSpacing(0);

    QDesktopWidget *desktop = QApplication::desktop();
    int width = desktop->screenGeometry(desktop->primaryScreen()).width();
    int height = desktop->screenGeometry(desktop->primaryScreen()).height();

    mTitlePreview = new ThemePreview("title", width, height);
    connect(mTitlePreview, SIGNAL(previewReady(QPixmap)), this, SLOT(setTitlePixmap(QPixmap)));

    mContextPreview = new ThemePreview("contextbar", width, height);
    connect(mContextPreview, SIGNAL(previewReady(QPixmap)), this, SLOT(setContextPixmap(QPixmap)));

    previewLayout->addWidget( &mTitleLabel, 0, Qt::AlignHCenter );
    previewLayout->addWidget( &mContextLabel, 0, Qt::AlignHCenter );

    layout->addStretch(1);
    layout->addLayout(previewLayout);
    layout->addWidget(&mBackgroundLabel);
    layout->addStretch(1);
    formLayout->addRow( mGroupBox );

    l->addWidget(appearanceWrapper);
}

QString AppearanceSettings::findFile(const QString &file)
{
    QStringList instPaths = Qtopia::installPaths();
    foreach (QString path, instPaths) {
        QString themeDataPath( path + file );
        if (QFile::exists(themeDataPath)) {
            return themeDataPath;
        }
    }

    return QString();
}

bool AppearanceSettings::readTheme(const QString &configFileName, ThemeInfo &theme)
{
    QTranslatableSettings themeConfig(configFileName, QSettings::IniFormat);
    // Ensure that we only provide valid theme choices.
    if ( themeConfig.status() != QSettings::NoError ) {
        // failed to read theme.conf. try next one
        qLog(UI) << "Failed to read, ignore" << configFileName.toLocal8Bit().data();
        return false;
    } else {
        themeConfig.beginGroup( "Theme" ); // No tr
        QString styleName = themeConfig.value("Style", "Qtopia").toString();
        QString serverWidgetName = themeConfig.value("ServerWidget", "Phone").toString();
        QStringList colSchemes = themeConfig.value("ColorScheme").toString().split( "|", QString::SkipEmptyParts );
        QStringList bg = themeConfig.value("Backgrounds").toString().split( "|", QString::SkipEmptyParts );

//          FIXME: We don't check the validity of the theme for performance issues...
//             QStringList list;
//             list << "TitleConfig" << "HomeConfig"
//                 << "ContextConfig" << "DialerConfig"
//                 << "CallScreenConfig" << "DecorationConfig"
//                 << "SecondaryTitleConfig" << "SecondaryHomeConfig"
//                 << "BootChargerConfig";
//             for (QStringList::const_iterator it = list.constBegin(); it != list.constEnd(); it++){
//                 if (themeConfig.contains(*it)){
//                     QFileInfo info(Qtopia::qtopiaDir() + "etc/themes/" + themeConfig.value(*it).toString());
//                     if (!info.isFile()){
//                         qLog(UI) << "QSettings entry" << (*it).toLocal8Bit().data()
//                                 << "in" << configFileName.toLocal8Bit().data()
//                                 << "points to non-existant file" << info.filePath().toLocal8Bit().data();
//                         valid = false;
//                         break;
//                     }
//                 }
//             }
//             if ( !valid ) {
//                 // this theme does not have all the necessary files, try next one
//                 qLog(UI) << "Missing files, ignore" << configFileName.toLocal8Bit().data();
//                 continue;
//             }

        if ( !themeConfig.contains( "Name" ) ) {
            // this theme does not have a name, try next one
            qLog(UI) << "No name, ignore" << configFileName.toLocal8Bit().data();
            return false;
        }

        theme.setThemeName( themeConfig.value( "Name" ).toString() );
        theme.setStyleName( styleName );
        theme.setDecorationFileName( themeConfig.value("DecorationConfig").toString() );
        theme.setServerWidget( serverWidgetName );
        theme.setColorSchemes( colSchemes );
        theme.setBackgroundImages( bg );
        theme.extendedFocusHighlight = themeConfig.value("ExtendedFocusHighlight", "1").toString();
        theme.formStyle = themeConfig.value("FormStyle", "QtopiaDefaultStyle").toString();
        theme.popupShadows = themeConfig.value("PopupShadows", "0").toString();
        theme.hideMenuIcons = themeConfig.value("HideMenuIcons", "0").toString();
        theme.fullWidthMenu = themeConfig.value("FullWidthMenu", "0").toString();
        theme.iconPath = themeConfig.value("IconPath").toString();

        return true;
    }
}

void AppearanceSettings::readThemeSettings()
{
    QStringList instPaths = Qtopia::installPaths();
    foreach (QString path, instPaths) {
        QString themeDataPath( path + "etc/themes/" );
        QDir dir;
        if ( !dir.exists( themeDataPath ) ) {
            qLog(UI) << "Theme style configuration path not found" << themeDataPath.toLocal8Bit().data(); // No tr
            continue;
        }

        // read theme.conf files
        dir.setPath( themeDataPath );
        dir.setNameFilters( QStringList( "*.conf" )); // No tr

        for (int index = 0; index < (int)dir.count(); index++) {
            QString configFileName = themeDataPath + dir[index];
            ThemeInfo theme;
            if (readTheme(configFileName, theme)) {
                theme.setThemeFileName(dir[index]);
                mThemes.append( theme );
            }
        }
    }
}

void AppearanceSettings::readColorSchemeSettings()
{
    QStringList instPaths = Qtopia::installPaths();
    foreach (QString path, instPaths) {
        QString colorSchemePath( path + "etc/colors/" );
        QDir dir;
        if ( !dir.exists( colorSchemePath ) ) {
            qLog(UI) << "Color scheme configuration path not found" << colorSchemePath.toLocal8Bit().data(); // No tr
            continue;
        }

        dir.setPath( colorSchemePath );
        dir.setNameFilters( QStringList( "*.scheme" )); // No tr

        for (int index = 0; index < (int)dir.count(); index++) {
            QString name = dir[index].left( dir[index].indexOf( ".scheme" ) );
            mColorListIDs.append(name);
        }
    }
}

void AppearanceSettings::themeSelected( int index )
{
    if (index >= mThemes.count())
        return;

    QString dir = mThemes[index].themeFileName();
    dir.chop(5);

    QString themeFile = findFile(QLatin1String("etc/themes/") + dir + "/title.xml");
    mTitlePreview->setXmlFilename(themeFile);
    themeFile = findFile(QLatin1String("etc/themes/") + dir + "/context.xml");
    mContextPreview->setXmlFilename(themeFile);

    mBgCombo->clear();
    if ( mThemes[index].backgroundImages().count() ) {
        foreach ( QString bg, mThemes[index].backgroundImages() ) {
            mBgCombo->addItem( bg );
            if ( bg == mActiveBackground )
                mBgCombo->setCurrentIndex( mBgCombo->count() - 1 );
        }
    }

    if ( !mColorListIDs.count() )
        return;

    mColorCombo->clear();

    if ( mThemes[index].colorSchemes().count() ) {
        foreach ( QString colorId, mThemes[index].colorSchemes() ) {
            QString col = colorId.left( colorId.indexOf( ".scheme" ) );
            if ( mColorListIDs.contains( col ) ) {
                QString schemeFile = findFile("etc/colors/" + col + ".scheme");
                QTranslatableSettings scheme(schemeFile, QSettings::IniFormat);
                mColorCombo->addItem( scheme.value("Global/Name", col).toString(), col );
                if ( col == mActiveColor )
                    mColorCombo->setCurrentIndex( mColorCombo->count() - 1 );
            }
        }
        return;
    }

    // theme doesn't have preference, show all color schemes
    int defaultIdx = 0;
    foreach ( QString colorId, mColorListIDs ) {
        QString schemeFile = findFile("etc/colors/" + colorId + ".scheme");
        QTranslatableSettings scheme(schemeFile, QSettings::IniFormat);
        mColorCombo->addItem( scheme.value("Global/Name", colorId).toString(), colorId );
        if ( colorId == mActiveColor )
            mColorCombo->setCurrentIndex( mColorCombo->count() - 1 );
        if ( colorId == "Qtopia" )
            defaultIdx = mColorCombo->count() - 1;
    }
    if ( !mColorCombo->currentIndex() )
        mColorCombo->setCurrentIndex( defaultIdx );
}

void AppearanceSettings::colorSelected( int index )
{
    QString text = mColorCombo->itemData( index ).toString();
    if (!text.isNull()) {
        QString schemeFile = findFile("etc/colors/" + text + ".scheme");
        mTitlePreview->setColor(schemeFile);
        mContextPreview->setColor(schemeFile);
        mTitlePreview->requestPreview();
        mContextPreview->requestPreview();
    }
}

void AppearanceSettings::labelToggled( bool toggled )
{
    mContextPreview->setIconLabel(toggled);
    mContextPreview->requestPreview();
}

void AppearanceSettings::backgroundSelected( const QString &text )
{
    if (text.isNull()) {
        mBackgroundLabel.setPixmap(QPixmap());
        return;
    }

    QString themeFileName;

    foreach(ThemeInfo info, mThemes) {
        if (info.themeName() == mThemeCombo->currentText())
            themeFileName = info.themeFileName();
    }
    themeFileName.chop(5);

    QDesktopWidget *desktop = QApplication::desktop();
    int width = desktop->screenGeometry(desktop->primaryScreen()).width();
    QString filename = findFile("pics/themes/" + themeFileName + '/' + text + ".png");
    QPixmap back(filename);
    mBackgroundLabel.setPixmap(back.scaledToWidth((int)(width * 0.2), Qt::SmoothTransformation));
}

void AppearanceSettings::setTitlePixmap(const QPixmap &pixmap)
{
    mTitleLabel.setPixmap(pixmap);
    mGroupBox->show();
}

void AppearanceSettings::setContextPixmap(const QPixmap &pixmap)
{
    mContextLabel.setPixmap(pixmap);
    mGroupBox->show();
}

void AppearanceSettings::writeThemeSettings(const ThemeInfo &theme)
{
    gConfig.beginGroup( "Appearance" );
    if (!theme.themeFileName().isEmpty() && (theme.themeFileName() != mThemeCombo->currentText()) ){
        gConfig.setValue("Style", theme.styleName());
        gConfig.setValue("Theme", theme.themeFileName());
        gConfig.setValue("DecorationTheme", theme.decorationFileName());
        qLog(UI) << "Write config theme select" << theme.styleName().toLatin1().data() <<
            mThemeCombo->currentText().toLatin1().data();
    } else {
        QString s = theme.themeFileName().isEmpty() ? mThemeCombo->currentText() : theme.themeFileName();
        qLog(UI) << "Write simple config theme select" << theme.styleName().toLatin1().data() <<
                mThemeCombo->currentText().toLatin1().data();
        gConfig.setValue( "Style", s );
        gConfig.setValue( "Theme", "");
        gConfig.setValue( "DecorationTheme", "");
    }
    gConfig.endGroup();

    gConfig.beginGroup("Style");
    gConfig.setValue("ExtendedFocusHighlight", theme.extendedFocusHighlight);
    gConfig.setValue("FormStyle", theme.formStyle);
    gConfig.setValue("PopupShadows", theme.popupShadows);
    gConfig.setValue("HideMenuIcons", theme.hideMenuIcons);
    gConfig.setValue("FullWidthMenu", theme.fullWidthMenu);
    gConfig.endGroup();

    gConfig.sync();
}

void AppearanceSettings::applyBackgroundImage()
{
    QString s = mBgCombo->currentText();
    gConfig.beginGroup( "Appearance" );
    gConfig.setValue( "BackgroundImage", s );
    gConfig.endGroup();
    gConfig.sync();
    QtopiaChannel::send("QPE/System", "applyHomeScreenImage()");
    if (QtopiaApplication::desktop()->numScreens() > 1)
        QtopiaChannel::send("QPE/System", "applySecondaryBackgroundImage()");
}

void AppearanceSettings::writeColorSchemeSettings()
{
    QString s = mColorCombo->itemData( mColorCombo->currentIndex() ).toString();
    gConfig.beginGroup( "Appearance" );
    gConfig.setValue( "Scheme", s );

    QString schemeFile = findFile("etc/colors/" + s + ".scheme");
    QSettings scheme(schemeFile, QSettings::IniFormat);
    if (scheme.status()==QSettings::NoError){
        scheme.beginGroup("Colors");
        QString color = scheme.value( "Background", "#EEEEEE" ).toString();
        gConfig.setValue( "Background", color );
        QString alpha = scheme.value( "Background_alpha", "64" ).toString();
        gConfig.setValue( "Background_alpha", alpha );
        color = scheme.value( "Foreground", "#000000" ).toString();
        gConfig.setValue( "Foreground", color );
        color = scheme.value( "Button", "#F0F0F0" ).toString();
        gConfig.setValue( "Button", color );
        alpha = scheme.value( "Button_alpha", "176" ).toString();
        gConfig.setValue( "Button_alpha", alpha );
        color = scheme.value( "Highlight", "#8BAF31" ).toString();
        gConfig.setValue( "Highlight", color );
        alpha = scheme.value( "Highlight_alpha", "176" ).toString();
        gConfig.setValue( "Highlight_alpha", alpha );
        color = scheme.value( "HighlightedText", "#FFFFFF" ).toString();
        gConfig.setValue( "HighlightedText", color );
        color = scheme.value( "Text", "#000000" ).toString();
        gConfig.setValue( "Text", color );
        color = scheme.value( "ButtonText", "#000000" ).toString();
        gConfig.setValue( "ButtonText", color );
        color = scheme.value( "ButtonText_disabled", "" ).toString();
        gConfig.setValue( "ButtonText_disabled", color );
        alpha = scheme.value( "ButtonText_disabled_alpha", 255 ).toString();
        gConfig.setValue( "ButtonText_disabled_alpha", alpha );
        color = scheme.value( "Base", "#FFFFFF" ).toString();
        gConfig.setValue( "Base", color );
        alpha = scheme.value( "Base_alpha", "176" ).toString();
        gConfig.setValue( "Base_alpha", alpha );
        color = scheme.value( "AlternateBase", "#CBEF71" ).toString();
        gConfig.setValue( "AlternateBase", color );
        alpha = scheme.value( "AlternateBase_alpha", "176" ).toString();
        gConfig.setValue( "AlternateBase_alpha", alpha );
        color = scheme.value( "Text_disabled", "" ).toString();
        gConfig.setValue("Text_disabled", color);
        alpha = scheme.value( "Text_disabled_alpha", 255 ).toString();
        gConfig.setValue("Text_disabled_alpha", alpha);
        color = scheme.value( "Foreground_disabled", "" ).toString();
        gConfig.setValue("Foreground_disabled", color);
        alpha = scheme.value( "Foreground_disabled_alpha", 255 ).toString();
        gConfig.setValue("Foreground_disabled_alpha", alpha);
        color = scheme.value( "Shadow", "" ).toString();
        gConfig.setValue("Shadow", color);
        color = scheme.value( "Link", "#0000FF" ).toString();
        gConfig.setValue( "Link", color );
        color = scheme.value( "LinkVisited", "#FF00FF" ).toString();
        gConfig.setValue( "LinkVisited", color );
    }
    gConfig.endGroup();
    gConfig.sync();
}

void AppearanceSettings::applySoftKeyLabels()
{
    gConfig.beginGroup( "ContextMenu" );
    gConfig.setValue( "LabelType", mLabelCkBox->isChecked() ? (int)QSoftMenuBar::IconLabel : (int)QSoftMenuBar::TextLabel );
    gConfig.endGroup();
    gConfig.sync();

    QtopiaChannel::send("QPE/System", "updateContextLabels()");
}

void AppearanceSettings::pushSettingStatus()
{
    QtopiaServiceRequest e( "SettingsManager", "pushSettingStatus(QString,QString,QString)" );
    e << QString( "appearance" ) << QString( windowTitle() ) << status();
    e.send();
}

void AppearanceSettings::pullSettingStatus()
{
    QtopiaServiceRequest e( "SettingsManager", "pullSettingStatus(QString,QString,QString)" );
    e << QString( "appearance" ) << QString( windowTitle() ) << status();
    e.send();
}

QString AppearanceSettings::status()
{
    QString status;
    status += QString::number( mThemeCombo->currentIndex() ) + ",";
    status += QString::number( mColorCombo->currentIndex() ) + ",";
    status += QString::number( mBgCombo->currentIndex() ) + ",";
    status += QString::number( mLabelCkBox->isChecked() ) + ",";
    return status;
}

void AppearanceSettings::setStatus( const QString details )
{
    QStringList s = details.split( ',' );
    mThemeCombo->setCurrentIndex( s.at( 0 ).toInt() );
    mColorCombo->setCurrentIndex( s.at( 1 ).toInt() );
    mBgCombo->setCurrentIndex( s.at( 2 ).toInt() );
    mLabelCkBox->setCheckState( s.at( 3 ).toInt() ? Qt::Checked : Qt::Unchecked );
}

void AppearanceSettings::receive( const QString& msg, const QByteArray& data )
{
    QDataStream ds( data );
    if ( msg == "Settings::setStatus(bool,QString)" ) {
        // must show widget to keep running
        QtopiaApplication::instance()->showMainWidget();
        mIsStatusView = true;
        QSoftMenuBar::removeMenuFrom( this, QSoftMenuBar::menuFor( this ) );
        QSoftMenuBar::menuFor(this);
        QString details;
        ds >> mIsFromActiveProfile;
        ds >> details;
        setStatus( details );
        applyStyle();
    } else if ( msg == "Settings::activateSettings(QString)" ) {
        hide();
        QString details;
        ds >> details;
        setStatus( details );
        applyStyle();
    } else if ( msg == "Settings::pullSettingStatus()" ) {
        hide();
        pullSettingStatus();
    } else if ( msg == "Settings::activateDefault()" ) {
        hide();
        int i = 0;
        foreach ( ThemeInfo theme, mThemes ) {
            if ( theme.themeFileName() == "qtopia.conf" ) {
                mThemeCombo->setCurrentIndex( i );
                break;
            }
            i++;
        }
        for ( i = 0; i < mColorCombo->count(); i++ ) {
            if ( mColorCombo->itemData( i ).toString() == "Qtopia" ) {
                mColorCombo->setCurrentIndex( i );
                break;
            }
        }
        mLabelCkBox->setCheckState( Qt::Checked );
        applyStyle();
    }
}

void AppearanceSettings::openHomescreenSettings()
{
    QtopiaIpcEnvelope env( "QPE/Application/homescreen", "HomescreenSettings::configure()" );
}

