#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <exception>
#include <tuple>
#include <cmath>
#include <numeric>
#include <memory> 

// Aliasing various types for readability
using Price = std::int32_t; // Prices can be negative
using Quantity = std::uint32_t; // Quantity cannot be negative
using OrderId = std::uint64_t; // 64-bit for possibly large Order-id

// Supporting two types of orders
enum class OrderType {
  GoodTillCancel, // Order remains until explicitly canceled
  FillAndKill // Order is filled immediately or canceled
};

// Buy side -> bids; sell side -> asks
enum class Side {
  Buy, // Buy side of the order book
  Sell // Sell side of the order book
};

// Info about the state of one level in the order book
struct LevelInfo {
  Price price_; // Price level
  Quantity quantity_; // Total quantity at this price level
};

// Aliasing a vector of LevelInfos
using LevelInfos = std::vector<LevelInfo>;

// Class to hold order book level information
class OrderbookLevelInfos {
public:
  OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks) :
    bids_{bids},
    asks_{asks}
  {}

  const LevelInfos& GetBids() const { return bids_; }
  const LevelInfos& GetAsks() const { return asks_; }

private:
  LevelInfos bids_; // Bid levels
  LevelInfos asks_; // Ask levels
};

// Order objects will contain order type, order id, side, price, quantity, etc.
class Order {
public:
  Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity) :
    orderType_{orderType},
    orderId_{orderId},
    side_{side},
    price_{price},
    initialQuantity_{quantity},
    remainingQuantity_{quantity}
  {}

  OrderId GetOrderId() const { return orderId_; }
  Side GetSide() const { return side_; }
  Price GetPrice() const { return price_; }
  OrderType GetOrderType() const { return orderType_; }
  Quantity GetInitialQuantity() const { return initialQuantity_; }
  Quantity GetRemainingQuantity() const { return remainingQuantity_; }
  Quantity GetFilledQuantity() const { return initialQuantity_ - remainingQuantity_; }
  bool IsFilled() const { return GetRemainingQuantity() == 0; }

  // Fill the order with a specified quantity
  void Fill(Quantity quantity) {
    if (quantity > GetRemainingQuantity()) {
      throw std::runtime_error("Cannot fill more than the remaining quantity");
    }
    remainingQuantity_ -= quantity;
  }

private:
  OrderType orderType_; // Type of the order
  OrderId orderId_; // Unique identifier for the order
  Side side_; // Side of the order (buy/sell)
  Price price_; // Price of the order
  Quantity initialQuantity_; // Initial quantity of the order
  Quantity remainingQuantity_; // Remaining quantity to be filled
};

using OrderPointer = std::shared_ptr<Order>; // Pointer to Order objects
using OrderPointers = std::list<OrderPointer>; // List of Order pointers

// Class to modify existing orders
class OrderModify {
public:
  OrderModify(OrderId orderId, Side side, Price price, Quantity quantity) :
    orderId_{orderId},
    price_{price},
    side_{side},
    quantity_{quantity}
  {}

  OrderId GetOrderId() const { return orderId_; }
  Side GetSide() const { return side_; }
  Price GetPrice() const { return price_; }
  Quantity GetQuantity() const { return quantity_; }

  // API to transform an existing order with OrderModify into a new order
  OrderPointer ToOrderPointer(OrderType type) const {
    return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
  }

private:
  OrderId orderId_; // Order ID to modify
  Side side_; // Side of the order
  Price price_; // New price
  Quantity quantity_; // New quantity
};

// Structure to hold trade information
struct TradeInfo {
  OrderId orderId_; // Order ID involved in the trade
  Price price_; // Price at which trade occurred
  Quantity quantity_; // Quantity traded
};

// Class to represent a trade between two orders
class Trade {
public:
  Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade) :
    bidTrade_{bidTrade}, askTrade_{askTrade} {}

  const TradeInfo& GetBidTrade() const { return bidTrade_; }
  const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
  TradeInfo bidTrade_; // Trade information for the bid side
  TradeInfo askTrade_; // Trade information for the ask side
};

using Trades = std::vector<Trade>; // Vector of trades

// Class representing the order book
class Orderbook {
private:
  // Structure to hold order entry information
  struct OrderEntry {
    OrderPointer order_{nullptr}; // Pointer to the order
    OrderPointers::iterator location_; // Iterator to the order's location in the list
  };

  std::map<Price, OrderPointers, std::greater<Price>> bids_; // Bids sorted by price descending
  std::map<Price, OrderPointers> asks_; // Asks sorted by price ascending
  std::unordered_map<OrderId, OrderEntry> orders_; // Map of order IDs to order entries

