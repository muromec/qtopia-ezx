/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include <qapplication.h>
#include "qsound.h"
#include "qsound_p.h"
#include <private/qt_mac_p.h>
#include <qhash.h>
#include <qdebug.h>
#import <AppKit/AppKit.h>

void qt_mac_beep() 
{
    NSBeep();
}

#ifndef QT_NO_SOUND

#include <AppKit/NSSound.h>

typedef QHash<QSound *, NSSound const *> Sounds;
static Sounds sounds;


class QAuServerMac : public QAuServer
{
    Q_OBJECT
public:
    QAuServerMac(QObject* parent) : QAuServer(parent) { }
    void play(const QString& filename);
    void play(QSound *s);
    void stop(QSound*);
    bool okay() { return true; }
    using QAuServer::decLoop; // promote to public.
protected:
    NSSound *createNSSound(const QString &filename, QSound *qSound);
};

@interface QMacSoundDelegate : NSObject {
    QSound *qSound; // may be null.
    QAuServerMac* server;
} 
-(id)initWithQSound:(QSound*)sound:(QAuServerMac*)server;
-(void)sound:(NSSound *)sound didFinishPlaying:(BOOL)aBool;
@end

@implementation QMacSoundDelegate
-(id)initWithQSound:(QSound*)s:(QAuServerMac*)serv {
    self = [super init];
    if(self) {
        qSound = s;
        server = serv;
    }
    return self;
}

// Delegate function that gets called each time a sound finishes.
-(void)sound:(NSSound *)sound didFinishPlaying:(BOOL)finishedOk
{
    // qSound is null if this sound was started by play(QString),
    // in which case there is no corresponding QSound object.
    if (qSound == 0) {
        [sound release];
        [self release];
        return;
    }

    // finishedOk is false if the sound cold not be played or
    // if it was interrupted by stop().
    if (finishedOk == false) {
        sounds.remove(qSound);
        [sound release];
        [self release];
        return;
    }

    // Check if the sound should loop "forever" (until stop).
    if (qSound->loops() == -1) { 
        [sound play];
        return;
    }

    const int remainingIterations = server->decLoop(qSound);
    if (remainingIterations > 0) {
        [sound play];
    } else {
        sounds.remove(qSound);
        [sound release];
        [self release];
    }
}
@end

void QAuServerMac::play(const QString &fileName)
{
    QMacCocoaAutoReleasePool pool;
    NSSound * const nsSound = createNSSound(fileName, 0);
    [nsSound play];
}

void QAuServerMac::play(QSound *qSound)
{
    QMacCocoaAutoReleasePool pool;
    NSSound * const nsSound = createNSSound(qSound->fileName(), qSound);
    [nsSound play];
    // Keep track of the nsSound object so we can find it again in stop().
    sounds[qSound] = nsSound;
}

void QAuServerMac::stop(QSound *qSound)
{
    Sounds::const_iterator it = sounds.find(qSound);
    if (it != sounds.end())
        [*it stop];
}

// Creates an NSSound object and installs a "sound finished" callack delegate on it.
NSSound *QAuServerMac::createNSSound(const QString &fileName, QSound *qSound)
{
    NSString *nsFileName = const_cast<NSString *>(reinterpret_cast<const NSString *>(QCFString::toCFStringRef(fileName)));
    NSSound * const nsSound = [[NSSound alloc] initWithContentsOfFile: nsFileName byReference:YES];
    QMacSoundDelegate * const delegate = [[QMacSoundDelegate alloc] initWithQSound:qSound:this];
    [nsSound setDelegate:delegate];
    return nsSound;
}

QAuServer* qt_new_audio_server()
{
    return new QAuServerMac(qApp);
}


#include "qsound_mac.moc"

#endif // QT_NO_SOUND
