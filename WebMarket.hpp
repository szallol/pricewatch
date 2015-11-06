//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_WEBMARKET_HPP
#define PRICEWATCH_WEBMARKET_HPP

#include <QDebug>
#include <QtWebKitWidgets/QWebPage>
#include "MarketWebPage.hpp"
#include "MarketProduct.hpp"

enum class Collect {OK, Failed};

class WebMarket: public QObject {
    Q_OBJECT
public:
    virtual ~WebMarket() {};
    virtual Collect fetchCategories()=0;
    virtual Collect fetchProducts()=0;
    virtual Collect fetchProductsFromCategory(ProductCategory &)=0;
    virtual Collect fetchProductPrice(int)=0;

    int getPriceLimit() const {
        return priceLimit;
    }

    void setPriceLimit(int priceLimit) {
        WebMarket::priceLimit = priceLimit;
    }
protected:
    std::vector<ProductCategory> categories;
    int priceLimit;
};


#endif //PRICEWATCH_WEBMARKET_HPP
