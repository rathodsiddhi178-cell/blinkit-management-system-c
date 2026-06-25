#include "blinkit.h"
#include <time.h>

void displayCustomer(Customer *c) {
    printf("\n");
    printf("Account ID : %-6d \n", c->account_id);
    printf(" Name       : %-30s \n", c->name);
    printf("Aadhar     : %-15s  \n", c->aadhar);
    printf("Phone      : %-15s \n", c->phone);
    printf("PinCode    : %-7s  \n", c->pincode);
    printf("Address    : %-30s |\n", c->address);
    printf("Balance    : Rs. %-10.2f \n", c->balance);
    printf("Total Spent: Rs. %-10.2f \n", c->total_spent);
    printf("Pending    : %-5d  Completed: %-5d \n",
           c->pending_order_count, c->completed_order_count);
    printf("\n");
}

void displayRider(Rider *r) {
    float last3 = r->monthly_income[0] + r->monthly_income[1] + r->monthly_income[2];
    printf("\n");
    printf(" Rider ID   : %-6d  \n", r->rider_id);
    printf(" Name       : %-30s \n", r->name);
    printf(" Phone      : %-15s \n", r->phone);
    printf(" PinCode    : %-7s \n", r->pincode);
    printf(" Dark Store : %-6d  Available: %-3s \n",
           r->dark_store_id, r->is_available ? "Yes" : "No");
    printf(" Orders/Month: %-5d \n", r->orders_this_month);
    printf(" Last 3M Earn: Rs. %-10.2f \n", last3);
    printf(" Total Income: Rs. %-10.2f \n", r->total_income);
    printf("\n");
}

void displayStore(DarkStore *s) {
    printf("\n");
    printf("Store ID : %-6d \n", s->store_id);
    printf("Name     : %-30s \n", s->name);
    printf("Location : %-30s \n", s->location);
    printf("PinCode  : %-7s \n", s->pincode);
    printf("--- Inventory ---\n");
    if (!s->inventory) printf("(empty)\n");
    else               invBTInorder(s->inventory);
    printf("\n");
}
 // register customers
void registerCustomer() {
    Customer c;
    memset(&c, 0, sizeof(Customer));
    c.completed_orders = c.pending_orders = NULL;
    c.account_id = next_customer_id++;

    printf("\n=== Register New Customer ===\n");
    printf("Enter Name       : "); scanf(" %[^\n]", c.name);
    printf("Enter Aadhar No  : "); scanf(" %s",     c.aadhar);
    printf("Enter Address    : "); scanf(" %[^\n]", c.address);
    printf("Enter PinCode    : "); scanf(" %s",     c.pincode);
    printf("Enter Phone      : "); scanf(" %s",     c.phone);
    printf("Enter Balance    : "); scanf("%f",      &c.balance);
    if (c.balance < 0) { printf("Balance cannot be negative!\n"); return; }

    distAddPincode(c.pincode);
    customerDB = custBTInsert(customerDB, c);
    markCustomerCacheDirty();
    printf("Customer registered! Account ID: %d\n", c.account_id);
}
 // update customers
void updateCustomer() {
    int id;
    printf("\n=== Update Customer ===\n");
    printf("Enter Account ID: "); scanf("%d", &id);
    Customer *c = custBTSearch(customerDB, id);
    if (!c) { printf("Customer not found!\n"); return; }

    printf("1.Name  2.Address  3.Phone  4.PinCode  5.Add Balance\nChoice: ");
    int ch; scanf("%d", &ch);
    switch (ch) {
        case 1: printf("New Name    : "); scanf(" %[^\n]", c->name);    break;
        case 2: printf("New Address : "); scanf(" %[^\n]", c->address); break;
        case 3: printf("New Phone   : "); scanf(" %s",     c->phone);   break;
        case 4:
            printf("New PinCode : "); scanf(" %s", c->pincode);
            distAddPincode(c->pincode);
            break;
        case 5: {
            float amt; printf("Amount to add: "); scanf("%f", &amt);
            if (amt <= 0) { printf("Invalid amount!\n"); return; }
            c->balance += amt;
            break;
        }
        default: printf("Invalid choice.\n"); return;
    }
    markCustomerCacheDirty();
    printf("Customer updated.\n");
}
 //delete customers