  // Check if an order can be matched
  bool CanMatch(Side side, Price price) const {
    if (side == Side::Buy) {
      if (asks_.empty()) return false; // No asks to match against
      const auto& [bestAsk, _] = *asks_.begin(); // Get the best ask price
      return price >= bestAsk; // Can match if bid price is greater than or equal to best ask
    } else {
      if (bids_.empty()) return false; // No bids to match against
      const auto& [bestBid, _] = *bids_.begin(); // Get the best bid price
      return price <= bestBid; // Can match if ask price is less than or equal to best bid
    }
  }

  // Match orders in the order book
  Trades MatchOrders() {
    Trades trades;
    trades.reserve(orders_.size()); // Reserve space for trades

    while (true) {
      if (bids_.empty() || asks_.empty()) break; // Stop if no bids or asks

      auto& [bidPrice, bids] = *bids_.begin(); // Get the best bid
      auto& [askPrice, asks] = *asks_.begin(); // Get the best ask

      if (bidPrice < askPrice) break; // No match possible if best bid is less than best ask

      while (bids.size() && asks.size()) {
        auto& bid = bids.front(); // Get the first bid order
        auto& ask = asks.front(); // Get the first ask order

        // Determine the quantity to trade
        Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

        // Fill the orders
        bid->Fill(quantity);
        ask->Fill(quantity);

        // Remove filled orders
        if (bid->IsFilled()) {
          bids.pop_front();
          orders_.erase(bid->GetOrderId());
        }
        if (ask->IsFilled()) {
          asks.pop_front();
          orders_.erase(ask->GetOrderId());
        }
        if (bids.empty()) bids_.erase(bidPrice);
        if (asks.empty()) asks_.erase(askPrice);

        // Record the trade
        trades.push_back(Trade {
          TradeInfo{bid->GetOrderId(), bid->GetPrice(), quantity},
          TradeInfo{ask->GetOrderId(), ask->GetPrice(), quantity}
        });
      }
    }

    // Handle FillAndKill orders that remain unmatched
    if (!bids_.empty()) {
      auto& [_, bids] = *bids_.begin();
      auto& order = bids.front();
      if (order->GetOrderType() == OrderType::FillAndKill) CancelOrder(order->GetOrderId());
    }
    if (!asks_.empty()) {
      auto& [_, asks] = *asks_.begin();
      auto& order = asks.front();
      if (order->GetOrderType() == OrderType::FillAndKill) CancelOrder(order->GetOrderId());
    }

    return trades;
  }

public:
  // Add an order to the order book
  Trades AddOrder(OrderPointer order) {
    if (orders_.contains(order->GetOrderId())) return {}; // Ignore if order already exists
    if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice())) return {}; // Ignore if FillAndKill cannot be matched

    OrderPointers::iterator iterator;

    if (order->GetSide() == Side::Buy) {
      auto& orders = bids_[order->GetPrice()];
      orders.push_back(order);
      iterator = std::next(orders.begin(), orders.size() - 1); // Get iterator to the last element
    } else {
      auto& orders = asks_[order->GetPrice()];
      orders.push_back(order);
      iterator = std::next(orders.begin(), orders.size() - 1); // Get iterator to the last element
    }

    orders_.insert({ order->GetOrderId(), OrderEntry{order, iterator}});
    return MatchOrders(); // Attempt to match orders after adding
  }

  // Cancel an order in the order book
  void CancelOrder(OrderId orderid) {
    // Check if the order exists in the map
    if (!orders_.contains(orderid)) {
        std::cerr << "Debug: Order ID " << orderid << " does not exist in the order book." << std::endl;
        return; // Exit if order does not exist
    }

    // Retrieve the order and its iterator
    const auto& [order, orderIterator] = orders_.at(orderid);
    
    // Check if the order pointer is valid
    if (!order) {
        std::cerr << "Debug: Order pointer for ID " << orderid << " is null." << std::endl;
        return;
    }

    // Output order details for debugging
    std::cout << "Debug: Cancelling order ID: " << orderid << std::endl;
    std::cout << "Debug: Order side: " << (order->GetSide() == Side::Buy ? "Buy" : "Sell") << std::endl;
    std::cout << "Debug: Order price: " << order->GetPrice() << std::endl;

    // Cancel the order based on its side
    if (order->GetSide() == Side::Buy) {
        auto price = order->GetPrice();
        auto& orders = bids_.at(price);
        orders.erase(orderIterator); // Remove order from list
        if (orders.empty()) {
            std::cout << "Debug: Removing empty bid level at price: " << price << std::endl;
            bids_.erase(price); // Remove price level if empty
        }
    } else {
        auto price = order->GetPrice();
        auto& orders = asks_.at(price);
        orders.erase(orderIterator); // Remove order from list
        if (orders.empty()) {
            std::cout << "Debug: Removing empty ask level at price: " << price << std::endl;
            asks_.erase(price); // Remove price level if empty
        }
    }

    // Remove the order from the map
    orders_.erase(orderid);
    std::cout << "Debug: Order ID " << orderid << " cancelled successfully." << std::endl;
  }

  // Match an order modification
  Trades MatchOrder(OrderModify order) {
    if (!orders_.contains(order.GetOrderId())) return {}; // Ignore if order does not exist
    const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
    CancelOrder(order.GetOrderId()); // Cancel existing order
    return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType())); // Add modified order
  }

  // Get the size of the order book
  std::size_t Size() const { return orders_.size(); }

  // Get order book level information
  OrderbookLevelInfos GetOrderInfos() const {
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    // Lambda to create level information
    // This lambda function takes a price and a list of orders at that price level.
    // It calculates the total remaining quantity for all orders at this price level.
    // It returns a LevelInfo object containing the price and the total quantity.
    auto CreateLevelInfos = [](Price price, const OrderPointers& orders) {
        // std::accumulate to sum up the remaining quantities of all orders at this price level.
        // The initial value for the sum is 0, and the lambda function inside accumulate
        // adds the remaining quantity of each order to the running sum.
        return LevelInfo{ 
            price, 
            std::accumulate(orders.begin(), orders.end(), (Quantity)0,
                [](Quantity runningSum, const OrderPointer& order) {
                    return runningSum + order->GetRemainingQuantity(); 
                }
            ) 
        };
    };

    // Iterate over all bid price levels and create LevelInfo objects for each.
    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevelInfos(price, orders));

    // Iterate over all ask price levels and create LevelInfo objects for each.
    for (const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevelInfos(price, orders));

    // Return the order book level information for both bids and asks.
    return OrderbookLevelInfos{ bidInfos, askInfos };
  }
};

