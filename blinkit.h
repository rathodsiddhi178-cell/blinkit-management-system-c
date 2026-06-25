#ifndef BLINKIT_H
#define BLINKIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME        100
#define MAX_ADDR        200
#define MAX_PHONE       15
#define MAX_AADHAR      13
#define MAX_PINCODE     7
#define MAX_ITEM_NAME   100
#define MAX_PINCODES    500
#define MAX_RECORDS     5000
#define INF             999999

#define BT_T   3                      /* minimum degree */
#define BT_MAX (2*BT_T - 1)           /* max keys per node  = 5  */
#define BT_MIN (BT_T   - 1)           /* min keys (non-root) = 2  */


typedef struct Order {
    int   order_id, customer_id, rider_id, dark_store_id, item_id, quantity, is_completed;
    char  item_name[MAX_ITEM_NAME];
    float amount;
    long  order_time;
} Order;

typedef struct InventoryItem {
    int   item_id, quantity;
    char  item_name[MAX_ITEM_NAME];
    float price;
} InventoryItem;

// Forward declarations for B-tree nodes 
typedef struct CustBTNode CustBTNode;
typedef struct RiderBTNode RiderBTNode;
typedef struct StoreBTNode StoreBTNode;
typedef struct InvBTNode InvBTNode;
typedef struct OrdBTNode OrdBTNode;

typedef struct Customer {
    int   account_id, completed_order_count, pending_order_count;
    char  name[MAX_NAME], aadhar[MAX_AADHAR], address[MAX_ADDR];
    char  pincode[MAX_PINCODE], phone[MAX_PHONE];
    float balance, total_spent;
    OrdBTNode *completed_orders, *pending_orders;
} Customer;

typedef struct Rider {
    int   rider_id, dark_store_id, is_available, orders_this_month;
    char  name[MAX_NAME], phone[MAX_PHONE], pincode[MAX_PINCODE];
    float monthly_income[12], total_income;
    OrdBTNode *completed_orders, *pending_orders;
} Rider;

typedef struct DarkStore {
    int  store_id;
    char name[MAX_NAME], location[MAX_ADDR], pincode[MAX_PINCODE];
    InvBTNode  *inventory;
    OrdBTNode  *pending_orders, *completed_orders;
    RiderBTNode *riders;
} DarkStore;



// Order B-tree
struct OrdBTNode {
    int   n;
    int   leaf;
    Order keys[BT_MAX];
    OrdBTNode *child[BT_MAX + 1];
};

// Inventory B-tree 
struct InvBTNode {
    int           n;
    int           leaf;
    InventoryItem keys[BT_MAX];
    InvBTNode *child[BT_MAX + 1];
};

//Customer B-tree
struct CustBTNode {
    int      n;
    int      leaf;
    Customer keys[BT_MAX];
    CustBTNode *child[BT_MAX + 1];
};

// Rider B-tree
struct RiderBTNode {
    int   n;
    int   leaf;
    Rider keys[BT_MAX];
    RiderBTNode *child[BT_MAX + 1];
};

//43DarkStore B-tree 
struct StoreBTNode {
    int       n;
    int       leaf;
    DarkStore keys[BT_MAX];
    StoreBTNode *child[BT_MAX + 1];
};

//distance table
typedef struct {
    char pincodes[MAX_PINCODES][MAX_PINCODE];
    int  dist[MAX_PINCODES][MAX_PINCODES];
    int  count;
} DistanceTable;

//dirty flag caches
typedef struct {
    Customer *byExpense[MAX_RECORDS];
    int       byExpenseCount;
    int       byExpenseDirty;
    Customer *byPincode[MAX_RECORDS];
    int       byPincodeCount;
    int       byPincodeDirty;
} CustomerCache;

typedef struct {
    Rider *by3Month[MAX_RECORDS];
    int    by3MonthCount;
    int    by3MonthDirty;
    Rider *byOrders[MAX_RECORDS];
    int    byOrdersCount;
    int    byOrdersDirty;
    Rider *byPinEarning[MAX_RECORDS];
    int    byPinEarningCount;
    int    byPinEarningDirty;
} RiderCache;
//global fuunctns
extern CustBTNode  *customerDB;
extern RiderBTNode *riderDB;
extern StoreBTNode *storeDB;
extern DistanceTable distTable;
extern CustomerCache custCache;
extern RiderCache    riderCache;
extern int next_order_id, next_customer_id, next_rider_id, next_store_id;

//dirty flag
void markCustomerCacheDirty(void);
void markRiderCacheDirty(void);

//generic functns
int maxInt(int a, int b);

