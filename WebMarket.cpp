//
// Created by szall on 10/23/2015.
//

#include "WebMarket.hpp"

#include <QtSql>
#include <QProcess>
#include <QNetworkAccessManager>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>

#include "EMagWebPage.hpp"

#include <chrono>
#include <deque>
#include <thread>

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
	using namespace boost::asio;

	io_service ioservice;
	steady_timer timer{ioservice, std::chrono::seconds{i}};
	timer.async_wait([i](const boost::system::error_code &ec)
		{ BOOST_LOG_TRIVIAL (info) << "waiting "<< i << " sec done\n"; });

	BOOST_LOG_TRIVIAL (info) << "waiting "<< i << " sec\n"; 
	ioservice.run();
//    QEventLoop evFinishedWait;
//    QTimer bigTimeout;
//
//    BOOST_LOG_TRIVIAL(info) << "\twaiting: " << i << " seconds";
//    std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();
//    bigTimeout.singleShot(i * 1000, Qt::VeryCoarseTimer, &evFinishedWait, [&] {
//        std::chrono::time_point<std::chrono::system_clock> end_time = std::chrono::system_clock::now();
//        std::chrono::duration<double> elapsed_seconds = end_time-start_time;
////        BOOST_LOG_TRIVIAL(info) << "\twaited: " << elapsed_seconds.count();
//     if (bigTimeout.isActive())   bigTimeout.stop();
//      if (evFinishedWait.isRunning())  evFinishedWait.quit();
//    });
////    QTimer::singleShot(i * 1000,  &evFinishedWait, SLOT(quit()));
//
//    evFinishedWait.exec();
//
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

TaskResult WebMarket::addCategoriesToDb(std::vector<ProductCategory> &categories) {
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

TaskResult WebMarket::fetchCategoriesFromDb() {
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

	q.prepare("INSERT into prices (product_id, timestamp_id) SELECT id, 1 FROM products WHERE shop_id=" + QString::number(getMarketId()));
	if (!q.exec()) {
		BOOST_LOG_TRIVIAL(error) << "failed to generate prices for timestamp: " << timestamp;
		return TaskResult::Failed;
	}
	return TaskResult::Completed;
}

TaskResult WebMarket::fetchPrices() {
	QSqlQuery q;
	BOOST_LOG_TRIVIAL(info) << "running " << getMaxProcess() << " processes";

	bool morePricesToFetch = true; // is there more price to fetch

	while (morePricesToFetch) {
		// try to get next unprocessed priceid
		q.prepare("SELECT id FROM prices WHERE price=0 LIMIT 1");

		if(!q.exec()) {
			BOOST_LOG_TRIVIAL(error) << "failed to get available prices from db";
			return TaskResult::Failed;
		} 
		
		int nextPriceId=0;
		if (q.first()) {
			nextPriceId=q.value(0).toInt();
		} else {
			morePricesToFetch=false;
			continue;
		}

		// mark priceid with -1 price value to skip on next request
		q.prepare("UPDATE prices SET price=-1 WHERE id=" + QString::number(nextPriceId));

		if(!q.exec()) {
			BOOST_LOG_TRIVIAL(error) << "failed to get available prices from db";
			return TaskResult::Failed;
		} 

		BOOST_LOG_TRIVIAL(info) << "find unprocessed priceid:" << nextPriceId;

		while (getRunningWorkers() > getMaxProcess()) {
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}

		QStringList arguments;
		arguments.append("--fetch-product-price");
		arguments.append(QString::number(nextPriceId));

		QProcess *tmpProc = new QProcess(); 

		tmpProc->setWorkingDirectory(qApp->applicationDirPath());
		tmpProc->setProcessChannelMode(QProcess::MergedChannels);
		if (!tmpProc->startDetached("pricewatch.exe", arguments)) {
			BOOST_LOG_TRIVIAL(error) << tmpProc->errorString().toStdString();
			BOOST_LOG_TRIVIAL(info) << tmpProc->readAll().toStdString();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	} 
	return TaskResult::Failed;
}

int WebMarket::getRunningWorkers() {
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setReadChannelMode(QProcess::MergedChannels);

    process.start(tr("tasklist /FI \"IMAGENAME eq pricewatch*\" /FO CSV  /NH"));

    process.waitForStarted(1000);
    process.waitForFinished(1000);
    process.waitForReadyRead(2000);

    QString std_out = QString(process.readAllStandardOutput());

    QStringList out_lines = std_out.split("\n", QString::SkipEmptyParts);
    int count = out_lines.filter("pricewatch.exe").count();

    if (count>-1) return count;
    return -1;
}
