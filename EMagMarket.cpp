#pragma clang diagnostic ignored "-Wignored-attributes"

//
// Created by szall on 10/23/2015.
//

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/qwebelement.h>
#include <QtSql>

#include "EMagWebPage.hpp"
#include "EMagMarket.hpp"

#include <thread>
#include <iostream>


EMagMarket::EMagMarket() : WebMarket()  {
    QSqlQuery q;
    q.prepare("SELECT id FROM shops WHERE name = 'emag.ro'");

    if (!q.exec()){
        BOOST_LOG_TRIVIAL(error) << "\tfailed to fetch shop id " << q.lastError().text().toStdString();
    }

    if(!q.first()) {
        BOOST_LOG_TRIVIAL(error) << "\tfailed to fetch shop id " << q.lastError().text().toStdString();
    } else {
        marketId = q.value(0).toInt(); // add 1 to incrase to next available id
    }
    BOOST_LOG_TRIVIAL(info) << "found market id: " << marketId;
}

Collect EMagMarket::fetchProducts() {
    if (categories.size()==0) { //if there is no category fetched/loaded
        fetchCategoriesFromDb();
    }
//    EMagWebPage pageEMag;
    int catCounter=1;
	std::vector<ProductCategory> subcategories;
	std::copy(std::find_if(categories.begin(), categories.end(), [](const ProductCategory elem) {return elem.getId()==408;}) , categories.end(), std::back_inserter(subcategories));
//    for (auto elem: categories) {
//        qDebug () << QString::fromStdString(elem.getName());
//    }
//    return Collect::OK;
    for (auto &category : subcategories) {
        BOOST_LOG_TRIVIAL(info) << "fetching products from category(" << catCounter << " of " << subcategories.size() << ") : " << category.getName() << ", " << category.getCategoryUrl();

        if (fetchProductsFromCategory(category)!=Collect::OK) {
            return Collect::Failed;
        }
        BOOST_LOG_TRIVIAL(info) << "----------------------------------------------------";
        catCounter++;
    }

    return Collect::OK;
}

Collect EMagMarket::fetchProductsFromCategory(ProductCategory &category) {
    if (category.getId() == 0) {
        BOOST_LOG_TRIVIAL(info) << "category: " << category.getName() << " id not found. trying to fetch from Db.";
        QSqlQuery q;

        q.prepare("SELECT id, title, url FROM categories WHERE shop_id=1 AND title=\"" +QString::fromStdString(category.getName())+"\"");

        if (!q.exec()){
            BOOST_LOG_TRIVIAL (error) << "\tfailed to fetch category. " << q.lastError().text().toStdString();
            return Collect::Failed;
        }
 
        bool found = false;
        while (q.next()) {
            found=true;
            int  catId = q.value(0).toInt();
            category.setId(catId);
        }
        if (!found) {
            BOOST_LOG_TRIVIAL (error) << "something wrong with category: " << category.getName();
            return  Collect::Failed;
        }
    }
    EMagWebPage pageEMag;

	pageEMag.load("http://m.emag.ro/fullversion");
    pageEMag.load(category.getCategoryUrl());
//    pageEMag.show();

    QWebElementCollection listProducts;
    ClickElementResult nextPageResult;
    std::vector<MarketProduct> products;
    do {
        int prodCounter=0;

        if (pageEMag.waitUntilContains("Dante International", 30000) == Wait::OK) {
//        qDebug() << QString::fromStdString(pageEMag.getContentText());
        }

        waitSeconds(3);

        listProducts = pageEMag.mainFrame()->findAllElements("div.product-holder-grid"); //("div.poza-produs > a");

        foreach (QWebElement elementProduct, listProducts) {
                std::string strProductTitle = elementProduct.findFirst("div.poza-produs > a").attribute("title").remove('"').toStdString();
                std::string strProductUrl = "http://www.emag.ro" + elementProduct.findFirst("div.poza-produs > a").attribute("href").toStdString();
                int intProductPrice = elementProduct.findFirst("span.price-over > span.money-int").toInnerXml().trimmed().remove('.').toInt();

                if (getPriceLimit() > intProductPrice) continue;

                QWebElement sellerMarket = elementProduct.findFirst("form > div#pret2 > div.top > span.feedback-right-msg");
                if (!sellerMarket.isNull()) {
                    if (sellerMarket.toPlainText().toStdString().find("Vandut de eMAG") == std::string::npos) {
                        continue; // not matching with current market
                    }
                }
                bool containsSkipWord = false;
                for (auto strWord: skipWordsProduct) {
                    if (strProductUrl.find(strWord) != std::string::npos) containsSkipWord=true;
                }

                if (!containsSkipWord) {
                    products.push_back(MarketProduct(category.getId(), strProductTitle, strProductUrl));
//                                    qDebug() << QString::fromStdString(strProductTitle) << " : "<< QString::fromStdString(strProductUrl) << intProductPrice;
                    prodCounter++;
                }
        }

        BOOST_LOG_TRIVIAL(info) << "\tfound : "<< prodCounter << " products on page: "<< pageEMag.mainFrame()->url().toString().toStdString();
//        pageEMag.savePageImage("c:/tmp/ll/pricewatch_lastscreen.jpg");
        nextPageResult = clickProductListNextPage(pageEMag);
    } while (nextPageResult == ClickElementResult::Clicked);

    if (addProductsToDb(products)!=TaskResult::Completed) {
        return Collect::Failed;
    }
    return Collect::OK;
}

