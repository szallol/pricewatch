#pragma clang diagnostic ignored "-Wignored-attributes"
//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_WEBMARKET_HPP
#define PRICEWATCH_WEBMARKET_HPP

#include <QDebug>
#include <QtWebKitWidgets/QWebPage>
#include <QtSql>
#include <QProcess>

#include <random>
#include <mutex>

#include <boost/log/trivial.hpp>

#include "MarketWebPage.hpp"
#include "MarketProduct.hpp"

enum class Collect {OK, Failed};
enum class TaskResult {Completed=1 , Failed};
enum class ClickElementResult {Clicked=1, NotFound, Disabled};

//static std::mutex dbMutex;
//
//static const std::string alphanum{"01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
//
//static std::random_device rd;
//static std::mt19937 gen(rd());
//static std::uniform_int_distribution<> dis(1, alphanum.length());
//
//static const char genRandChar() {
//    return alphanum[ dis(gen)];
//}
//
//static std::string genRandStr(int length) {
//    std::string tmp_string;
//    for (int i=0; i< length; i++) {
//        tmp_string+=genRandChar();
//    }
//    return  tmp_string;
//}

class WebMarket: public QObject {
    Q_OBJECT
public:
    WebMarket ();
    virtual ~WebMarket() {
        db.close();
    };
    virtual Collect fetchCategories()=0;
    virtual Collect fetchProducts()=0;
    virtual Collect fetchProductsFromCategory(ProductCategory &)=0;
    virtual Collect fetchIndividualProductPrice(int)=0;
//    virtual Collect fetchProductPrice(MarketWebPage &, int)=0;
    bool equalProductsAndDescriptions();
    TaskResult addProductsToDb(std::vector<MarketProduct> &);
    TaskResult addCategoriesToDb (std::vector<ProductCategory> &);
	TaskResult generatePricesForTimestamp(std::string);
	TaskResult fetchPrices(); // fetch prices with indivdual method
	virtual TaskResult fetchPricesBulk()=0; // fetch prices with category list method

    int getPriceLimit() const {
        return priceLimit;
    }

    void setPriceLimit(int priceLimit) {
        WebMarket::priceLimit = priceLimit;
    }

	int getMaxProcess() const {
		return maxProcess;
	}

	void setMaxParocess(int maxProc) {
		WebMarket::maxProcess = maxProc;
	}

    virtual int getMarketId() const=0;

    virtual void setMarketId(int marketId)=0;

    Wait waitSeconds(int);
private:
    std::vector<std::string> args;
	int maxProcess;

	int getRunningWorkers();

protected:
    TaskResult fetchCategoriesFromDb();
    std::vector<ProductCategory> categories;
    int priceLimit;
    QSqlDatabase db;
};


#endif //PRICEWATCH_WEBMARKET_HPP