void deleteCustomer() {
    int id;
    printf("\n=== Delete Customer ===\n");
    printf("Enter Account ID: "); scanf("%d", &id);
    Customer *c = custBTSearch(customerDB, id);
    if (!c) { printf("Customer not found!\n"); return; }
    if (c->pending_order_count > 0) {
        printf("Cannot delete: %d pending orders exist!\n", c->pending_order_count);
        return;
    }
    customerDB = custBTDelete(customerDB, id);
    markCustomerCacheDirty();
    printf("Customer deleted.\n");
}
 // display customers by expenses descending

void displayCustomersByExpense() {
    printf("\n=== Customers by Total Expenses (Descending) ===\n");
    if (!customerDB) { printf("No customers.\n"); return; }

    if (custCache.byExpenseDirty) {
        rebuildCustExpenseCache();
        printf("[sorted]\n");
    } else {
        printf("[Using previous sorted data]\n");
    }

    for (int i = 0; i < custCache.byExpenseCount; i++)
        displayCustomer(custCache.byExpense[i]);
}

//  display customers by pincode ascending

void displayCustomersByPincode() {
    printf("\n=== Customers by PinCode ===\n");
    if (!customerDB) { printf("No customers.\n"); return; }

    if (custCache.byPincodeDirty) {
        rebuildCustPincodeCache();
        printf("[sorted]\n");
    } else {
        printf("[Using previous sorted data]\n");
    }

    for (int i = 0; i < custCache.byPincodeCount; i++)
        displayCustomer(custCache.byPincode[i]);
}
//   find nearest store by btree traversal
static DarkStore *findNearestStoreRec(StoreBTNode *root, const char *custPin, 
                                       int item_id, int qty, int *bestDist) {
    if (!root) return NULL;
    DarkStore *best = NULL;
    
    for (int i = 0; i < root->n; i++) {
        InventoryItem *inv = invBTSearch(root->keys[i].inventory, item_id);
        if (inv && inv->quantity >= qty) {
            int d = distGetDistance(custPin, root->keys[i].pincode);
            if (d < *bestDist) {
                *bestDist = d;
                best = &root->keys[i];
            }
        }
    }
    
    if (!root->leaf) {
        for (int i = 0; i <= root->n; i++) {
            DarkStore *childBest = findNearestStoreRec(root->child[i], custPin, item_id, qty, bestDist);
            if (childBest) best = childBest;
        }
    }
    return best;
}

static DarkStore *findNearestStore(const char *custPin, int item_id, int qty) {
    int bestDist = INF;
    return findNearestStoreRec(storeDB, custPin, item_id, qty, &bestDist);
}

//  find free rider by tree traversal
static Rider *findFreeRiderForStore(int store_id) {
    Rider *all[MAX_RECORDS];
    int cnt = 0;
    riderBTCollect(riderDB, all, &cnt);
    for (int i = 0; i < cnt; i++) {
        if (all[i]->dark_store_id == store_id && all[i]->is_available)
            return all[i];
    }
    return NULL;
}

static Rider *findFreeRiderNearby(const char *storePin) {
    Rider *all[MAX_RECORDS];
    int cnt = 0;
    riderBTCollect(riderDB, all, &cnt);
    Rider *best = NULL;
    int bestDist = INF;
    for (int i = 0; i < cnt; i++) {
        if (all[i]->is_available) {
            DarkStore *sn = storeBTSearch(storeDB, all[i]->dark_store_id);
            if (sn) {
                int d = distGetDistance(storePin, sn->pincode);
                if (d < bestDist) {
                    bestDist = d;
                    best = all[i];
                }
            }
        }
    }
    return best;
}
 // register rider
