//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_WEBMARKET_HPP
#define PRICEWATCH_WEBMARKET_HPP


#include <QtWebKitWidgets/QWebPage>
#include "MarketWebPage.hpp"
#include "MarketProduct.hpp"

enum class Collect {OK, Failed};

class WebMarket: public QObject {
    Q_OBJECT
public:

    virtual Collect fetchCategories()=0;
    virtual Collect fetchProducts()=0;
    virtual Collect fetchProductsFromCategory(ProductCategory &)=0;


protected:
    std::vector<ProductCategory> categories;
};


#endif //PRICEWATCH_WEBMARKET_HPP
