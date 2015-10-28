//
//
// Created by szall on 10/23/2015.
//

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/qwebelement.h>
#include <QtSql>

#include "EMagWebPage.hpp"
#include "EMagMarket.hpp"

#include <string>
#include <thread>

EMagMarket::EMagMarket() {
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("pricewatch");
    db.setUserName("root");
    bool ok = db.open();
    if (!ok) {
        qDebug() << "connecting to database failed: " << db.lastError().text();
        return;
    }

    if (fetchCategories() != Collect::OK)
    {
        return;
    }

    fetchProducts();
}

Collect EMagMarket::fetchProducts() {
//    EMagWebPage pageEMag;
    int catCounter=1;
    for (auto &category : categories) {
        qDebug() << "fetching products from category(" << QString::number(catCounter) << " of " << QString::number(categories.size()) << ") : " << QString::fromStdString(category.getName()) << ", " << QString::fromStdString(category.getCategoryUrl());

        fetchProductsFromCategory(category);
        qDebug() << "----------------------------------------------------";
        catCounter++;
    }

    return Collect::OK;
}

Collect EMagMarket::fetchProductsFromCategory(ProductCategory &category) {
    if (category.getId() == 0) {
        qDebug() << "category: " << QString::fromStdString(category.getName()) << " id not found. trying to fetch from Db.";
        QSqlQuery q;

        q.prepare("SELECT id, title, url FROM categories WHERE shop_id=1 AND title=\"" +QString::fromStdString(category.getName())+"\"");

        if (!q.exec()){
            qDebug () << "\tfailed to fetch category. " << q.lastError().text();
            return Collect::Failed;
        }

        bool found = false;
        while (q.next()) {
            found=true;
            int  catId = q.value(0).toInt();
            category.setId(catId);
        }
        if (!found) {
            qDebug () << "something wrong with category: " << QString::fromStdString(category.getName());
            return  Collect::Failed;
        }

    }
    EMagWebPage pageEMag;

    pageEMag.load(category.getCategoryUrl());

    QWebElementCollection listProducts;
    ClickElementResult nextPageResult;
    std::vector<MarketProduct> products;
    do {
        int prodCounter=0;

        if (pageEMag.waitUntilContains("Dante International", 20000) == Wait::OK) {
//        qDebug() << QString::fromStdString(pageEMag.getContentText());
        }

        pageEMag.waitSeconds(2);

        listProducts = pageEMag.mainFrame()->findAllElements("div.poza-produs > a");

        foreach (auto elementProduct, listProducts) {
                std::string strProductTitle = elementProduct.attribute("title").remove('"').toStdString();
                std::string strProductUrl;
                strProductUrl = "http://www.emag.ro" + elementProduct.attribute("href").toStdString();

                bool containsSkipWord = false;
                for (auto strWord: skipWordsProduct) {
                    if (strProductUrl.find(strWord) != std::string::npos) containsSkipWord=true;
                }

                if (!containsSkipWord) {
                    products.push_back(MarketProduct("", category.getId(), strProductTitle, strProductUrl, 199.0));
                    //                Categories.push_back(new ProductCategory(strCategoryName, strCategoryUrl));
                    //                qDebug() << QString::fromStdString(strProductTitle) << " : "<< QString::fromStdString(strProductUrl);
                    prodCounter++;
                }
        }

        qDebug() << "\tfound : "<< QString::number(prodCounter)<< " products on page: "<< pageEMag.mainFrame()->url();
//        pageEMag.savePageImage("c:/tmp/ll/pricewatch_lastscreen.jpg");
        nextPageResult = clickProductListNextPage(pageEMag);
    } while (nextPageResult == ClickElementResult::Clicked);

    addProductsToDb(products);
    return Collect::OK;
}

ClickElementResult EMagMarket::clickProductListNextPage(EMagWebPage &page) {
    QWebElement nextPage = page.mainFrame()->findFirstElement("span.icon-i44-go-right");

    if (!nextPage.isNull()) {
        if (nextPage.classes().contains("emg-pagination-disabled")) {
            qDebug() << "\tnext page button found but disabled";
            return  ClickElementResult::Disabled;
            }
        else {
            qDebug() << "\tnext page button found";
            page.mainFrame()->evaluateJavaScript("$('span.icon-i44-go-right').click();null;");
            return ClickElementResult::Clicked;
        }
    }
    return ClickElementResult::NotFound;
}