void registerRider() {
    Rider r;
    memset(&r, 0, sizeof(Rider));
    r.completed_orders = r.pending_orders = NULL;
    r.is_available = 1;
    r.rider_id = next_rider_id++;

    printf("\n=== Register Delivery Rider ===\n");
    printf("Enter Name   : "); scanf(" %[^\n]", r.name);
    printf("Enter Phone  : "); scanf(" %s",     r.phone);
    printf("Enter PinCode: "); scanf(" %s",     r.pincode);
    distAddPincode(r.pincode);

    DarkStore *allStores[MAX_RECORDS];
    int cnt = 0;
    storeBTCollect(storeDB, allStores, &cnt);
    DarkStore *nearest = NULL;
    int bestDist = INF;
    for (int i = 0; i < cnt; i++) {
        int d = distGetDistance(r.pincode, allStores[i]->pincode);
        if (d < bestDist) {
            bestDist = d;
            nearest = allStores[i];
        }
    }
    if (!nearest) {
        printf("No dark stores exist. Add a store first.\n");
        next_rider_id--;
        return;
    }
    r.dark_store_id = nearest->store_id;
    riderDB = riderBTInsert(riderDB, r);
    markRiderCacheDirty();
    printf("Rider registered! ID: %d -> Store: %d (%s)\n",
           r.rider_id, r.dark_store_id, nearest->name);
}

  // delete rider
void deleteRider() {
    int id;
    printf("\n=== Delete Rider ===\n");
    printf("Enter Rider ID: "); scanf("%d", &id);
    Rider *r = riderBTSearch(riderDB, id);
    if (!r) { printf("Rider not found!\n"); return; }
    if (!r->is_available) {
        printf("Cannot delete: rider is on a delivery!\n"); return;
    }
    riderDB = riderBTDelete(riderDB, id);
    markRiderCacheDirty();
    printf("Rider deleted.\n");
}

//   riders by 3 month earnings descending
void displayRidersByEarning3Months() {
    printf("\n=== Riders by Last 3 Months Earnings (Desc) ===\n");
    if (!riderDB) { printf("No riders.\n"); return; }

    if (riderCache.by3MonthDirty) {
        rebuildRider3MonthCache();
        printf("[sorted]\n");
    } else {
        printf("[Using previous sorted data]\n");
    }

    for (int i = 0; i < riderCache.by3MonthCount; i++)
        displayRider(riderCache.by3Month[i]);
}

 // riders by orders last month decending

void displayRidersByOrdersLastMonth() {
    printf("\n=== Riders by Orders This Month (Desc) ===\n");
    if (!riderDB) { printf("No riders.\n"); return; }

    if (riderCache.byOrdersDirty) {
        rebuildRiderOrdersCache();
        printf("[sorted]\n");
    } else {
        printf("[Using previous sorted data]\n");
    }

    for (int i = 0; i < riderCache.byOrdersCount; i++)
        displayRider(riderCache.byOrders[i]);
}

//  riders by pincode then earnings
void displayRidersByPincodeEarning() {
    printf("\n=== Riders by PinCode then 3-Month Earning ===\n");
    if (!riderDB) { printf("No riders.\n"); return; }

    if (riderCache.byPinEarningDirty) {
        rebuildRiderPinEarnCache();
        printf("[sorted]\n");
    } else {
        printf("[using previous sorted data]\n");
    }

    for (int i = 0; i < riderCache.byPinEarningCount; i++)
        displayRider(riderCache.byPinEarning[i]);
}

