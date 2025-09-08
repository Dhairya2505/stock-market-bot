#include <iostream>
#include <random>
#include <string>
#include <map>
#include <thread>
#include <unistd.h>
#include <mutex>
using namespace std;

class OrderBook
{
private:
    string name;
    double current_price;
    double base_price;
    map<double, int> asks;
    map<double, int, greater<double>> bids;

    mutable mutex book_mutex;

    int fill_asks(double price, int quantity)
    {
        if (asks.begin() == asks.end())
            return quantity;
        auto it = asks.begin();
        if (it->first <= price)
        {
            if (asks[it->first] > quantity)
            {
                asks[it->first] -= quantity;
                this->current_price = it->first;
                return 0;
            }
            else
            {
                while (it->first <= price && quantity > 0 && it != asks.end())
                {
                    if (asks[it->first] > quantity)
                    {
                        asks[it->first] -= quantity;
                        this->current_price = it->first;
                        return 0;
                    }
                    else
                    {
                        quantity -= asks[it->first];
                        this->current_price = it->first;
                        it = asks.erase(it);
                    }
                }
                return quantity;
            }
        }
        else
        {
            return quantity;
        }
    }

    int fill_bids(double price, int quantity)
    {
        if (bids.begin() == bids.end())
            return quantity;
        auto it = bids.begin();
        if (it->first >= price)
        {
            if (bids[it->first] > quantity)
            {
                bids[it->first] -= quantity;
                this->current_price = it->first;
                return 0;
            }
            else
            {
                while (it->first >= price && quantity > 0 && it != bids.end())
                {
                    if (bids[it->first] > quantity)
                    {
                        bids[it->first] -= quantity;
                        this->current_price = it->first;
                        return 0;
                    }
                    else
                    {
                        quantity -= bids[it->first];
                        this->current_price = it->first;
                        it = bids.erase(it);
                    }
                }
                return quantity;
            }
        }
        else
        {
            return quantity;
        }
    }

public:
    OrderBook(string name, double price)
    {
        this->name = name;
        this->current_price = price;
        this->base_price = price;
    }

    double get_price()
    {
        lock_guard<mutex> lock(book_mutex);
        return this->current_price;
    }

    double get_base_price()
    {
        return this->base_price;
    }

    map<double, int> get_asks()
    {
        lock_guard<mutex> lock(book_mutex);
        return this->asks;
    }

    map<double, int, greater<double>> get_bids()
    {
        lock_guard<mutex> lock(book_mutex);
        return this->bids;
    }

    void place_bids(double price, int quantity)
    {
        lock_guard<mutex> lock(book_mutex);
        int remaining_quantity = fill_asks(price, quantity);
        if (remaining_quantity > 0)
        {
            bids[price] += remaining_quantity;
        }
    }

    void place_asks(double price, int quantity)
    {
        lock_guard<mutex> lock(book_mutex);
        int remaining_quantity = fill_bids(price, quantity);
        if (remaining_quantity > 0)
        {
            asks[price] += remaining_quantity;
        }
    }
};

auto round_to_2 = [](double value)
{
    return std::round(value * 100.0) / 100.0;
};

void buyer(OrderBook &ob)
{

    static thread_local random_device rd;
    static thread_local mt19937 gen(rd());
    uniform_real_distribution<> gen_trade(0.0, 1.0);
    uniform_real_distribution<> perc(0, 50);
    uniform_int_distribution<> qty_dist(1, 10);

    while (1)
    {
        double x = gen_trade(gen) * 100.0;
        double change = perc(gen) / 10.0;
        int qty = qty_dist(gen);
        double price;
        if (x > 1.0)
        {
            price = ob.get_price() * (1.0 - change / 100.0);
        }
        else
        {
            price = ob.get_price() * (1.0 + change / 100.0);
        }
        ob.place_bids(round_to_2(price), qty);
    }
}

void seller(OrderBook &ob)
{

    static thread_local random_device rd;
    static thread_local mt19937 gen(rd());
    uniform_real_distribution<> gen_trade(0.0, 1.0);
    uniform_real_distribution<> perc(0, 50);
    uniform_int_distribution<> qty_dist(1, 10);

    while (1)
    {

        double x = gen_trade(gen) * 100.0;
        double change = perc(gen) / 10.0;
        int qty = qty_dist(gen);
        double price;
        if (x > 1.0)
        {
            price = ob.get_price() * (1.0 + change / 100.0);
        }
        else
        {
            price = ob.get_price() * (1.0 - change / 100.0);
        }
        ob.place_asks(round_to_2(price), qty);
    }
}

void print_orderBook(OrderBook &ob)
{
    while (1)
    {
        map<double, int> asks = ob.get_asks();
        map<double, int, greater<double>> bids = ob.get_bids();
        system("cls");
        
        int count = 0;
        vector<pair<double, int>> first10;
        for (auto &p : asks)
        {
            first10.push_back(p);
            if (++count == 10)
                break;
        }
        for (auto it = first10.rbegin(); it != first10.rend(); ++it)
        {
            cout << it->first << " " << it->second << endl;
        }

        cout << "---------------" << endl;
        cout << ob.get_price() << endl;
        cout << "---------------" << endl;

        count = 0;
        for (auto &p : bids)
        {
            cout << p.first << " " << p.second << endl;
            if (++count == 10)
                break;
        }
        sleep(1);
    }
}

int main()
{
    OrderBook ob("GOOGLE", 100.0);

    thread t1(buyer, ref(ob));
    thread t2(seller, ref(ob));
    thread t3(print_orderBook, ref(ob));

    t1.join();
    t2.join();
    t3.join();

    return 0;
}