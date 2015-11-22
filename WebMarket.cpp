//
// Created by szall on 10/23/2015.
//

#include "WebMarket.hpp"

#include <QtSql>

#include <chrono>

WebMarket::WebMarket() {
       BOOST_LOG_TRIVIAL(info) << "connecting to db";
//        db = QSqlDatabase::addDatabase("QMYSQL", QString::fromStdString(genRandStr(10)));
        if (!QSqlDatabase::database().isOpen()) {
            db = QSqlDatabase::addDatabase("QMYSQL");
            db.setHostName("localhost");
            db.setDatabaseName("pricewatch");
            db.setUserName("root");
            bool ok = db.open();
            if (!ok) {
                BOOST_LOG_TRIVIAL(fatal) << "connecting to database failed: " << db.lastError().text().toStdString();
                return;
            }
        } else {
            db = QSqlDatabase::database();
        }
}

Wait WebMarket::waitSeconds(int i) {
    QEventLoop evFinishedWait;
    QTimer bigTimeout;

    BOOST_LOG_TRIVIAL(info) << "\twaiting: " << i << " seconds";
    std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();
    bigTimeout.singleShot(i * 1000, Qt::VeryCoarseTimer, &evFinishedWait, [&] {
        std::chrono::time_point<std::chrono::system_clock> end_time = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time-start_time;
//        BOOST_LOG_TRIVIAL(info) << "\twaited: " << elapsed_seconds.count();
     if (bigTimeout.isActive())   bigTimeout.stop();
      if (evFinishedWait.isRunning())  evFinishedWait.quit();
    });
//    QTimer::singleShot(i * 1000,  &evFinishedWait, SLOT(quit()));

    evFinishedWait.exec();

//    BOOST_LOG_TRIVIAL(info) << "\twaitSeconds finish";
    return Wait::OK;
}

bool WebMarket::equalProductsAndDescriptions() {
    QSqlQuery q;
    int countProducts=0;

    q.prepare("SELECT COUNT(*) FROM products");

    if (!q.exec()){
        BOOST_LOG_TRIVIAL (error) << "\tfailed to count products. " << q.lastError().text().toStdString();
        return false;
    }

    if(q.first()) {
        countProducts = q.value(0).toInt();
    }

    int countProductDetails=0;

    q.prepare("SELECT COUNT(*) FROM productdetails");

    if (!q.exec()){
        BOOST_LOG_TRIVIAL (error) << "\tfailed to count productdetails. " << q.lastError().text().toStdString();
        return false;
    }

    if(q.first()) {
        countProductDetails= q.value(0).toInt();
    }

    return countProducts == countProductDetails;
}

TaskResult WebMarket::addProductsToDb(std::vector<MarketProduct> &products) {
    QSqlQuery q;
    std::vector<MarketProduct> dbProducts;

    // fetch all existing products from db for later check
    q.prepare("SELECT productdetails.id, products.cat_id, title, url, shop_id FROM productdetails LEFT JOIN products ON products.id=productdetails.id WHERE shop_id = 1 ");

    if (!q.exec()){
        BOOST_LOG_TRIVIAL (error) << "\tfailed to fetch products for check. " << q.lastError().text().toStdString();
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
        BOOST_LOG_TRIVIAL(error) << "\tfailed to fetch product. " << q.lastError().text().toStdString();
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
        if (!equalProductsAndDescriptions()) {
            BOOST_LOG_TRIVIAL(fatal) << "something wrong with products count (productCount != productDetailsCount)!";
            BOOST_LOG_TRIVIAL (fatal) << product.getTitle() << " : " << product.getProductUrl() << " : " <<product.getCatId();
            return TaskResult::Failed;
        }
        auto resultFind = std::find_if(dbProducts.begin(), dbProducts.end(), [&](const MarketProduct prod) {return prod.getProductUrl()==product.getProductUrl();});
        if (resultFind != dbProducts.end()) { // product exist in db
        } else {
            if (product.getTitle().empty()) continue; // ship empty product names
//            product.getTitle().replace("\"", "\\\""); //escape double quotes

            // insert to details table
            if (!q.prepare(QLatin1String("INSERT INTO productdetails (id, title, url) VALUES(?, ?, ?)"))) {
                BOOST_LOG_TRIVIAL(fatal) << "failed to prepare productdetails ";
                return TaskResult::Failed;
            }

            q.addBindValue(lastIndex);
            q.addBindValue(QString::fromStdString(product.getTitle()));
            q.addBindValue(QString::fromStdString(product.getProductUrl()));
            if (!q.exec()) {
                BOOST_LOG_TRIVIAL(error) << "addProductsToDb(productdetails) error: " << q.lastError().text().toStdString() << product.getCatId();
                continue; //jump to next product if something failes
            }

            //insert into products table
            if (!q.prepare(QLatin1String("INSERT INTO products (id, shop_id, cat_id) VALUES(?, ?, ?)"))){
                BOOST_LOG_TRIVIAL(fatal) << "failed to prepare productdetails ";
                return TaskResult::Failed;
            }

            q.addBindValue(lastIndex);
            q.addBindValue(1);
            q.addBindValue(product.getCatId());
            if (!q.exec()) {
                BOOST_LOG_TRIVIAL(error) << "addProductsToDb(products) error: " << q.lastError().text().toStdString() << product.getCatId();
            }

            dbProducts.push_back(MarketProduct(product.getCatId(), product.getTitle(), product.getProductUrl()));
            lastIndex++;
        }
    }

    return TaskResult::Completed;
}

TaskResult WebMarket::generatePricesForTimestamp(std::string timestamp) {
    QSqlQuery q;
    int timestampId=0;

	//check if allready exits
	q.prepare("SELECT id FROM timestamps WHERE timedate='" + QString::fromStdString(timestamp) + "'");
	if (q.exec()) {
		if (q.first()) {
			timestampId = q.value(0).toInt();
			BOOST_LOG_TRIVIAL(info) << "found timestamp in db with id:" << std::to_string(timestampId);
		}
	} else {
		BOOST_LOG_TRIVIAL (error) << "\tfailed to find timestamp" << q.lastError().text().toStdString();
	}

	if (timestampId==0) { // timestamp not found in db, then generate new one 
		q.prepare("INSERT INTO `pricewatch`.`timestamps` (`timedate`) VALUES ('" +QString::fromStdString(timestamp) + "');");

		if (!q.exec()){
			BOOST_LOG_TRIVIAL (error) << "\tfailed to create new timestamps" << q.lastError().text().toStdString();
			return TaskResult::Failed;
		} else {
			q.exec("SELECT LAST_INSERT_ID();"); //  get last inserted timestamp id
		}

		if(q.first()) {
			timestampId = q.value(0).toInt();
		}
	}

	if (timestampId==0) {
		BOOST_LOG_TRIVIAL(error) << "failed to generate/get timestamp id";
		return TaskResult::Failed;
	} 

	q.prepare("INSERT into prices (product_id, timestamp_id) SELECT id, 1 FROM products");
	if (!q.exec()) {
		BOOST_LOG_TRIVIAL(error) << "failed to generate prices for timestamp: " << timestamp;
		return TaskResult::Failed;
	}
	return TaskResult::Completed;
}