//  place order
void placeOrder() {
    printf("\n=== Place Order ===\n");
    if (!storeDB) { printf("No dark stores available.\n"); return; }

    int cust_id, item_id, qty;
    printf("Enter Customer Account ID : "); scanf("%d", &cust_id);
    Customer *c = custBTSearch(customerDB, cust_id);
    if (!c) { printf("Customer not found!\n"); return; }

    printf("Enter Item ID : "); scanf("%d", &item_id);
    printf("Enter Quantity: "); scanf("%d", &qty);
    if (qty <= 0) { printf("Invalid quantity!\n"); return; }

    DarkStore *sn = findNearestStore(c->pincode, item_id, qty);
    if (!sn) { printf("No store has enough stock!\n"); return; }

    InventoryItem *inv = invBTSearch(sn->inventory, item_id);
    float amount = inv->price * qty;

    if (c->balance < amount) {
        printf("Insufficient balance! Need Rs.%.2f, have Rs.%.2f\n",
               amount, c->balance);
        return;
    }
    c->balance -= amount;

    Rider *r = findFreeRiderForStore(sn->store_id);
    if (!r) {
        printf("No free rider at store. Searching nearby...\n");
        r = findFreeRiderNearby(sn->pincode);
    }
    if (!r) { printf("No available riders. Try later.\n"); return; }

    Order o;
    o.order_id      = next_order_id++;
    o.customer_id   = cust_id;
    o.rider_id      = r->rider_id;
    o.dark_store_id = sn->store_id;
    o.item_id       = item_id;
    o.quantity      = qty;
    o.amount        = amount;
    o.is_completed  = 0;
    o.order_time    = (long)time(NULL);
    strncpy(o.item_name, inv->item_name, MAX_ITEM_NAME - 1);
    o.item_name[MAX_ITEM_NAME - 1] = '\0';

    inv->quantity -= qty;
    if (inv->quantity == 0)
        sn->inventory = invBTDelete(sn->inventory, item_id);

    c->pending_orders = ordBTInsert(c->pending_orders, o);
    c->pending_order_count++;

    r->pending_orders = ordBTInsert(r->pending_orders, o);
    r->is_available   = 0;

    sn->pending_orders = ordBTInsert(sn->pending_orders, o);

    markCustomerCacheDirty();
    markRiderCacheDirty();

    printf("\nOrder placed!\n");
    printf("Order ID : %d | Item: %s x%d | Rs.%.2f\n",
           o.order_id, o.item_name, o.quantity, o.amount);
    printf("Rider    : %s (ID:%d) | Store: %s (ID:%d)\n",
           r->name, r->rider_id, sn->name, sn->store_id);
}

// deliver order
void deliverOrder() {
    printf("\n=== Deliver Order ===\n");
    int order_id;
    printf("Enter Order ID: "); scanf("%d", &order_id);

    Customer *allCust[MAX_RECORDS];
    int custCnt = 0;
    custBTCollect(customerDB, allCust, &custCnt);
    
    Customer *targetCust = NULL;
    Rider *targetRider = NULL;
    DarkStore *targetStore = NULL;
    Order targetOrder;
    int found = 0;

    for (int i = 0; i < custCnt && !found; i++) {
        Order *o = ordBTSearch(allCust[i]->pending_orders, order_id);
        if (o) {
            targetOrder = *o;
            targetCust = allCust[i];
            found = 1;
        }
    }
    if (!targetCust) { printf("Order not found!\n"); return; }

    targetRider = riderBTSearch(riderDB, targetOrder.rider_id);
    targetStore = storeBTSearch(storeDB, targetOrder.dark_store_id);
    if (!targetRider || !targetStore) { printf("Data inconsistency!\n"); return; }

    targetCust->total_spent += targetOrder.amount;
    targetOrder.is_completed = 1;
    targetCust->pending_orders   = ordBTDelete(targetCust->pending_orders, order_id);
    targetCust->completed_orders = ordBTInsert(targetCust->completed_orders, targetOrder);
    targetCust->pending_order_count--;
    targetCust->completed_order_count++;

    float commission = targetOrder.amount * 0.10f;
    targetRider->monthly_income[0] += commission;
    targetRider->total_income      += commission;
    targetRider->orders_this_month++;
    targetRider->is_available = 1;
    targetRider->pending_orders   = ordBTDelete(targetRider->pending_orders, order_id);
    targetRider->completed_orders = ordBTInsert(targetRider->completed_orders, targetOrder);

    targetStore->pending_orders   = ordBTDelete(targetStore->pending_orders, order_id);
    targetStore->completed_orders = ordBTInsert(targetStore->completed_orders, targetOrder);

    markCustomerCacheDirty();
    markRiderCacheDirty();

    printf("\nOrder #%d delivered!\n", order_id);
    printf("Amount     : Rs.%.2f\n", targetOrder.amount);
    printf("Commission : Rs.%.2f -> Rider %s\n", commission, targetRider->name);
}

