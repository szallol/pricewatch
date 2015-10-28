//
// Created by szall on 10/22/2015.
//

#ifndef PROBA_CLION_MARKETWEBPAGE_HPP
#define PROBA_CLION_MARKETWEBPAGE_HPP

#include <QtWebKitWidgets/QWebPage>

#include <ProductCategory.hpp>

#include <string>
#include <vector>

enum class Wait {OK, TIMEOUT};


class MarketWebPage : public QWebPage{
Q_OBJECT
public:
    MarketWebPage(){};
    void load(const std::string);
    void savePageImage(const std::string);

    Wait waitUntilContains(std::string, int);
    Wait waitSeconds(int);
    std::string getContentText();

protected:
    std::string strLoadedUrl;

};


#endif //PROBA_CLION_MARKETWEBPAGE_HPP
