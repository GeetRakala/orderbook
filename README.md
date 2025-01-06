# Order Book System

This project implements a simple order book system for managing buy and sell orders. It supports two types of orders: `GoodTillCancel` and `FillAndKill`. The system can add, cancel, and modify orders, as well as match them to generate trades.

## Data Structures

### Enums

- **OrderType**: Represents the type of order.
  - `GoodTillCancel`: Order remains until explicitly canceled.
  - `FillAndKill`: Order is filled immediately or canceled.

- **Side**: Represents the side of the order.
  - `Buy`: Buy side of the order book.
  - `Sell`: Sell side of the order book.

### Structs

- **LevelInfo**: Represents the state of one level in the order book.
  - `Price price_`: Price level.
  - `Quantity quantity_`: Total quantity at this price level.

- **TradeInfo**: Holds trade information.
  - `OrderId orderId_`: Order ID involved in the trade.
  - `Price price_`: Price at which trade occurred.
  - `Quantity quantity_`: Quantity traded.

## Classes

### OrderbookLevelInfos

- **Constructor**: `OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)`
  - Initializes with bid and ask level information.

- **GetBids**: `const LevelInfos& GetBids() const`
  - Returns bid level information.

- **GetAsks**: `const LevelInfos& GetAsks() const`
  - Returns ask level information.

### Order

- **Constructor**: `Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)`
  - Initializes an order with specified parameters.

- **GetOrderId**: `OrderId GetOrderId() const`
  - Returns the order ID.

- **GetSide**: `Side GetSide() const`
  - Returns the side of the order.

- **GetPrice**: `Price GetPrice() const`
  - Returns the price of the order.

- **GetOrderType**: `OrderType GetOrderType() const`
  - Returns the type of the order.

- **GetInitialQuantity**: `Quantity GetInitialQuantity() const`
  - Returns the initial quantity of the order.

- **GetRemainingQuantity**: `Quantity GetRemainingQuantity() const`
  - Returns the remaining quantity to be filled.

- **GetFilledQuantity**: `Quantity GetFilledQuantity() const`
  - Returns the filled quantity of the order.

- **IsFilled**: `bool IsFilled() const`
  - Checks if the order is completely filled.

- **Fill**: `void Fill(Quantity quantity)`
  - Fills the order with a specified quantity.

### OrderModify

- **Constructor**: `OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)`
  - Initializes an order modification with specified parameters.

- **GetOrderId**: `OrderId GetOrderId() const`
  - Returns the order ID to modify.

- **GetSide**: `Side GetSide() const`
  - Returns the side of the order.

- **GetPrice**: `Price GetPrice() const`
  - Returns the new price.

- **GetQuantity**: `Quantity GetQuantity() const`
  - Returns the new quantity.

- **ToOrderPointer**: `OrderPointer ToOrderPointer(OrderType type) const`
  - Transforms an existing order with `OrderModify` into a new order.

### Trade

- **Constructor**: `Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)`
  - Initializes a trade between two orders.

- **GetBidTrade**: `const TradeInfo& GetBidTrade() const`
  - Returns trade information for the bid side.

- **GetAskTrade**: `const TradeInfo& GetAskTrade() const`
  - Returns trade information for the ask side.

### Orderbook

- **AddOrder**: `Trades AddOrder(OrderPointer order)`
  - Adds an order to the order book and attempts to match it.

- **CancelOrder**: `void CancelOrder(OrderId orderid)`
  - Cancels an order in the order book.

- **MatchOrder**: `Trades MatchOrder(OrderModify order)`
  - Matches an order modification.

- **Size**: `std::size_t Size() const`
  - Returns the size of the order book.

- **GetOrderInfos**: `OrderbookLevelInfos GetOrderInfos() const`
  - Returns order book level information for both bids and asks.

## Main Function

- **main**: Initializes an `Orderbook`, adds an order, prints the size, cancels the order, and prints the size again.

## Usage

Compile and run the `main.cpp` file to see the order book system in action. The main function demonstrates adding and canceling an order. 