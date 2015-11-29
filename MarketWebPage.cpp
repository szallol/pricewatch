//
// Created by szall on 10/22/2015.
//

#include "MarketWebPage.hpp"

#include <QEventLoop>
#include <Qtimer>
#include <QImage>
#include <QPainter>
#include <QUrl>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>

#include <boost/log/trivial.hpp>

#include <string>
#include <chrono>
#include <memory>
#include <QtGui/qpainter.h>


void MarketWebPage::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
//	BOOST_LOG_TRIVIAL(info) << "javascript console: " << message.toStdString();
}

void MarketWebPage::javaScriptAlert(QWebFrame *originatingFrame, const QString &msg) {
	BOOST_LOG_TRIVIAL(info) << "[alert] " << msg.toStdString();
}

bool MarketWebPage::javaScriptConfirm(QWebFrame *frame, const QString &msg) {
	return true;
}

QString MarketWebPage::userAgentForUrl(const QUrl &url) const {
	return "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36";
}

Wait MarketWebPage::waitUntilContains(std::string content, int timeout) {
    QEventLoop evFinishedLoading;
    QTimer timerCheckContent;
    QTimer bigTimeout;

    bigTimeout.singleShot(timeout, Qt::VeryCoarseTimer,&evFinishedLoading, [&] {
        BOOST_LOG_TRIVIAL(info) << "\twaitUntilContains: "<< mainFrame()->url().toString().toStdString()<< " timed out(" << timeout <<" msec)";
		
        evFinishedLoading.exit(1); //exit with return code 1
    });

    QObject::connect(&timerCheckContent, &QTimer::timeout, [&] {
        if (getContentText().find(content) != std::string::npos) {
            BOOST_LOG_TRIVIAL(info) << "\twaitUntilContains found: " << content;
            timerCheckContent.stop();
            bigTimeout.stop();
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

Wait MarketWebPage::waitUnitlElementLoaded(std::string elementPath, int timeout) {
    QEventLoop evFinishedLoading;
    QTimer timerCheckContent;
    QTimer BigTimeout;

    BigTimeout.singleShot(timeout, Qt::VeryCoarseTimer,&evFinishedLoading, [&] {
        BOOST_LOG_TRIVIAL(info) << "\twaitUntilElementLoaded: "<< mainFrame()->url().toString().toStdString()<< " timed out(" << timeout <<" msec)";
        evFinishedLoading.exit(1); //exit with return code 1
    });

    QObject::connect(&timerCheckContent, &QTimer::timeout, [&] {
        if (!findElement(elementPath).isNull()) {
            BOOST_LOG_TRIVIAL(info) << "\twaitUntilElementLoaded: " << elementPath;
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
    strLoadedUrl=url;
    mainFrame()->load(QUrl::fromUserInput(QString::fromStdString(url)));
    evnFinishedLoading.exec();
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
        BOOST_LOG_TRIVIAL(info) << "\tsavePageImage:" << fileName << " saved.ok";
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "\tsavePageImage:" << fileName << " failed to saved";
    }
}

void MarketWebPage::saveElementImage(const std::string strElement, const std::string strFileName) {
    QWebElement webelement = mainFrame()->findFirstElement(QString::fromStdString(strElement));
    if (webelement.isNull()) {
        BOOST_LOG_TRIVIAL(error) << "web element not found: " << strElement;
        return;
    }

    QImage image(webelement.geometry().size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::GlobalColor::white);

    QPainter painter(&image);
    webelement.render(&painter);
    painter.end();

    bool ret = image.save (QString::fromStdString(strFileName), "JPEG");// writes image into ba in JPEG format
    if (ret)
    {
        BOOST_LOG_TRIVIAL(info) << "\tsaveElementImage:" << strFileName << " saved.ok";
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "\tsaveElementImage:" << strFileName << " failed to saved";
    }

}

QWebElement MarketWebPage::findElement(std::string elementPath) {
    return mainFrame()->findFirstElement(QString::fromStdString(elementPath));
}

MarketWebPage::MarketWebPage() {
    settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    settings()->setAttribute(QWebSettings::XSSAuditingEnabled, true);
    settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(finishedLoading(bool)));
}

void MarketWebPage::finishedLoading(bool b) {
    BOOST_LOG_TRIVIAL(info) << "finished loading...";
    evnFinishedLoading.quit();
}