//customer b tree key=customer id
CustBTNode *custBTNewNode(int leaf);
void        custBTSplitChild(CustBTNode *x, int i);
void        custBTInsertNonFull(CustBTNode *x, Customer c);
CustBTNode *custBTInsert(CustBTNode *root, Customer c);
Customer   *custBTSearch(CustBTNode *root, int id);
CustBTNode *custBTDelete(CustBTNode *root, int id);
void        custBTInorder(CustBTNode *root);
void        custBTRangeSearch(CustBTNode *root, int b1, int b2);
int         custBTCount(CustBTNode *root);
void        custBTCollect(CustBTNode *root, Customer **arr, int *cnt);

//rider btree key=rider id
RiderBTNode *riderBTNewNode(int leaf);
void         riderBTSplitChild(RiderBTNode *x, int i);
void         riderBTInsertNonFull(RiderBTNode *x, Rider r);
RiderBTNode *riderBTInsert(RiderBTNode *root, Rider r);
Rider       *riderBTSearch(RiderBTNode *root, int id);
RiderBTNode *riderBTDelete(RiderBTNode *root, int id);
void         riderBTInorder(RiderBTNode *root);
int          riderBTCount(RiderBTNode *root);
void         riderBTCollect(RiderBTNode *root, Rider **arr, int *cnt);

//store b tree key=store id
StoreBTNode *storeBTNewNode(int leaf);
void         storeBTSplitChild(StoreBTNode *x, int i);
void         storeBTInsertNonFull(StoreBTNode *x, DarkStore s);
StoreBTNode *storeBTInsert(StoreBTNode *root, DarkStore s);
DarkStore   *storeBTSearch(StoreBTNode *root, int id);
DarkStore   *storeBTSearchByPin(StoreBTNode *root, const char *pin);
void         storeBTInorder(StoreBTNode *root);
int          storeBTCount(StoreBTNode *root);
void         storeBTCollect(StoreBTNode *root, DarkStore **arr, int *cnt);

//inventory b tree
InvBTNode    *invBTNewNode(int leaf);
void          invBTSplitChild(InvBTNode *x, int i);
void          invBTInsertNonFull(InvBTNode *x, InventoryItem item);
InvBTNode    *invBTInsert(InvBTNode *root, InventoryItem item);
InventoryItem *invBTSearch(InvBTNode *root, int id);
InvBTNode    *invBTDelete(InvBTNode *root, int id);
void          invBTInorder(InvBTNode *root);
int           invBTCount(InvBTNode *root);
void          invBTCollect(InvBTNode *root, InventoryItem **arr, int *cnt);

//order b tree key=ord id
OrdBTNode *ordBTNewNode(int leaf);
void       ordBTSplitChild(OrdBTNode *x, int i);
void       ordBTInsertNonFull(OrdBTNode *x, Order o);
OrdBTNode *ordBTInsert(OrdBTNode *root, Order o);
Order     *ordBTSearch(OrdBTNode *root, int id);
OrdBTNode *ordBTDelete(OrdBTNode *root, int id);
void       ordBTInorder(OrdBTNode *root);
int        ordBTCount(OrdBTNode *root);
void       ordBTCollect(OrdBTNode *root, Order **arr, int *cnt);


void rebuildCustExpenseCache(void);
void rebuildCustPincodeCache(void);
void rebuildRider3MonthCache(void);
void rebuildRiderOrdersCache(void);
void rebuildRiderPinEarnCache(void);

// Merge sort functions (sort.c)
void mergeSortCustExpense(Customer **arr, int l, int r);
void mergeSortCustPincode(Customer **arr, int l, int r);
void mergeSortRider3Month(Rider **arr, int l, int r);
void mergeSortRiderOrders(Rider **arr, int l, int r);
void mergeSortRiderPinEarning(Rider **arr, int l, int r);
void collectCustomers(CustBTNode *root, Customer **arr, int *cnt);
void collectRiders(RiderBTNode *root, Rider **arr, int *cnt);

//distance table
void distInit(void);
int  distGetIndex(const char *pin);
int  distAddPincode(const char *pin);
void distSetDistance(const char *pin1, const char *pin2, int d);
int  distGetDistance(const char *pin1, const char *pin2);
void distSaveToFile(const char *f);
void distLoadFromFile(const char *f);

//operations
void registerCustomer(void);
void updateCustomer(void);
void deleteCustomer(void);
void displayCustomersByExpense(void);
void displayCustomersByPincode(void);

void registerRider(void);
void deleteRider(void);
void displayRidersByEarning3Months(void);
void displayRidersByOrdersLastMonth(void);
void displayRidersByPincodeEarning(void);

void placeOrder(void);
void deliverOrder(void);
void rangeSearchCustomers(void);

void addDarkStore(void);
void displayDarkStores(void);
void addInventoryItem(void);
void addDistance(void);

void saveAllData(void);
void loadAllData(void);
void displayCustomer(Customer  *c);
void displayRider   (Rider     *r);
void displayStore   (DarkStore *s);

#endif 