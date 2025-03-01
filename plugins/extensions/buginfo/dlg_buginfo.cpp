/*
 * SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dlg_buginfo.h"

#include <klocalizedstring.h>
#include <kis_debug.h>
#include <opengl/kis_opengl.h>
#include <KritaVersionWrapper.h>
#include <QSysInfo>
#include <kis_image_config.h>
#include <QScreen>
#include <QClipboard>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QScreen>

#include <KoFileDialog.h>
#include <QMessageBox>


#include <KisPortingUtils.h>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif

DlgBugInfo::DlgBugInfo(QWidget *parent, KoDialog::ButtonCodes customButtons)
    : KoDialog(parent)
    , m_page(new WdgBugInfo(this))
{
    setCaption(i18n("Please paste this information in your bug report"));

    setButtons(User1 | User2 | Ok | customButtons);
    setButtonText(User1, i18n("Copy to clipboard"));
    setButtonText(User2, i18n("Save to file"));
    setDefaultButton(Ok);

    Q_CHECK_PTR(m_page);

    setMainWidget(m_page);

    connect(this, &KoDialog::user1Clicked, this, [this](){
        QGuiApplication::clipboard()->setText(m_page->txtBugInfo->toPlainText());
        m_page->txtBugInfo->selectAll(); // feedback
    });

    connect(this, &KoDialog::user2Clicked, this, &DlgBugInfo::saveToFile);

}

void DlgBugInfo::initialize()
{
    initializeText();
    setCaption(captionText());
}

void DlgBugInfo::initializeText()
{
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QSettings kritarc(configPath + QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);

    QString info = infoText(kritarc);

    m_page->txtBugInfo->setText(info);
}

void DlgBugInfo::saveToFile()
{
    KoFileDialog dlg(this, KoFileDialog::SaveFile, i18n("Save to file"));
    dlg.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultNewFileName());
    dlg.setMimeTypeFilters(QStringList("text/plain"), "text/plain");
    QString filename = dlg.filename();

    if (filename.isEmpty()) {
        return;
    } else {

        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, i18n("Unable to open file"),
                file.errorString());
            return;
        }

        QTextStream out(&file);
        KisPortingUtils::setUtf8OnStream(out);
        QString originalLogFileName = originalFileName();
        if (originalLogFileName.isEmpty() && QFileInfo(originalLogFileName).exists()) {
            QFile src(originalLogFileName);
            out << src.readAll();
            src.close();
        } else {
            out << m_page->txtBugInfo->toPlainText();
        }
        file.close();
    }
}

QString DlgBugInfo::basicSystemInformationReplacementText()
{
    QString info;

    // Krita version info
    info.append("Krita");
    info.append("\n  Version: ").append(KritaVersionWrapper::versionString(true));
    info.append("\n\n");

    info.append("Qt");
    info.append("\n  Version (compiled): ").append(QT_VERSION_STR);
    info.append("\n  Version (loaded): ").append(qVersion());
    info.append("\n\n");

    // OS information
    info.append("OS Information");
    info.append("\n  Build ABI: ").append(QSysInfo::buildAbi());
    info.append("\n  Build CPU: ").append(QSysInfo::buildCpuArchitecture());
    info.append("\n  CPU: ").append(QSysInfo::currentCpuArchitecture());
    info.append("\n  Kernel Type: ").append(QSysInfo::kernelType());
    info.append("\n  Kernel Version: ").append(QSysInfo::kernelVersion());
    info.append("\n  Pretty Productname: ").append(QSysInfo::prettyProductName());
    info.append("\n  Product Type: ").append(QSysInfo::productType());
    info.append("\n  Product Version: ").append(QSysInfo::productVersion());
#ifdef Q_OS_ANDROID
    QString manufacturer =
        QAndroidJniObject::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;").toString();
    const QString model =
        QAndroidJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString();
    manufacturer[0] = manufacturer[0].toUpper();
    info.append("\n  Product Model: ").append(manufacturer + " " + model);
#endif
    info.append("\n\n");

    // OpenGL information
    info.append("\n").append(KisOpenGL::getDebugText());
    info.append("\n\n");
    // Hardware information
    info.append("Hardware Information");
    info.append(QString("\n Memory: %1").arg(KisImageConfig(true).totalRAM() / 1024)).append(" Gb");
    info.append(QString("\n Cores: %1").arg(QThread::idealThreadCount()));
    info.append("\n Swap: ").append(KisImageConfig(true).swapDir());

    return info;
}

QString DlgBugInfo::infoText(QSettings& kritarc)
{
    QString info;

    if (!kritarc.value("LogUsage", true).toBool() || !QFileInfo(originalFileName()).exists()) {

        // NOTE: This is intentionally not translated!

        info.append(replacementWarningText());
        info.append("File name and location: " + originalFileName());
        info.append("------------------------------------");
        info.append("\n\n");

        info.append(basicSystemInformationReplacementText());
    }
    else {

        QFile log(originalFileName());
        log.open(QFile::ReadOnly | QFile::Text);
        info += QString::fromUtf8(log.readAll());
        log.close();
    }

    return info;

}

DlgBugInfo::~DlgBugInfo()
{
    delete m_page;
}
