#include <QApplication>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/qwebframe.h>
#include <QTimer>

#include <boost/program_options.hpp>

#include <iostream>

#include "EMagMarket.hpp"

namespace  po = boost::program_options;
using namespace std;

static const vector<string> sitesAvailable {"emag.ro"};

int main(int argv, char **args) {
    QApplication app(argv, args);
    app.setApplicationName("PriceWatch");

    try {
        int priceLimit;
        int priceId=0;
        po::options_description desc ("Options");
        desc.add_options()
                ("help", "produce help message")
                ("list-sites", "list available sites")
                ("site", po::value<std::string>(), "select working site(ex. emag.ro)")
                ("fetch-categories", "fetch categories from sitle and update in database")
                ("fetch-products", "fetch products from site categories and uptade in database")
                ("fetch-price-limit", po::value<int>(&priceLimit)->default_value(100), "don't fetch products with price tag lower than this value")
                ("fetch-product-details", "fetch product details and update in database ")
                ("fetch-product-price", po::value<int>(&priceId), "fetch product price and add to database with current date")
                ;

        po::variables_map vm;
        po::store(po::parse_command_line(argv, args, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }
        if (vm.count("list-sites")) {
            cout << "available sites: " << "emag.ro" << "\n";
            return 0;
        }

        if (vm.count("fetch-product-price")) {
            QTimer::singleShot(2000, [&] {
                if (priceId==0) {
                    cout << "no price id defined";
                    return 1;
                }
                EMagMarket marketEMag;
                if (marketEMag.fetchProductPrice(priceId) == Collect::OK) {
                    cout << "successfully finished fetching product price...." << "\n";
                    marketEMag.deleteLater();
                    app.quit();
                } else {
                    cout << "failed fetching product price.." << "\n";
                    marketEMag.deleteLater();
                    app.quit();
                }
            });

            return app.exec();
        }

        string selectedSite;
        if (vm.count("site")) {
            selectedSite = vm["site"].as<std::string>();
        }  else {
            cout<< "no site selected for operation. please select one (--list sites)" << "\n";
            return 1;
        }

        if (!selectedSite.empty() && std::find(sitesAvailable.begin(), sitesAvailable.end(), selectedSite) == sitesAvailable.end()) {
            cout << "selected site not in available sites list" << "\n";
            return 1;
        }

        if (vm.count("fetch-categories")) {
            QTimer::singleShot(2000, [&] {
               EMagMarket marketEMag;
                if (marketEMag.fetchCategories() == Collect::OK) {
                    cout << "successfully finished fetching categories...." << "\n";
                    marketEMag.deleteLater();
                    app.quit();
                } else {
                    cout << "failed fetching categories.." << "\n";
                    marketEMag.deleteLater();
                    app.quit();
                }
            });

            return app.exec();
        }

        if (vm.count("fetch-products")) {
            QTimer::singleShot(2000, [&] {
                EMagMarket marketEMag;
                marketEMag.setPriceLimit(priceLimit);

                if (marketEMag.fetchProducts() == Collect::OK) {
                    cout << "successfully finished fetching products...." << "\n";
                    marketEMag.deleteLater();
                    app.quit();
                } else {
                    cout << "failed fetching products.." << "\n";
                    marketEMag.deleteLater();
                    app.quit();
                }
            });

            return app.exec();
        }
    }
    catch (exception &e) {
        cout << e.what() << "\n";
    }

    cout << "no argument specified: use --help for details" << "\n";
    return  0;
}