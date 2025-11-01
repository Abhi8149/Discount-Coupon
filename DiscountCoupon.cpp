#include <vector>
#include <string>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

class DiscountStrategy{
  public:
  ~DiscountStrategy(){}
  virtual double calculate(double baseamount)=0;
};

class FlatDiscountStrategy:public DiscountStrategy{
    private:
    double amount;

    public:
    FlatDiscountStrategy(double amt){
        amount=amt;
    }

    double calculate(double baseamount){
        return min(amount,baseamount);
    }
};

class PercentDiscountStrategy:public DiscountStrategy{
    private:
    double percent;

    public:

    PercentDiscountStrategy(double perc){
        percent=perc;
    }

    double calculate(double baseamount){
        return (percent/100.0)*baseamount;
    }
};

class PercentWithCapStrategy:public DiscountStrategy{
    private:
    double percent;
    double cap;

    public:
    PercentWithCapStrategy(double per, double c){
        percent=per;
        cap=c;
    }

    double calculate(double baseamount){
        double disc=(percent/100.0)*baseamount;
        if(disc>cap){
            return cap;
        }

        return disc;
    }
};

enum StrategyType{
    FLAT,
    PERCENT,
    PERCENT_CAP
};

class DiscountStrategyManager{
    private:
    static DiscountStrategyManager* instance;
    DiscountStrategyManager() {}
    DiscountStrategyManager(const DiscountStrategyManager&) = delete;
    DiscountStrategyManager& operator=(const DiscountStrategyManager&) = delete;

    public:
    
    static DiscountStrategyManager* getInstance(){
        if(!instance){
            instance=new DiscountStrategyManager();
        }

        return instance;
    }

    DiscountStrategy* getStrategy(StrategyType type, double param1, double param2=0.0) const {
        if(type==StrategyType::FLAT){
            return new FlatDiscountStrategy(param1);
        }

        if(type==StrategyType::PERCENT){
            return new PercentDiscountStrategy(param1);
        }

        if(type==StrategyType::PERCENT_CAP){
            return new PercentWithCapStrategy(param1,param2);
        }

        return nullptr;
    }
};

DiscountStrategyManager* DiscountStrategyManager::instance=nullptr;

class Product{
    private:
    string name;
    string category;
    double price;

    public:

    Product(const string& n, const string&cat, double p){
        name=n;
       category=cat;
       price=p;
    }

    string getName(){
        return name;
    }

    string getCategory(){
        return category;
    }

    double getPrice(){
        return price;
    }
};

class CartItem{
    private:
    Product *product;
    int qty;

    public:
    CartItem(Product *p, int q){
       product=p;
       qty=q;
    }

    double itemCost(){
        return product->getPrice()*qty;
    }

    Product *getProduct(){
        return product;
    }
};

class Cart{
    private:
    vector<CartItem*>items;
    bool loyalmember;
    double originalcost;
    double currentcost;
    string bank;

    public:

    void addProduct(Product *product,int qty=1){
        CartItem *item=new CartItem(product,qty);
        items.push_back(item);
        originalcost+=item->itemCost();
        currentcost+=item->itemCost();
    }

    double applyDiscount(double amount){
        currentcost-=amount;
        if(currentcost<0){
            currentcost=0;
        }

        return currentcost;
    }

    double getOriginalCost(){
        return originalcost;
    }

    double getCurrentCost(){
        return currentcost;
    }

    void setloyalitymember(bool member){
        loyalmember=member;
    }

    bool isloyalmember(){
        return loyalmember;
    }

    void setBank(const string& b){
        bank=b;
    }

    string getPaymentBank(){
        return bank;
    }

    vector<CartItem*>getItems(){
        return items;
    }

};

class Coupon{
    private:
    Coupon *next;

    public:

    Coupon(){
        next=nullptr;
    }

    void setNext(Coupon *cop){
        next=cop;
    }

    Coupon *getNext(){
        return next;
    }

    void applyDiscount(Cart *cart){
        if(isApplicable(cart)){
            double disc=getDiscount(cart);
            cart->applyDiscount(disc);
            cout << name() << " applied: " << disc << endl;
            if (!isCombinable()) {
                return;
            }
        }

        if(next){
            next->applyDiscount(cart);
        }

    }

    virtual bool isApplicable(Cart* cart)=0;
    virtual double getDiscount(Cart *cart)=0;
    virtual bool isCombinable(){
        return true;
    }

    virtual string name()=0;
};

class SeasonalOffer:public Coupon{
    private:
    double percent;
    string category;
    DiscountStrategy *strat;

    public:
    SeasonalOffer(double per, string cat){
        percent=per;
        category=cat;
        strat=DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT, percent);
    }

    bool isApplicable(Cart *cart) override{
        for(auto item: cart->getItems()){
            if(item->getProduct()->getCategory()==category){
                return true;
            }
        }

        return false;
    }

    double getDiscount(Cart *cart) override{
        double total=0.0;
        for(CartItem *item: cart->getItems()){
            if(item->getProduct()->getCategory()==category){
                total+=item->itemCost();
            }
        }

        return strat->calculate(total);
    }

    bool isCombinable() override{
        return true;
    }

    string name() override {
        return "Seasonal Offer " + to_string((int)percent) + " % off " + category;
    }
};

