# Blinkit Management System

## Overview

A Blinkit-inspired quick-commerce backend developed in C.

The system manages customers, delivery riders, dark stores, inventory, and orders using B-Tree based indexing. It supports efficient searching, sorting, order processing, rider assignment, inventory tracking, caching, and persistent storage.

---

## Features

### Customer Management
- Customer registration and updates
- Balance management
- Expense tracking
- Customer search and range queries

### Rider Management
- Rider registration and deletion
- Earnings tracking
- Order delivery statistics
- Availability management

### Order Management
- Order placement and delivery
- Pending and completed order tracking
- Customer and rider order history

### Dark Store & Inventory Management
- Dark store creation and management
- Inventory tracking and updates
- Store-wise item management

### Delivery Routing
- Pincode-based distance table
- Nearest store selection
- Rider assignment for deliveries

### Persistent Storage
- Save and load all records from files
- Automatic reconstruction of B-Tree structures

---

## Data Structures & Algorithms

### B-Trees
Used for storing:

- Customers
- Riders
- Orders
- Inventory Items
- Dark Stores

Supported Operations:

- Search
- Insert
- Delete
- Traversal
- Range Queries

### Caching System
- Dirty-flag based cache
- Avoids repeated sorting operations
- Rebuilds only when data changes

### Sorting
- Merge Sort based ranking and reporting
- Customer expense analytics
- Rider earnings analytics

---

## Key Highlights

- B-Tree based indexing for efficient data retrieval
- Cache optimization using dirty flags
- Distance-based delivery assignment
- Inventory and order management
- File-based persistence layer
- Modular C project design

---

## Technologies Used

- C Programming
- Data Structures
- B-Trees
- Merge Sort
- File Handling
- Dynamic Memory Management

---

## Learning Outcomes

- Database indexing concepts
- Backend system design
- Efficient search and sorting techniques
- Persistent storage implementation
- Inventory and order processing workflows

---

## Build

```bash
make
```

Run:

```bash
./blinkit
```

---

## Author

Siddhi Rathod