int main() {
    Orderbook orderbook;

    // Test adding a GoodTillCancel order
    const OrderId orderId1 = 1;
    auto order1 = std::make_shared<Order>(OrderType::GoodTillCancel, orderId1, Side::Buy, 100, 10);
    orderbook.AddOrder(order1);
    std::cout << "Orderbook size after adding GoodTillCancel order: " << orderbook.Size() << std::endl; // Expect 1

    // Test adding a FillAndKill order that can be matched
    const OrderId orderId2 = 2;
    auto order2 = std::make_shared<Order>(OrderType::FillAndKill, orderId2, Side::Sell, 100, 5);
    orderbook.AddOrder(order2);
    std::cout << "Orderbook size after adding FillAndKill order: " << orderbook.Size() << std::endl; // Expect 1

    // Test adding a FillAndKill order that cannot be matched
    const OrderId orderId3 = 3;
    auto order3 = std::make_shared<Order>(OrderType::FillAndKill, orderId3, Side::Sell, 110, 5);
    orderbook.AddOrder(order3);
    std::cout << "Orderbook size after adding unmatched FillAndKill order: " << orderbook.Size() << std::endl; // Expect 1

    // Test cancelling an order
    orderbook.CancelOrder(orderId1);
    std::cout << "Orderbook size after cancelling orderId1: " << orderbook.Size() << std::endl; // Expect 0

    // Test adding multiple orders
    const OrderId orderId4 = 4;
    const OrderId orderId5 = 5;
    auto order4 = std::make_shared<Order>(OrderType::GoodTillCancel, orderId4, Side::Buy, 95, 20);
    auto order5 = std::make_shared<Order>(OrderType::GoodTillCancel, orderId5, Side::Sell, 105, 15);
    orderbook.AddOrder(order4);
    orderbook.AddOrder(order5);
    std::cout << "Orderbook size after adding two more orders: " << orderbook.Size() << std::endl; // Expect 2

    // Test matching orders
    const OrderId orderId6 = 6;
    auto order6 = std::make_shared<Order>(OrderType::GoodTillCancel, orderId6, Side::Sell, 95, 20);
    orderbook.AddOrder(order6);
    std::cout << "Orderbook size after matching orders: " << orderbook.Size() << std::endl; // Expect 1

    // Test order modification
    OrderModify modifyOrder(orderId5, Side::Sell, 100, 10);
    orderbook.MatchOrder(modifyOrder);
    std::cout << "Orderbook size after modifying orderId5: " << orderbook.Size() << std::endl; // Expect 1

    // Final state of the order book
    auto orderInfos = orderbook.GetOrderInfos();
    std::cout << "Final bid levels:" << std::endl;
    for (const auto& level : orderInfos.GetBids()) {
        std::cout << "Price: " << level.price_ << ", Quantity: " << level.quantity_ << std::endl;
    }
    std::cout << "Final ask levels:" << std::endl;
    for (const auto& level : orderInfos.GetAsks()) {
        std::cout << "Price: " << level.price_ << ", Quantity: " << level.quantity_ << std::endl;
    }

    return 0;
}