class LoyalityDiscount: public Coupon{
    private:
    double percent;
    DiscountStrategy *strat;

    public:

    LoyalityDiscount(double per){
        percent=per;
        strat=DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT,percent);
    }

    bool isApplicable(Cart *cart) override{
        return cart->isloyalmember();
    }

    double getDiscount(Cart *cart) override{
        return strat->calculate(cart->getCurrentCost());
    }

    bool isCombinable() override{
        return true;
    }

    string name() override {
        return "loyality Offer " + to_string((int)percent) + " % off " ;
    }
};

class BulkPurchaseDiscount:public Coupon{
    private:
    double threshold;
    double flatoff;
    DiscountStrategy *strat;

    public:
    BulkPurchaseDiscount(double thres, double flat){
        threshold=thres;
        flatoff=flat;
        strat=DiscountStrategyManager::getInstance()->getStrategy(StrategyType::FLAT, flatoff);
    }

    bool isApplicable(Cart *cart) override{
        return cart->getOriginalCost()>=threshold;
    }

    double getDiscount(Cart *cart){
        return strat->calculate(cart->getCurrentCost());
    }

    bool isCombinable() override{
        return true;
    }

    string name() override {
        return "BulkPurchase Offer " + to_string((int)flatoff) + " % off " ;
    }
};

class BankCoupon:public Coupon{
    private:
    string bank;
    double minSpend;
    double percent;
    double offcap;
    DiscountStrategy *strat;

    public:
    
    BankCoupon(string b, double ms, double per, double cap){
        bank=b;
        minSpend=ms;
        offcap=cap;
        percent=per;
        strat=DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT_CAP, percent, offcap);
    }

    bool isApplicable(Cart *cart) override{
        return cart->getPaymentBank()==bank;
    }

    double getDiscount(Cart *cart) override{
        return strat->calculate(cart->getCurrentCost());
    }

    bool isCombinable() override{
        return true;
    }

    string name() override {
        return "Bank Offer " + to_string((int)offcap) + " % off "+"on miniumum spend of"+ to_string((int)minSpend);
    }
};

class CouponManager{
    private:
    static CouponManager *instance;
    Coupon *head;
    mutable mutex mtx;

    CouponManager(){
        head=nullptr;
    }

    public:

    static CouponManager *getInstance(){
        if(!instance){
            instance=new CouponManager();
        }

        return instance;
    }

    void registerCoupon(Coupon *c){
        lock_guard<mutex>lock(mtx);
        if(!head){
            head=c;
        }
        else{
            Coupon *curr=head;
            while(curr->getNext()){
                curr=curr->getNext();
            }

            curr->setNext(c);
        }
    }

    double applyAllCoupon(Cart *cart){
        lock_guard<mutex>lock(mtx);
        if(head){
            head->applyDiscount(cart);
        }

        return cart->getCurrentCost();
    }

    vector<string> getApplicableCoupon(Cart *cart){
        lock_guard<mutex>lock(mtx);
        vector<string>res;
        Coupon *curr=head;
        while(curr){
            if(curr->isApplicable(cart)){
                res.push_back(curr->name());
            }

            curr=curr->getNext();
        }

        return res;
    }
};

CouponManager* CouponManager::instance = nullptr;

int main(){

    CouponManager* mgr = CouponManager::getInstance();
    mgr->registerCoupon(new SeasonalOffer(10, "Clothing"));
    mgr->registerCoupon(new LoyalityDiscount(5));
    mgr->registerCoupon(new BulkPurchaseDiscount(1000, 100));
    mgr->registerCoupon(new BankCoupon("ABC", 2000, 15, 500));

    Product* p1 = new Product("Winter Jacket", "Clothing", 1000);
    Product* p2 = new Product("Smartphone", "Electronics", 20000);
    Product* p3 = new Product("Jeans", "Clothing", 1000);
    Product* p4 = new Product("Headphones", "Electronics", 2000);


    Cart* cart = new Cart();
    cart->addProduct(p1, 1);
    cart->addProduct(p2, 1);
    cart->addProduct(p3, 2);
    cart->addProduct(p4, 1);
    cart->setloyalitymember(true);
    cart->setBank("ABC");

    cout << "Original Cart Total: " << cart->getOriginalCost() << " Rs" << endl;

    vector<string> applicable = mgr->getApplicableCoupon(cart);
    cout << "Applicable Coupons:" << endl;
    for (string name : applicable) {
        cout << " - " << name << endl;
    }

    double finalTotal = mgr->applyAllCoupon(cart);
    cout << "Final Cart Total after discounts: " << finalTotal << " Rs" << endl;

    delete p1;
    delete p2;
    delete p3;
    delete p4;
    delete cart;

    return 0;
}

