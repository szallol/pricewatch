//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_EMAGMARKET_HPP
#define PRICEWATCH_EMAGMARKET_HPP

#include "WebMarket.hpp"
#include <EMagWebPage.hpp>

#include <QtSql>
#include <QDebug>
#include <QtSql/qsqldatabase.h>


static const std::vector<std::string> skipWordsCategory = {"supermarket", "imbracaminte"};
static const std::vector<std::string> skipWordsProduct = {};

enum class TaskResult {Completed=1 , Failed};
enum class ClickElementResult {Clicked=1, NotFound, Disabled};

class EMagMarket : public WebMarket {
    Q_OBJECT
public:
    EMagMarket();
    ~EMagMarket() {db.close();};
    virtual Collect fetchCategories() override;
    virtual Collect fetchProducts() override;
    virtual Collect fetchProductsFromCategory(ProductCategory &);
    virtual Collect fetchProductPrice(int);

private:
    QSqlDatabase db;

    ClickElementResult clickProductListNextPage(EMagWebPage &);

    TaskResult fetchCategoriesFromDb();
    TaskResult addProductsToDb (std::vector<MarketProduct> &);
    TaskResult addCategoriesToDb (std::vector<ProductCategory> &);

    bool equalProductsAndDesctiptions();
};


#endif //PRICEWATCH_EMAGMARKET_HPP
