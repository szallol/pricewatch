#pragma clang diagnostic ignored "-Wignored-attributes"
//#pragma clang diagnostic ignored "-Winconsistent-missing-override"
//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_EMAGMARKET_HPP
#define PRICEWATCH_EMAGMARKET_HPP

#include "WebMarket.hpp"
#include <EMagMarket.hpp>
#include <EMagWebPage.hpp>

#include <QtSql>
#include <QDebug>
#include <QtSql/qsqldatabase.h>


static const std::vector<std::string> skipWordsCategory = {"supermarket", "imbracaminte", "asigurari", "rovinete"};
static const std::vector<std::string> skipWordsProduct = {};


class EMagMarket : public WebMarket {
    Q_OBJECT
public:
    EMagMarket();
    ~EMagMarket() {};
    virtual Collect fetchCategories() override;
    virtual Collect fetchProducts() override;
    virtual Collect fetchProductsFromCategory(ProductCategory &) override;
    virtual Collect fetchProductPrice(int) override;

    virtual int getMarketId() const override {return marketId;} ;
    virtual void setMarketId(int intMarketId) override {marketId=intMarketId;};

private:

    ClickElementResult clickProductListNextPage(EMagWebPage &);

    TaskResult fetchCategoriesFromDb();
    TaskResult addCategoriesToDb (std::vector<ProductCategory> &);

    int marketId;
};


#endif //PRICEWATCH_EMAGMARKET_HPP
