//
// Created by szall on 10/22/2015.
//

#include "MarketWebPage.hpp"

#include <QEventLoop>
#include <Qtimer>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QtWebKitWidgets/QWebFrame>

#include <string>
#include <memory>
#include <QtGui/qpainter.h>

Wait MarketWebPage::waitSeconds(int i) {
    QEventLoop evFinishedWait;

    qDebug() << "\twaiting: " << i << "seconds";
    QTimer::singleShot(i * 1000, [&] {
      evFinishedWait.exit();
    });
    evFinishedWait.exec();
//    qDebug() << "\twaitSeconds finish";
    return Wait::OK;
}

Wait MarketWebPage::waitUntilContains(std::string content, int timeout) {
    QEventLoop evFinishedLoading;
    QTimer timerCheckContent;
    QTimer BigTimeout;

    BigTimeout.singleShot(timeout, Qt::VeryCoarseTimer,&evFinishedLoading, [&] {
        qDebug() << "\twaitUntilContains: "<< mainFrame()->url().toString()<< " timed out(" << timeout <<" msec)";
        evFinishedLoading.exit(1); //exit with return code 1
    });

    QObject::connect(&timerCheckContent, &QTimer::timeout, [&] {
        if (getContentText().find(content) != std::string::npos) {
            qDebug() << "\twaitUntilContains found: " << QString::fromStdString(content);
            timerCheckContent.stop();
            BigTimeout.stop();
            evFinishedLoading.exit(); // if found content text then return with good results
        }
        timerCheckContent.start(); // restart page content check
    });
    timerCheckContent.start(500); // check page content every 0.5 second
    if (evFinishedLoading.exec() != 0) {
        return  Wait::TIMEOUT;
    }

    return Wait::OK;

}

void MarketWebPage::load(const std::string url) {
    strLoadedUrl =url;
    mainFrame()->load(QUrl::fromUserInput(QString::fromStdString(url)));
}

std::string MarketWebPage::getContentText() {
    return mainFrame()->toPlainText().toStdString();
}


void MarketWebPage::savePageImage(const std::string fileName) {
   setViewportSize(mainFrame()->contentsSize());
    QImage image(mainFrame()->geometry().size(), QImage::Format_ARGB32_Premultiplied);
    //image.fill(Qt::t);

    QPainter painter(&image);
    mainFrame()->render(&painter);
    painter.end();

    bool ret = image.save (QString::fromStdString(fileName), "JPEG");// writes image into ba in JPEG format
    if (ret)
    {
        qDebug() << "\tsavePageImage:" << QString::fromStdString(fileName) << " saved.ok";
    }
    else
    {
        qDebug() << "\tsavePageImage:" << QString::fromStdString(fileName) << " failed to saved";
    }
}