Collect EMagMarket::fetchCategories() {
    EMagWebPage pageEMag;

    pageEMag.load("http://www.emag.ro");

    if (pageEMag.waitUntilContains("Dante International", 20000) == Wait::OK) {
//        qDebug() << QString::fromStdString(pageEMag.getContentText());
    }
    pageEMag.waitSeconds(5);

    QWebElementCollection menuProductsCategory;
    menuProductsCategory = pageEMag.mainFrame()->findAllElements("a.emg-megamenu-link");


    foreach (auto elementMenu, menuProductsCategory) {
        std::string strCategoryName = elementMenu.toInnerXml().trimmed().remove('"').toStdString();
        std::string strCategoryUrl = "http://www.emag.ro"+elementMenu.attribute("href").toStdString()+"&pc=60";

        bool containsSkipWord = false;
        for (auto strWord: skipWordsCategory) {
           if (strCategoryUrl.find(strWord) != std::string::npos) containsSkipWord=true;
        }

        if (!containsSkipWord && !strCategoryName.empty()) {
            categories.push_back(ProductCategory(1, strCategoryName, strCategoryUrl)); // create category for shop:1 emag.ro
    //                qDebug() << QString::fromStdString(strCategoryName) << " : "<< QString::fromStdString(CategoryUrl);
        }
    }

    if (addCategoriesToDb(categories)!=TaskResult::Completed) {
        return  Collect::Failed;
    }

    return Collect::OK;
}

TaskResult EMagMarket::addProductsToDb(std::vector<MarketProduct> &products) {
    QSqlQuery q;

    for (auto product: products) {
//        qDebug() << QString::fromStdString(product.getTitle()) << QString::fromStdString(product.getProductUrl());

        if (!q.prepare(QLatin1String("insert into products (shop_id, cat_id,  title, url) values(?, ?, ?, ?)")))
            return TaskResult::Failed;

        q.addBindValue(1);
        q.addBindValue(product.getCatId());
        q.addBindValue(QString::fromStdString(product.getTitle()));
        q.addBindValue(QString::fromStdString(product.getProductUrl()));
        q.exec();
        if (!q.exec()) {
            qDebug() << "addProductsToDb error: " << q.lastError().text() << product.getCatId();
        }
    }

    return TaskResult::Failed;
}

TaskResult EMagMarket::addCategoriesToDb(std::vector<ProductCategory> &categories) {
    QSqlQuery q;

    for (auto category: categories) {
        //check if category already in db
        bool found = false;
        q.prepare("SELECT shop_id, title, url FROM categories WHERE shop_id=1 AND title=\"" +QString::fromStdString(category.getName())+"\"");
        q.bindValue(":title", QString::fromStdString(category.getName()));

        if (!q.exec()){
            qDebug () << "\tfailed to fetch category. " << q.lastError().text();
            return TaskResult::Failed;
        }
//        qDebug() << q.lastQuery() << q.executedQuery();
//        return  TaskResult::Failed;
        while (q.next()) {
            found=true;
            int shop_id = q.value(0).toInt();
            std::string title = q.value(1).toString().toStdString();
            std::string url = q.value(2).toString().toStdString();

            if (category.getName() == title && category.getCategoryUrl() != url) // category url changed
            {
                qDebug() << "category url changed: " << QString::fromStdString(category.getName()) << ", from " << QString::fromStdString(url) << " -> " << QString::fromStdString(category.getCategoryUrl());
            }
        }

        if (!found){
            qDebug() << "adding new category: " << QString::fromStdString(category.getName()) << QString::fromStdString(category.getCategoryUrl());
            if (!q.prepare(QLatin1String("insert into categories (shop_id, title, url) values(?, ?, ?)")))
                return TaskResult::Failed;

            q.addBindValue(1);
            q.addBindValue(QString::fromStdString(category.getName()));
            q.addBindValue(QString::fromStdString(category.getCategoryUrl()));
            if (!q.exec()) {
                qDebug() << "addCategoriesToDb error: " << q.lastError().text();
            }
        }
    }

    return TaskResult::Completed;
}