ClickElementResult EMagMarket::clickProductListNextPage(EMagWebPage &page) {
    QWebElement nextPage = page.mainFrame()->findFirstElement("span.icon-i44-go-right");

    if (!nextPage.isNull()) {
        if (nextPage.classes().contains("emg-pagination-disabled")) {
            BOOST_LOG_TRIVIAL(info) << "\tnext page button found but disabled";
            return  ClickElementResult::Disabled;
            }
        else {
            BOOST_LOG_TRIVIAL(info) << "\tnext page button found";
            page.mainFrame()->evaluateJavaScript("$('span.icon-i44-go-right').click();null;");
            return ClickElementResult::Clicked;
        }
    }
    return ClickElementResult::NotFound;
}

Collect EMagMarket::fetchCategories() {
    EMagWebPage pageEMag;

    pageEMag.load("http://www.emag.ro");

    if (pageEMag.waitUntilContains("Dante International", 5000) == Wait::OK) {
//        qDebug() << QString::fromStdString(pageEMag.getContentText());
    }
    waitSeconds(2);

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

TaskResult EMagMarket::addCategoriesToDb(std::vector<ProductCategory> &categories) {
    QSqlQuery q;

    for (auto category: categories) {
        //check if category already in db
        bool found = false;
        q.prepare("SELECT shop_id, title, url FROM categories WHERE shop_id=1 AND title=\"" +QString::fromStdString(category.getName())+"\"");
        q.bindValue(":title", QString::fromStdString(category.getName()));

        if (!q.exec()){
            BOOST_LOG_TRIVIAL (error) << "\tfailed to fetch category. " << q.lastError().text().toStdString();
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
                BOOST_LOG_TRIVIAL(info) << "category url changed: " << category.getName() << ", from " << url << " -> " << category.getCategoryUrl();
            }
        }

        if (!found){
            BOOST_LOG_TRIVIAL(info) << "adding new category: " << category.getName() << category.getCategoryUrl();
            if (!q.prepare(QLatin1String("insert into categories (shop_id, title, url) values(?, ?, ?)")))
                return TaskResult::Failed;

            q.addBindValue(1);
            q.addBindValue(QString::fromStdString(category.getName()));
            q.addBindValue(QString::fromStdString(category.getCategoryUrl()));
            if (!q.exec()) {
                BOOST_LOG_TRIVIAL(error) << "addCategoriesToDb error: " << q.lastError().text().toStdString();
            }
        }
    }

    return TaskResult::Completed;
}

TaskResult EMagMarket::fetchCategoriesFromDb() {
    QSqlQuery q;

    q.prepare("SELECT id, shop_id, title, url FROM categories");

    if (!q.exec()){
        BOOST_LOG_TRIVIAL (error) << "\tfailed to fetch category. " << q.lastError().text().toStdString();
        return TaskResult::Failed;
    }
//        qDebug() << q.lastQuery() << q.executedQuery();
//        return  TaskResult::Failed;
    bool found = false;
    categories.clear();
    while (q.next()) {
        found = true;
        int cat_id = q.value(0).toInt();
        int shop_id = q.value(1).toInt();
        std::string catTitle = q.value(2).toString().toStdString();
        std::string catUrl = q.value(3).toString().toStdString();
        categories.push_back(ProductCategory(cat_id, shop_id, catTitle, catUrl));
    }

    if (!found) {
        BOOST_LOG_TRIVIAL (error) << "no category found in database. please fets/update categories.";
        return TaskResult::Failed;
    }
    return TaskResult::Failed;
}


Collect EMagMarket::fetchProductPrice(int priceId) {
//    std::lock_guard<std::mutex> lock(dbMutex);

    QSqlQuery q;
    int productId=0;

    q.prepare("SELECT product_id FROM prices WHERE id="+QString::number(priceId));

    if (!q.exec()){
        BOOST_LOG_TRIVIAL (error) << "\tfailed to fetch price" << q.lastError().text().toStdString();
        return Collect::Failed;
    }

    if(q.first()) {
        productId = q.value(0).toInt();
    } else {
        BOOST_LOG_TRIVIAL(error) << "\tno price found witfh id: " << priceId;
        return  Collect::Failed;
    }

    std::string productUrl;
    q.prepare("SELECT url FROM productdetails WHERE id="+QString::number(productId));

    if (!q.exec()){
        BOOST_LOG_TRIVIAL (error) << "\tfailed to fetch product url" << q.lastError().text().toStdString();
        return Collect::Failed;
    }

    if(q.first()) {
        productUrl = q.value(0).toString().toStdString();
    }

    if (productUrl.empty()) {
        BOOST_LOG_TRIVIAL(error) << "found product with empty url, productId: " << productId;
    }

    EMagWebPage pageEMag;
    pageEMag.show();
    BOOST_LOG_TRIVIAL(info) << "loading product page: " << productUrl;

    pageEMag.load(productUrl);
    waitSeconds(5);

    pageEMag.saveElementImage("form#addToCartForm", "/tmp/ll/product.jpg");
//    pageEMag.saveElementImage("img.poza_produs", "/tmp/ll/product.jpg");

    QWebElement spanPrice = pageEMag.mainFrame()->findFirstElement("div.prices > span");
    if (spanPrice.isNull()) {
        BOOST_LOG_TRIVIAL(error) << "can't find product price tag: " << productUrl ;
        return Collect::Failed;
    }

    double productPrice = spanPrice.attribute("content").toDouble();
    if (productPrice > 0 ) {
        BOOST_LOG_TRIVIAL(info) << "product price: " << productPrice;

        q.prepare(QString::fromStdString("UPDATE prices SET price=" + std::to_string(productPrice) + " WHERE id= " + std::to_string(priceId)));
        if (!q.exec()){
            BOOST_LOG_TRIVIAL (error) << "\tfailed to update product price" << q.lastError().text().toStdString();
            return Collect::Failed;
        }
    }

    return Collect::OK;
}
