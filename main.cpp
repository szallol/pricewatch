#include <QApplication>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/qwebframe.h>
#include <QTimer>

#include <boost/program_options.hpp>
#include <iostream>
#include "EMagMarket.hpp"

namespace  po = boost::program_options;
using namespace std;

int main(int argv, char **args) {
    QApplication app (argv, args);

    po::options_description desc ("Options");
    desc.add_options()
            ("help", "produce help message")
            ("compression", po::value<int>(), "set compression level")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argv, args, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    EMagMarket marketEMag;

//    QTimer::singleShot(60000, [&] {
//        app.exit(0);
//    });
    return app.exec();
}