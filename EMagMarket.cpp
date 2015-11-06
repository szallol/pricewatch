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
}

Collect EMagMarket::fetchProducts() {
    if (categories.size()==0) { //if there is no category fetched/loaded
        fetchCategoriesFromDb();
    }
//    EMagWebPage pageEMag;
    int catCounter=1;
    std::vector<ProductCategory> subcategories;
    std::copy(std::find_if(categories.begin(), categories.end(), [](const ProductCategory elem) {return elem.getName()=="Imbracaminte copii";}) , categories.end(), std::back_inserter(subcategories));
//    for (auto elem: categories) {
//        qDebug () << QString::fromStdString(elem.getName());
//    }
//    return Collect::OK;
    for (auto &category : subcategories) {
        qDebug() << "fetching products from category(" << QString::number(catCounter) << " of " << QString::number(subcategories.size()) << ") : " << QString::fromStdString(category.getName()) << ", " << QString::fromStdString(category.getCategoryUrl());

        if (fetchProductsFromCategory(category)!=Collect::OK) {
            return Collect::Failed;
        }
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

        if (pageEMag.waitUntilContains("Dante International", 30000) == Wait::OK) {
//        qDebug() << QString::fromStdString(pageEMag.getContentText());
        }

        pageEMag.waitSeconds(3);

        listProducts = pageEMag.mainFrame()->findAllElements("div.product-holder-grid"); //("div.poza-produs > a");

        foreach (auto elementProduct, listProducts) {
                std::string strProductTitle = elementProduct.findFirst("div.poza-produs > a").attribute("title").remove('"').toStdString();
                std::string strProductUrl = "http://www.emag.ro" + elementProduct.findFirst("div.poza-produs > a").attribute("href").toStdString();
                int intProductPrice = elementProduct.findFirst("span.price-over > span.money-int").toInnerXml().trimmed().remove('.').toInt();

                if (getPriceLimit() > intProductPrice) continue;

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

        qDebug() << "\tfound : "<< QString::number(prodCounter)<< " products on page: "<< pageEMag.mainFrame()->url();
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

    if (pageEMag.waitUntilContains("Dante International", 5000) == Wait::OK) {
//        qDebug() << QString::fromStdString(pageEMag.getContentText());
    }
    pageEMag.waitSeconds(2);

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
    std::vector<MarketProduct> dbProducts;

    // fetch all existing products from db for later check
    q.prepare("SELECT productdetails.id, products.cat_id, title, url, shop_id FROM productdetails LEFT JOIN products ON products.id=productdetails.id WHERE shop_id = 1 ");

    if (!q.exec()){
        qDebug () << "\tfailed to fetch products for check. " << q.lastError().text();
        return TaskResult::Failed;
    }

    while (q.next()) {
        int intProductId = q.value(0).toInt();
        int intProductCatId = q.value(1).toInt();
        std::string strProductTitle = q.value(2).toString().toStdString();
        std::string strProductUrl = q.value(3).toString().toStdString();

        dbProducts.push_back(MarketProduct(intProductId, intProductCatId, strProductTitle, strProductUrl));
    }

    // get last product index
    q.prepare("SELECT id FROM products ORDER BY id DESC LIMIT 1");

    if (!q.exec()){
        qDebug () << "\tfailed to fetch product. " << q.lastError().text();
        return TaskResult::Failed;
    }

    int lastIndex;
    if(!q.first()) {
        lastIndex=1;
    } else {
        lastIndex = q.value(0).toInt() + 1; // add 1 to incrase to next available id
    }

    for (auto product: products) {
//        qDebug() << QString::fromStdString(product.getTitle()) << QString::fromStdString(product.getProductUrl());
        if (!equalProductsAndDesctiptions()) {
            qDebug() << "something wrong with products count (productCount != productDetailsCount)!";
            qDebug () << QString::fromStdString(product.getTitle()) << " : " << QString::fromStdString(product.getProductUrl()) << " : " <<product.getCatId();
            return TaskResult::Failed;
        }
        auto resultFind = std::find_if(dbProducts.begin(), dbProducts.end(), [&](const MarketProduct prod) {return prod.getProductUrl()==product.getProductUrl();});
        if (resultFind != dbProducts.end()) { // product exist in db
        } else {
            if (product.getTitle().empty()) continue; // ship empty product names
//            product.getTitle().replace("\"", "\\\""); //escape double quotes

            // insert to details table
            if (!q.prepare(QLatin1String("INSERT INTO productdetails (id, title, url) VALUES(?, ?, ?)"))) {
                qDebug() << "failed to prepare productdetails ";
                return TaskResult::Failed;
            }

            q.addBindValue(lastIndex);
            q.addBindValue(QString::fromStdString(product.getTitle()));
            q.addBindValue(QString::fromStdString(product.getProductUrl()));
            if (!q.exec()) {
                qDebug() << "addProductsToDb(productdetails) error: " << q.lastError().text() << product.getCatId();
                continue; //jump to next product if something failes
            }

            //insert into products table
            if (!q.prepare(QLatin1String("INSERT INTO products (id, shop_id, cat_id) VALUES(?, ?, ?)"))){
                qDebug() << "failed to prepare productdetails ";
                return TaskResult::Failed;
            }

            q.addBindValue(lastIndex);
            q.addBindValue(1);
            q.addBindValue(product.getCatId());
            if (!q.exec()) {
                qDebug() << "addProductsToDb(products) error: " << q.lastError().text() << product.getCatId();
            }

            dbProducts.push_back(MarketProduct(product.getCatId(), product.getTitle(), product.getProductUrl()));
            lastIndex++;
        }
    }

    return TaskResult::Completed;
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

TaskResult EMagMarket::fetchCategoriesFromDb() {
    QSqlQuery q;

    q.prepare("SELECT id, shop_id, title, url FROM categories");

    if (!q.exec()){
        qDebug () << "\tfailed to fetch category. " << q.lastError().text();
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
        qDebug () << "no category found in database. please fets/update categories.";
        TaskResult::Failed;
    }
    return TaskResult::Failed;
}

bool EMagMarket::equalProductsAndDesctiptions() {
    QSqlQuery q;
    int countProducts=0;

    q.prepare("SELECT COUNT(*) FROM products");

    if (!q.exec()){
        qDebug () << "\tfailed to count products. " << q.lastError().text();
        return false;
    }

    if(q.first()) {
        countProducts = q.value(0).toInt();
    }

    int countProductDetails=0;

    q.prepare("SELECT COUNT(*) FROM productdetails");

    if (!q.exec()){
        qDebug () << "\tfailed to count productdetails. " << q.lastError().text();
        return false;
    }

    if(q.first()) {
        countProductDetails= q.value(0).toInt();
    }

    return countProducts == countProductDetails;
}

Collect EMagMarket::fetchProductPrice(int priceId) {
    QSqlQuery q;
    int productId=0;

    q.prepare("SELECT product_id FROM prices WHERE id="+QString::number(priceId));

    if (!q.exec()){
        qDebug () << "\tfailed to fetch price" << q.lastError().text();
        return Collect::Failed;
    }

    if(q.first()) {
        productId = q.value(0).toInt();
    }

    qDebug() << "fetching product id: " << productId;
    return Collect::Failed;
}
