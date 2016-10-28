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
#include <regex>


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

        pageEMag.waitUntilContains("Dante International", 30000);
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

Collect EMagMarket::fetchIndividualProductPrice(int priceId) {
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
    BOOST_LOG_TRIVIAL(info) << "loading product page: " << productUrl;

    pageEMag.load(productUrl);
    waitSeconds(5);
//	pageEMag.savePageImage("/tmp/ll/lasloaded_page.jpg");

    pageEMag.saveElementImage("div#product-info", "/tmp/ll/product.jpg");

	double productPrice=-1;
	bool noMoreOffer=false;

    QWebElement divNoMoreOffer = pageEMag.mainFrame()->findFirstElement("div#offer-price-stock-add");
	if (!divNoMoreOffer.isNull() && divNoMoreOffer.toPlainText().contains("nu mai face parte din oferta")) {
		BOOST_LOG_TRIVIAL(error) << "product no more available";
		productPrice=-1;
		noMoreOffer=true;
	}

	if (!noMoreOffer) {
		QWebElement spanPrice = pageEMag.mainFrame()->findFirstElement("div.prices > span");
		if (spanPrice.isNull()) {
			BOOST_LOG_TRIVIAL(info) << "can't find product price tag: " << productUrl ;
			return Collect::Failed;
		} else {
			productPrice = spanPrice.attribute("content").toDouble();
		}

		if (productPrice > 0 ) {
			BOOST_LOG_TRIVIAL(info) << "product price: " << productPrice;
		}
	}

	q.prepare(QString::fromStdString("UPDATE prices SET price=" + std::to_string(productPrice) + " WHERE id= " + std::to_string(priceId)));
	if (!q.exec()){
		BOOST_LOG_TRIVIAL (error) << "\tfailed to update product price" << q.lastError().text().toStdString();
		return Collect::Failed;
	}

    return Collect::OK;
}

TaskResult EMagMarket::fetchPricesBulk() {
    EMagWebPage pageEMag;

	pageEMag.load("http://m.emag.ro/vendors/vendor/emag/all-products/sort-popularitydesc");

//	BOOST_LOG_TRIVIAL(info) << pageEMag.mainFrame()->toPlainText().toStdString();
    QWebElementCollection listProducts;
	QWebElement nextPageButton;
    do {
        int prodCounter=0;

//        pageEMag.waitUntilContains("Varianta desktop", 30000);
//        waitSeconds(3);

        listProducts = pageEMag.mainFrame()->findAllElements("a.product-container"); 
//		BOOST_LOG_TRIVIAL(info) << "found a.product-continer lement: " << listProducts.count();
        foreach (QWebElement elementProduct, listProducts) {
				std::string strProductTitle = elementProduct.findFirst("span.product > span.product-info-lite > span.product-title").toInnerXml().remove("\n").remove('"').trimmed().toStdString();
				std::string strProductUrl = "http://www.emag.ro" + elementProduct.attribute("href").toStdString();
				float intProductPrice = elementProduct.findFirst("span.product > span.product-info-lite > div.primary-and-secondary-info > span.price > span.produs-listing-price-box > span.pret-produs-listing > span.price-over > span.money-int").toPlainText().trimmed().remove('.').toDouble() + elementProduct.findFirst("span.product > span.product-info-lite > div.primary-and-secondary-info > span.price > span.produs-listing-price-box > span.pret-produs-listing > span.price-over > sup.money-decimal").toPlainText().trimmed().remove('.').toDouble() / 100.0;
				BOOST_LOG_TRIVIAL(info) << "found product : "<< strProductTitle;
				BOOST_LOG_TRIVIAL(info) << "\t\turl: "<< strProductUrl;
				BOOST_LOG_TRIVIAL(info) << "\t\tprice: "<< intProductPrice;

				if (getPriceLimit() > intProductPrice) continue;

//                QWebElement sellerMarket = elementProduct.findFirst("form > div#pret2 > div.top > span.feedback-right-msg");
//                if (!sellerMarket.isNull()) {
//                    if (sellerMarket.toPlainText().toStdString().find("Vandut de eMAG") == std::string::npos) {
//                        continue; // not matching with current market
//                    }
//                }
				bool containsSkipWord = false;
				for (auto strWord: skipWordsProduct) {
					if (strProductUrl.find(strWord) != std::string::npos) containsSkipWord=true;
				}

				if (!containsSkipWord) {
//					products.push_back(MarketProduct(category.getId(), strProductTitle, strProductUrl));
//                                    qDebug() << QString::fromStdString(strProductTitle) << " : "<< QString::fromStdString(strProductUrl) << intProductPrice;
					prodCounter++;
				}
        }

//        BOOST_LOG_TRIVIAL(info) << "\tfound : "<< prodCounter << " products on page: "<< pageEMag.mainFrame()->url().toString().toStdString();
//        pageEMag.savePageImage("c:/tmp/ll/pricewatch_lastscreen.jpg");
		std::string strTarget = pageEMag.mainFrame()->findFirstElement("div.pagination > p ").toInnerXml().toStdString();
		std::smatch sm;
		std::string strProgress;
		std::regex regPage("([0-9]*) din [0-9]*");
		std::regex_search(strTarget, sm, regPage);
		BOOST_LOG_TRIVIAL(info) << "page: " << sm[0];



		nextPageButton =  pageEMag.mainFrame()->findFirstElement("a.listing-pagination-next");
		if (!nextPageButton.isNull()) {
			pageEMag.load("http://m.emag.ro" + nextPageButton.attribute("href").toStdString());

		}
    } while (!nextPageButton.isNull());

//    if (addProductsToDb(products)!=TaskResult::Completed) {
//        return Collect::Failed;
//    }
	return TaskResult::Failed;
}
