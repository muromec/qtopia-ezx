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

#include "confrecorder.h"
#include "mediarecorder.h"
#include "pluginlist.h"

#include <qcombobox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qsettings.h>
#include <qtopiaapplication.h>


// Default quality settings, if not yet set in the configuration.
static QualitySetting DefaultQualities[] = {
    {11025, 1, "audio/x-wav", "pcm"},
    {22050, 2, "audio/x-wav", "pcm"},
    {44100, 2, "audio/x-wav", "pcm"},
    {8000, 1, "audio/x-wav", "pcm"},
};

// Section names within the configuration file.
static const char * const ConfigSections[MaxQualities] = {
    "VoiceQuality", "MusicQuality", "CDQuality", "CustomQuality"
};


ConfigureRecorder::ConfigureRecorder( QualitySetting *_qualities, MediaRecorderPluginList *_plugins, QWidget *parent, Qt::WFlags f )
    : QDialog( parent, f )
    , qualities( _qualities )
    , plugins( _plugins )
    , quality( CustomQuality )
{
    // Create the UI.
    conf = new Ui::ConfigureRecorderBase();
    conf->setupUi( this );
    setObjectName( "settings" );     // To display the correct help page.

    // Load the default quality settings.
    int qual;
    for ( qual = 0; qual < MaxQualities; ++qual ) {
        qualities[qual].frequency = DefaultQualities[qual].frequency;
        qualities[qual].channels = DefaultQualities[qual].channels;
        qualities[qual].mimeType = DefaultQualities[qual].mimeType;
        qualities[qual].formatTag = DefaultQualities[qual].formatTag;
    }

    // Load configuration overrides.
    loadConfig();

    // Populate the list of sample rates.
    conf->sampleRate->addItem( tr("8 kHz") );
    conf->sampleRate->addItem( tr("11 kHz") );
    conf->sampleRate->addItem( tr("22 kHz") );
    conf->sampleRate->addItem( tr("44 kHz") );

    // Populate the list of formats.
    if ( plugins != 0 ) {
        uint numPlugins = plugins->count();
        for ( uint plugin = 0; plugin < numPlugins; ++plugin ) {
            conf->format->addItem( plugins->formatNameAt( plugin ) );
        }
    }

    // Group Quality settings buttons
    QButtonGroup*   qualitybuttonGroup = new QButtonGroup(this);
    qualitybuttonGroup->addButton(conf->voiceQuality, VoiceQuality);
    qualitybuttonGroup->addButton(conf->musicQuality, MusicQuality);
    qualitybuttonGroup->addButton(conf->cdQuality, CDQuality);
    qualitybuttonGroup->addButton(conf->customQuality, CustomQuality);

    connect(qualitybuttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(setQuality(int)));

    // Group channel setting buttons
    QButtonGroup*   channelButtonGroup = new QButtonGroup(this);
    channelButtonGroup->addButton(conf->monoChannels, 0);
    channelButtonGroup->addButton(conf->stereoChannels, 1);

    connect(channelButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(setChannels(int)));

    // Hook up interesting signals.
    connect( conf->sampleRate, SIGNAL(activated(int)),
             this, SLOT(setSampleRate(int)) );
    connect( conf->format, SIGNAL(activated(int)),
             this, SLOT(setFormat(int)) );
}


ConfigureRecorder::~ConfigureRecorder()
{
}


static void copyQualities( QualitySetting *dest, QualitySetting *src, int num )
{
    while ( num > 0 ) {
        dest->frequency = src->frequency;
        dest->channels = src->channels;
        dest->mimeType = src->mimeType;
        dest->formatTag = src->formatTag;
        ++dest;
        ++src;
        --num;
    }
}

void ConfigureRecorder::processPopup()
{
    // Save the current quality settings, in case we have to cancel.
    QualitySetting savedQualities[MaxQualities];
    int savedQuality = quality;
    copyQualities( savedQualities, qualities, MaxQualities );

    // Update the configuration dialog's display with the current state.
    setQuality( quality );

    // Process the dialog.
    if ( QtopiaApplication::execDialog( this ) != QDialog::Accepted) {
        // Copy the saved configuration back.
        copyQualities( qualities, savedQualities, MaxQualities );
        quality = savedQuality;
    }
}