// range search
void rangeSearchCustomers() {
    int b1, b2;
    printf("\n=== Range Search on Customers ===\n");
    printf("Enter B1 (start Account ID): "); scanf("%d", &b1);
    printf("Enter B2 (end   Account ID): "); scanf("%d", &b2);
    if (b1 > b2) { int t = b1; b1 = b2; b2 = t; }
    printf("\nCustomers with IDs %d to %d:\n", b1, b2);
    custBTRangeSearch(customerDB, b1, b2);
}

//  dark store management
 
void addDarkStore() {
    DarkStore s;
    memset(&s, 0, sizeof(DarkStore));
    s.inventory = NULL; s.pending_orders = NULL;
    s.completed_orders = NULL; s.riders = NULL;
    s.store_id = next_store_id++;

    printf("\n=== Add Dark Store ===\n");
    printf("Enter Name     : "); scanf(" %[^\n]", s.name);
    printf("Enter Location : "); scanf(" %[^\n]", s.location);
    printf("Enter PinCode  : "); scanf(" %s",     s.pincode);

    DarkStore *ex = storeBTSearchByPin(storeDB, s.pincode);
    if (ex) {
        printf("A store already exists at PinCode %s!\n", s.pincode);
        next_store_id--;
        return;
    }
    distAddPincode(s.pincode);
    storeDB = storeBTInsert(storeDB, s);
    printf("Dark Store added! ID: %d\n", s.store_id);
}

void displayDarkStores() {
    printf("\n=== All Dark Stores ===\n");
    if (!storeDB) { printf("No stores.\n"); return; }
    storeBTInorder(storeDB);
}

void addInventoryItem() {
    int store_id;
    printf("\n=== Add Inventory Item ===\n");
    printf("Enter Store ID: "); scanf("%d", &store_id);
    DarkStore *sn = storeBTSearch(storeDB, store_id);
    if (!sn) { printf("Store not found!\n"); return; }

    InventoryItem item;
    printf("Enter Item ID   : "); scanf("%d",      &item.item_id);
    printf("Enter Item Name : "); scanf(" %[^\n]",  item.item_name);
    printf("Enter Quantity  : "); scanf("%d",       &item.quantity);
    printf("Enter Price(Rs) : "); scanf("%f",       &item.price);

    if (item.quantity <= 0) { printf("Invalid quantity!\n"); return; }
    if (item.price    <= 0) { printf("Invalid price!\n");    return; }

    InventoryItem *ex = invBTSearch(sn->inventory, item.item_id);
    if (ex) {
        ex->quantity += item.quantity;
        ex->price     = item.price;
        printf("Existing item updated in store %d.\n", store_id);
    } else {
        sn->inventory = invBTInsert(sn->inventory, item);
        printf("Item added to store %d.\n", store_id);
    }
}

void addDistance() {
    char pin1[MAX_PINCODE], pin2[MAX_PINCODE];
    int  dist;
    printf("\n=== Add Distance Between PinCodes ===\n");
    printf("PinCode 1 : "); scanf(" %s", pin1);
    printf("PinCode 2 : "); scanf(" %s", pin2);
    printf("Distance  : "); scanf("%d",  &dist);
    distSetDistance(pin1, pin2, dist);
    printf("Set: %s <-> %s = %d km\n", pin1, pin2, dist);
}