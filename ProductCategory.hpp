//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_PRODUCTCATEGORY_HPP
#define PRICEWATCH_PRODUCTCATEGORY_HPP


#include <string>

class ProductCategory {
private:
public:
    ProductCategory(int intShopId, const std::string &strName, std::string strCategoryUrl)
            : shopId(intShopId), Name(strName), CategoryUrl(strCategoryUrl) { }

public:
    const std::string &getName() const {
        return Name;
    }

    const std::string &getCategoryUrl() const {
        return CategoryUrl;
    }

    int getShopId() const {
        return shopId;
    }

    int getId() const {
        return Id;
    }

    void setId(int Id) {
        ProductCategory::Id = Id;
    }

private:
    int Id=0;
    int shopId;
    std::string Name;
    std::string CategoryUrl;
};


#endif //PRICEWATCH_PRODUCTCATEGORY_HPP
