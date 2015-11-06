//
// Created by szall on 10/23/2015.
//

#ifndef PRICEWATCH_MARKETPRODUCT_HPP
#define PRICEWATCH_MARKETPRODUCT_HPP

#include <string>

class MarketProduct {
private:
public:
    MarketProduct(int intCatId, const std::string &Title, const std::string &ProductUrl) :
            catId(intCatId), Title(Title), ProductUrl(ProductUrl) { }
    MarketProduct(const int &intId, int intCatId, const std::string &Title, const std::string &ProductUrl) :
            Id(intId), catId(intCatId), Title(Title), ProductUrl(ProductUrl) { }

private:
public:
    const std::string &getIdentifier() const {
        return Identifier;
    }

    void setIdentifier(const std::string &Id) {
        MarketProduct::Identifier = Id;
    }

    const std::string &getTitle() const {
        return Title;
    }

    void setTitle(const std::string &Title) {
        MarketProduct::Title = Title;
    }

    const std::string &getProductUrl() const {
        return ProductUrl;
    }

    void setProductUrl(const std::string &ProductUrl) {
        MarketProduct::ProductUrl = ProductUrl;
    }

    double getPrice() const {
        return Price;
    }

    void setPrice(double Price) {
        MarketProduct::Price = Price;
    }

    int getCatId() const {
        return catId;
    }

    void setCatId(int catId) {
        MarketProduct::catId = catId;
    }

    int getId() const {
        return Id;
    }

    void setId(int id) {
        MarketProduct::Id = id;
    }

private:
    int Id;
    std::string Identifier;
    int catId=0;
    std::string Title;
    std::string ProductUrl;
    double  Price;

};


#endif //PRICEWATCH_MARKETPRODUCT_HPP
