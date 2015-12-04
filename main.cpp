#pragma clang diagnostic ignored "-Wignored-attributes"

#include <QApplication>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/qwebframe.h>
#include <QTimer>

#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>

#include <iostream>
#include <thread>

#include "EMagMarket.hpp"

namespace  po = boost::program_options;
using namespace std;

static const vector<string> sitesAvailable {"emag.ro", "altex.ro"};

int main(int argv, char **args) {
    BOOST_LOG_TRIVIAL(info) << "pricewatch..." ;
    QApplication app(argv, args);
    app.setApplicationName("PriceWatch");

    try {
        int priceLimit;
        int priceId=0;
		std::string dateStamp;
		int maxProcess;

        po::options_description desc ("Options");
        desc.add_options()
                ("list-sites", "list available sites")
                ("site", po::value<std::string>(), "select working site(ex. emag.ro)")
                ("fetch-categories", "fetch categories from sitle and update in database")
                ("fetch-products", "fetch products from site categories and uptade in database")
                ("fetch-price-limit", po::value<int>(&priceLimit)->default_value(100), "don't fetch products with price tag lower than this value")
                ("fetch-product-details", "fetch product details and update in database ")
                ("fetch-product-price", po::value<int>(&priceId), "fetch product price and add to database with current date")
				("generate-prices", po::value<std::string>(&dateStamp), "generate new empty price list for specified date (ex.2015-11-20)")
				("fetch-prices", po::value<int>(&maxProcess)->default_value(10), "daemon mode to update unfetched prices (arg-> max process number, default=10)")
                ("help", "produce help message")
                ;

        po::variables_map vm;
        po::store(po::parse_command_line(argv, args, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }
        if (vm.count("list-sites")) {
            BOOST_LOG_TRIVIAL(info) << "available sites: " << "emag.ro" << "\n";
            return 0;
        }

        if (vm.count("fetch-product-price")) {
//            QTimer::singleShot(2000, [&] {
                if (priceId==0) {
                    BOOST_LOG_TRIVIAL(error) << "no price id defined";
                    return 1;
                }
                EMagMarket marketEMag{};

                if (marketEMag.fetchProductPrice(priceId) == Collect::OK) {
                    BOOST_LOG_TRIVIAL(info) << "successfully finished fetching product price...." << "\n";
                    marketEMag.deleteLater();
					return 0;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "failed fetching product price.." << "\n";
                    marketEMag.deleteLater();
					return 1;
                }
//            });
        }

		if (vm.count("generate-prices")) {
			EMagMarket marketEMag;

			if (dateStamp=="") {
				BOOST_LOG_TRIVIAL(error) << "no timestamp specified";
				return 1;
			}
			if (marketEMag.generatePricesForTimestamp(dateStamp)==TaskResult::Completed) {
				 BOOST_LOG_TRIVIAL(info) << "successfully generated prices for timestamp: " << dateStamp;
				 return 0;
			} else {
				 BOOST_LOG_TRIVIAL(error) << "failed to generated prices for timestamp: " << dateStamp;
				 return 1;
			}
		}

		if (vm.count("fetch-prices")) {
			EMagMarket marketEMag;

			marketEMag.setMaxParocess(maxProcess);

			if (marketEMag.fetchPrices() != TaskResult::Completed) {
				 BOOST_LOG_TRIVIAL(info) << "successfully fetched prices" << dateStamp;
				 return 0;
			} else {
				 BOOST_LOG_TRIVIAL(error) << "failed to fetch prices" << dateStamp;
				 return 1;
			}
		}

        string selectedSite;
		if (vm.count("site")) {
            selectedSite = vm["site"].as<std::string>();
        }  else {
            BOOST_LOG_TRIVIAL(error)<< "no site selected for operation. please select one (--list sites)" << "\n";
            return 1;
        }

        if (!selectedSite.empty() && std::find(sitesAvailable.begin(), sitesAvailable.end(), selectedSite) == sitesAvailable.end()) {
            BOOST_LOG_TRIVIAL(error) << "selected site not in available sites list" << "\n";
            return 1;
        }

        if (vm.count("fetch-categories")) {
            QTimer::singleShot(2000, [&] {
               EMagMarket marketEMag;
                if (marketEMag.fetchCategories() == Collect::OK) {
                    BOOST_LOG_TRIVIAL(info) << "successfully finished fetching categories...." << "\n";
                    marketEMag.deleteLater();
                } else {
                    BOOST_LOG_TRIVIAL(error) << "failed fetching categories.." << "\n";
                    marketEMag.deleteLater();
                }
            });
        }

        if (vm.count("fetch-products")) {
            QTimer::singleShot(2000, [&] {
                EMagMarket marketEMag;
                marketEMag.setPriceLimit(priceLimit);

                if (marketEMag.fetchProducts() == Collect::OK) {
                    BOOST_LOG_TRIVIAL(info) << "successfully finished fetching products...." << "\n";
                    marketEMag.deleteLater();
					return 0;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "failed fetching products.." << "\n";
                    marketEMag.deleteLater();
					return 1;
                }
            });
        }


        app.exec();
    }
    catch (exception &e) {
        cout << e.what() << "\n";
    }

    BOOST_LOG_TRIVIAL(error) << "no argument specified: use --help for details" << "\n";
    return  0;
}
