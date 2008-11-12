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

#ifndef APPEARANCESETTINGS_H
#define APPEARANCESETTINGS_H

#include <QDialog>
#include <QMap>
#include <QSoftMenuBar>
#include <QLabel>

class QWaitWidget;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QPixmap;
class ThemedView;
class ThemeTextItem;
class ThemeImageItem;

class ThemePreview : public QObject
{
  Q_OBJECT

  public:
    ThemePreview(const QString &name, int width, int height);
    ~ThemePreview();
    void setXmlFilename(const QString &filename);
    void setButton(bool icon, int buttonIndex, QSoftMenuBar::StandardLabel label);
    void reload();
    void setColor(const QString &color);
    void setIconLabel(bool enable);
    void requestPreview();

  signals:
    void previewReady(const QPixmap &);

  private slots:
    void doPreview();

  private:
    QString     m_name;
    ThemedView  *themedView;
    bool        m_buttonIcons;
    int         m_width;
    int         m_height;
    QString     m_filename;
    QString     m_color;
    QPalette    m_palette;
    QPixmap     m_preview;
};

class ThemeInfo
{
public:
    ThemeInfo();
    void setThemeName( const QString &n );
    const QString & themeName();
    void setStyleName( const QString sn );
    const QString &styleName() const;
    void setThemeFileName( const QString fn );
    const QString &themeFileName() const;
    void setDecorationFileName( const QString fn );
    const QString &decorationFileName() const;
    void setServerWidget(const QString &);
    QString serverWidget() const;
    void setColorSchemes(const QStringList &);
    QStringList colorSchemes() const;
    void setBackgroundImages(const QStringList&);
    QStringList backgroundImages() const;

private:
    QString name;
    QString style;
    QString themeFile;
    QString decorationFile;
    QString srvWidget;
    QStringList colSchemes;
    QStringList bgImages;
    QString extendedFocusHighlight;
    QString formStyle;
    QString popupShadows;
    QString hideMenuIcons;
    QString fullWidthMenu;
    QString iconPath;

friend class AppearanceSettings;
};

class AppearanceSettings : public QDialog
{
    Q_OBJECT

public:
    AppearanceSettings( QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~AppearanceSettings();

protected:
    void accept();

protected slots:
    void populate();
    void themeSelected( int index );
    void colorSelected( int index );
    void backgroundSelected( const QString &text );
    void labelToggled( bool toggled );
    void receive( const QString& msg, const QByteArray& data );
    void pushSettingStatus();
    void setTitlePixmap(const QPixmap &);
    void setContextPixmap(const QPixmap &);
    void openHomescreenSettings();

private:
    void setupUi();
    QString findFile(const QString &file);
    bool readTheme(const QString &configFileName, ThemeInfo &theme);
    void readThemeSettings();
    void readColorSchemeSettings();
    void applyStyle();
    void writeThemeSettings(const ThemeInfo &theme);
    void writeColorSchemeSettings();
    void applyBackgroundImage();
    void applySoftKeyLabels();
    QString status();
    void setStatus( const QString details );
    void pullSettingStatus();

private:
    QComboBox       *mThemeCombo;
    QComboBox       *mColorCombo;
    QComboBox       *mBgCombo;
    QCheckBox       *mLabelCkBox;
    QGroupBox       *mGroupBox;
    QLabel          mTitleLabel;
    QLabel          mContextLabel;
    QLabel          mBackgroundLabel;
    ThemePreview    *mTitlePreview;
    ThemePreview    *mContextPreview;

    QList<ThemeInfo> mThemes;
    QStringList mColorListIDs;
    QString mServerWidgets;
    QString mActiveTheme, mActiveColor, mActiveBackground;
    int mActiveThemeIndex;
    QSoftMenuBar::LabelType mActiveLabelType;

    bool mIsStatusView, mIsFromActiveProfile;
};

#endif // APPEARANCESETTINGS_H
