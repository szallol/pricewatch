#pragma clang diagnostic ignored "-Wignored-attributes"
//
// Created by szall on 10/22/2015.
//

#ifndef PROBA_CLION_MARKETWEBPAGE_HPP
#define PROBA_CLION_MARKETWEBPAGE_HPP

#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>

#include <ProductCategory.hpp>

#include <string>
#include <vector>
#include <QtWebKit/qwebelement.h>
#include <QtCore/qeventloop.h>
#include <QtWebKitWidgets/qwebframe.h>

enum class Wait {OK, TIMEOUT};


class MarketWebPage : public QWebPage {
Q_OBJECT
public:
    MarketWebPage();
    void load(const std::string);
    void savePageImage(const std::string);
    void saveElementImage(const std::string, const std::string);

    Wait waitUntilContains(std::string, int);
    Wait waitUnitlElementLoaded(std::string, int);

    QWebElement findElement(std::string);

    std::string getContentText();

//    QWebFrame *mainFrame() { return mainFrame();};

protected:
	QString userAgentForUrl(const QUrl& ) const;	
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
//    bool javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result);
    void javaScriptAlert(QWebFrame* frame, const QString& msg);
    bool javaScriptConfirm(QWebFrame* frame, const QString& msg);
    std::string strLoadedUrl;
private:

    QEventLoop evnFinishedLoading;
private slots:
    void finishedLoading(bool);

};


#endif //PROBA_CLION_MARKETWEBPAGE_HPP