void ConfigureRecorder::setQuality( int index )
{
    // Set the quality value.
    quality = index;
    switch ( quality ) {
        case VoiceQuality:
            conf->voiceQuality->setChecked( true );
            break;

        case MusicQuality:
            conf->musicQuality->setChecked( true );
            break;

        case CDQuality:
            conf->cdQuality->setChecked( true );
            break;

        case CustomQuality:
            conf->customQuality->setChecked( true );
            break;
    }

    // Set the number of channels.
    conf->stereoChannels->setDisabled(true);
    if ( qualities[quality].channels == 1) {
        conf->monoChannels->setChecked( true );
    } else {
        //conf->stereoChannels->setChecked( true );
        conf->monoChannels->setChecked( true );
    }

    // Set the sample rate frequency.
    switch ( qualities[quality].frequency ) {
        case 8000:
            conf->sampleRate->setCurrentIndex( 0 );
            break;

        case 11025:
            conf->sampleRate->setCurrentIndex( 1 );
            break;

        case 22050:
            conf->sampleRate->setCurrentIndex( 2 );
            break;

        case 44100:
            conf->sampleRate->setCurrentIndex( 3 );
            break;
    }

    // Set the format.
    int formatIndex = plugins->indexFromType
            ( qualities[quality].mimeType, qualities[quality].formatTag );
    if( formatIndex >= 0 ) {
        conf->format->setCurrentIndex( formatIndex );
    }
}


void ConfigureRecorder::setChannels( int index )
{
    updateConfig( index + 1, qualities[quality].frequency,
                  qualities[quality].mimeType, qualities[quality].formatTag );
}


void ConfigureRecorder::setSampleRate( int index )
{
    int frequency;

    switch ( index ) {
        case 0:     frequency = 8000; break;
        case 1:     frequency = 11025; break;
        case 2:     frequency = 22050; break;
        default:    frequency = 44100; break;
    }

    updateConfig( qualities[quality].channels, frequency,
                  qualities[quality].mimeType, qualities[quality].formatTag );
}


void ConfigureRecorder::setFormat( int index )
{
    updateConfig( qualities[quality].channels,
                  qualities[quality].frequency,
                  plugins->at( (uint)index )->pluginMimeType(),
                  plugins->formatAt( (uint)index ) );
}


void ConfigureRecorder::updateConfig( int channels, int frequency, const QString& mimeType, const QString& formatTag )
{
    qualities[quality].channels = channels;
    qualities[quality].frequency = frequency;
    qualities[quality].mimeType = mimeType;
    qualities[quality].formatTag = formatTag;
}


void ConfigureRecorder::loadConfig()
{
    QSettings cfg("Trolltech","MediaRecorder");

    cfg.beginGroup( "Options" );
    QString qvalue = cfg.value( "Quality" ).toString();
    if (qvalue == "") {
        quality = VoiceQuality;
    }

    for ( int qual = 0; qual < MaxQualities; ++qual ) {
        if ( qvalue == ConfigSections[qual] ) {
            quality = qual;
        }

        cfg.endGroup();

        cfg.beginGroup( ConfigSections[qual] );

        int value = cfg.value( "Channels" ).toInt();
        if ( value == 1 || value == 2 ) {
            qualities[qual].channels = value;
        }

        value = cfg.value( "Frequency" ).toInt();
        if ( value == 8000 || value == 11025 ||
             value == 22050 || value == 44100 ) {
            qualities[qual].frequency = value;
        }

        QString svalue = cfg.value( "Type" ).toString();
        QString fvalue = cfg.value( "Format" ).toString();
        if ( svalue == QString() ) {
            svalue = qualities[qual].mimeType;
        }
        if ( fvalue == QString() ) {
            fvalue = qualities[qual].formatTag;
        }

        int index = plugins->indexFromType( svalue, fvalue );
        if( index >= 0 ) {
            qualities[qual].mimeType = plugins->at( index )->pluginMimeType();
            qualities[qual].formatTag = plugins->formatAt( index );
        } else {
            qualities[qual].mimeType = svalue;
            qualities[qual].formatTag = fvalue;
        }
    }
}


void ConfigureRecorder::saveConfig()
{
    // Write out only what is different from the defaults, which
    // makes it easier to migrate to new versions of the app
    // that change the defaults to something better.

    QSettings cfg("Trolltech","MediaRecorder");

    cfg.beginGroup( "Options" );
    if ( quality != VoiceQuality ) {
        cfg.setValue( "Quality", ConfigSections[quality] );
    } else {
        cfg.remove( "Quality" );
    }

    for ( int qual = 0; qual < MaxQualities; ++qual ) {
        cfg.endGroup();

        cfg.beginGroup( ConfigSections[qual] );
        cfg.remove("");

        if ( qualities[qual].channels != DefaultQualities[qual].channels ) {
            cfg.setValue( "Channels", qualities[qual].channels );
        }

        if ( qualities[qual].frequency != DefaultQualities[qual].frequency ) {
            cfg.setValue( "Frequency", qualities[qual].frequency );
        }

        if ( qualities[qual].mimeType != DefaultQualities[qual].mimeType ) {
            cfg.setValue( "Type", qualities[qual].mimeType );
        }

        if ( qualities[qual].formatTag != DefaultQualities[qual].formatTag ) {
            cfg.setValue( "Format", qualities[qual].formatTag );
        }
    }
}

