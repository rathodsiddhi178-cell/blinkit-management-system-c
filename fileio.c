#include "blinkit.h"
#include <time.h>
  //  write all orders in a tree to file
static void writeOrdersToTxt(OrdBTNode *root, FILE *fp) {
    if (!root) return;
    for (int i = 0; i < root->n; i++) {
        if (!root->leaf) writeOrdersToTxt(root->child[i], fp);
        fprintf(fp, "ORDER %d %d %d %d %d %d %.2f %d %ld %s\n",
                root->keys[i].order_id,
                root->keys[i].customer_id,
                root->keys[i].rider_id,
                root->keys[i].dark_store_id,
                root->keys[i].item_id,
                root->keys[i].quantity,
                root->keys[i].amount,
                root->keys[i].is_completed,
                root->keys[i].order_time,
                root->keys[i].item_name);
    }
    if (!root->leaf) writeOrdersToTxt(root->child[root->n], fp);
}
 //  read one order line from file
static Order readOrderFromLine(char *line) {
    Order o;
    memset(&o, 0, sizeof(Order));
    int consumed = 0;
    sscanf(line, "ORDER %d %d %d %d %d %d %f %d %ld%n",
           &o.order_id,
           &o.customer_id,
           &o.rider_id,
           &o.dark_store_id,
           &o.item_id,
           &o.quantity,
           &o.amount,
           &o.is_completed,
           &o.order_time,
           &consumed);
    if (consumed > 0 && line[consumed] != '\0') {
        char *nameStart = line + consumed;
        while (*nameStart == ' ') nameStart++;
        strncpy(o.item_name, nameStart, MAX_ITEM_NAME - 1);
        o.item_name[MAX_ITEM_NAME - 1] = '\0';
        int len = strlen(o.item_name);
        while (len > 0 && (o.item_name[len-1] == '\n' || o.item_name[len-1] == '\r'))
            o.item_name[--len] = '\0';
    }
    return o;
}
 //  save customers to customers.txt
static void saveCustomerNode(CustBTNode *root, FILE *fp) {
    if (!root) return;
    for (int i = 0; i < root->n; i++) {
        if (!root->leaf) saveCustomerNode(root->child[i], fp);
        Customer *c = &root->keys[i];
        fprintf(fp, "---CUSTOMER---\n");
        fprintf(fp, "ACCOUNT_ID %d\n", c->account_id);
        fprintf(fp, "NAME %s\n", c->name);
        fprintf(fp, "AADHAR %s\n", c->aadhar);
        fprintf(fp, "ADDRESS %s\n", c->address);
        fprintf(fp, "PINCODE %s\n", c->pincode);
        fprintf(fp, "PHONE %s\n", c->phone);
        fprintf(fp, "BALANCE %.2f\n", c->balance);
        fprintf(fp, "TOTAL_SPENT %.2f\n", c->total_spent);
        fprintf(fp, "COMPLETED_ORDERS %d\n", c->completed_order_count);
        fprintf(fp, "PENDING_ORDERS %d\n", c->pending_order_count);
        fprintf(fp, "---COMPLETED---\n");
        writeOrdersToTxt(c->completed_orders, fp);
        fprintf(fp, "---PENDING---\n");
        writeOrdersToTxt(c->pending_orders, fp);
        fprintf(fp, "---END_CUSTOMER---\n\n");
    }
    if (!root->leaf) saveCustomerNode(root->child[root->n], fp);
}

void saveCustomersToFile() {
    FILE *fp = fopen("customers.txt", "w");
    if (!fp) { printf("Cannot open customers.txt for writing!\n"); return; }
    fprintf(fp, "# Blinkit Customer Database\n");
    fprintf(fp, "TOTAL %d\n", custBTCount(customerDB));
    fprintf(fp, "NEXT_ID %d\n\n", next_customer_id);
    saveCustomerNode(customerDB, fp);
    fclose(fp);
}

void loadCustomersFromFile() {
    FILE *fp = fopen("customers.txt", "r");
    if (!fp) return;

    char line[512];
    Customer c;
    int inCustomer = 0, readingCompleted = 0, readingPending = 0;
    memset(&c, 0, sizeof(Customer));

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "NEXT_ID", 7) == 0) {
            sscanf(line, "NEXT_ID %d", &next_customer_id);
        }
        else if (strcmp(line, "---CUSTOMER---") == 0) {
            memset(&c, 0, sizeof(Customer));
            c.completed_orders = c.pending_orders = NULL;
            inCustomer = 1; readingCompleted = 0; readingPending = 0;
        }
        else if (strcmp(line, "---COMPLETED---") == 0) {
            readingCompleted = 1; readingPending = 0;
        }
        else if (strcmp(line, "---PENDING---") == 0) {
            readingCompleted = 0; readingPending = 1;
        }
        else if (strcmp(line, "---END_CUSTOMER---") == 0) {
            if (inCustomer) {
                customerDB = custBTInsert(customerDB, c);
                distAddPincode(c.pincode);
            }
            inCustomer = 0;
        }
        else if (inCustomer) {
            if (strncmp(line, "ORDER ", 6) == 0) {
                Order o = readOrderFromLine(line);
                if (readingCompleted)
                    c.completed_orders = ordBTInsert(c.completed_orders, o);
                else if (readingPending)
                    c.pending_orders = ordBTInsert(c.pending_orders, o);
            }
            else if (strncmp(line, "ACCOUNT_ID", 10) == 0) sscanf(line, "ACCOUNT_ID %d", &c.account_id);
            else if (strncmp(line, "NAME", 4) == 0) sscanf(line, "NAME %[^\n]", c.name);
            else if (strncmp(line, "AADHAR", 6) == 0) sscanf(line, "AADHAR %s", c.aadhar);
            else if (strncmp(line, "ADDRESS", 7) == 0) sscanf(line, "ADDRESS %[^\n]", c.address);
            else if (strncmp(line, "PINCODE", 7) == 0) sscanf(line, "PINCODE %s", c.pincode);
            else if (strncmp(line, "PHONE", 5) == 0) sscanf(line, "PHONE %s", c.phone);
            else if (strncmp(line, "BALANCE", 7) == 0) sscanf(line, "BALANCE %f", &c.balance);
            else if (strncmp(line, "TOTAL_SPENT", 11) == 0) sscanf(line, "TOTAL_SPENT %f", &c.total_spent);
            else if (strncmp(line, "COMPLETED_ORDERS", 16) == 0) sscanf(line, "COMPLETED_ORDERS %d", &c.completed_order_count);
            else if (strncmp(line, "PENDING_ORDERS", 14) == 0) sscanf(line, "PENDING_ORDERS %d", &c.pending_order_count);
        }
    }
    fclose(fp);
}
 //  save riders to riders.txt
static void saveRiderNode(RiderBTNode *root, FILE *fp) {
    if (!root) return;
    for (int i = 0; i < root->n; i++) {
        if (!root->leaf) saveRiderNode(root->child[i], fp);
        Rider *r = &root->keys[i];
        fprintf(fp, "---RIDER---\n");
        fprintf(fp, "RIDER_ID %d\n", r->rider_id);
        fprintf(fp, "NAME %s\n", r->name);
        fprintf(fp, "PHONE %s\n", r->phone);
        fprintf(fp, "PINCODE %s\n", r->pincode);
        fprintf(fp, "DARK_STORE_ID %d\n", r->dark_store_id);
        fprintf(fp, "IS_AVAILABLE %d\n", r->is_available);
        fprintf(fp, "ORDERS_THIS_MONTH %d\n", r->orders_this_month);
        fprintf(fp, "TOTAL_INCOME %.2f\n", r->total_income);
        fprintf(fp, "MONTHLY_INCOME");
        for (int j = 0; j < 12; j++) fprintf(fp, " %.2f", r->monthly_income[j]);
        fprintf(fp, "\n");
        fprintf(fp, "---COMPLETED---\n");
        writeOrdersToTxt(r->completed_orders, fp);
        fprintf(fp, "---PENDING---\n");
        writeOrdersToTxt(r->pending_orders, fp);
        fprintf(fp, "---END_RIDER---\n\n");
    }
    if (!root->leaf) saveRiderNode(root->child[root->n], fp);
}

void saveRidersToFile() {
    FILE *fp = fopen("riders.txt", "w");
    if (!fp) { printf("Cannot open riders.txt!\n"); return; }
    fprintf(fp, "# Blinkit Riders Database\n");
    fprintf(fp, "TOTAL %d\n", riderBTCount(riderDB));
    fprintf(fp, "NEXT_ID %d\n\n", next_rider_id);
    saveRiderNode(riderDB, fp);
    fclose(fp);
}

void loadRidersFromFile() {
    FILE *fp = fopen("riders.txt", "r");
    if (!fp) return;

    char line[512];
    Rider r;
    int inRider = 0, readingCompleted = 0, readingPending = 0;
    memset(&r, 0, sizeof(Rider));

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "NEXT_ID", 7) == 0) {
            sscanf(line, "NEXT_ID %d", &next_rider_id);
        }
        else if (strcmp(line, "---RIDER---") == 0) {
            memset(&r, 0, sizeof(Rider));
            r.completed_orders = r.pending_orders = NULL;
            inRider = 1; readingCompleted = 0; readingPending = 0;
        }
        else if (strcmp(line, "---COMPLETED---") == 0) {
            readingCompleted = 1; readingPending = 0;
        }
        else if (strcmp(line, "---PENDING---") == 0) {
            readingCompleted = 0; readingPending = 1;
        }
        else if (strcmp(line, "---END_RIDER---") == 0) {
            if (inRider) riderDB = riderBTInsert(riderDB, r);
            inRider = 0;
        }
        else if (inRider) {
            if (strncmp(line, "ORDER ", 6) == 0) {
                Order o = readOrderFromLine(line);
                if (readingCompleted)
                    r.completed_orders = ordBTInsert(r.completed_orders, o);
                else if (readingPending)
                    r.pending_orders = ordBTInsert(r.pending_orders, o);
            }
            else if (strncmp(line, "RIDER_ID", 8) == 0) sscanf(line, "RIDER_ID %d", &r.rider_id);
            else if (strncmp(line, "NAME", 4) == 0) sscanf(line, "NAME %[^\n]", r.name);
            else if (strncmp(line, "PHONE", 5) == 0) sscanf(line, "PHONE %s", r.phone);
            else if (strncmp(line, "PINCODE", 7) == 0) sscanf(line, "PINCODE %s", r.pincode);
            else if (strncmp(line, "DARK_STORE_ID", 13) == 0) sscanf(line, "DARK_STORE_ID %d", &r.dark_store_id);
            else if (strncmp(line, "IS_AVAILABLE", 12) == 0) sscanf(line, "IS_AVAILABLE %d", &r.is_available);
            else if (strncmp(line, "ORDERS_THIS_MONTH", 17) == 0) sscanf(line, "ORDERS_THIS_MONTH %d", &r.orders_this_month);
            else if (strncmp(line, "TOTAL_INCOME", 12) == 0) sscanf(line, "TOTAL_INCOME %f", &r.total_income);
            else if (strncmp(line, "MONTHLY_INCOME", 14) == 0) {
                sscanf(line, "MONTHLY_INCOME %f %f %f %f %f %f %f %f %f %f %f %f",
                       &r.monthly_income[0], &r.monthly_income[1], &r.monthly_income[2], &r.monthly_income[3],
                       &r.monthly_income[4], &r.monthly_income[5], &r.monthly_income[6], &r.monthly_income[7],
                       &r.monthly_income[8], &r.monthly_income[9], &r.monthly_income[10], &r.monthly_income[11]);
            }
        }
    }
    fclose(fp);
}
  // save inventory
static void saveInvToTxt(InvBTNode *root, FILE *fp) {
    if (!root) return;
    for (int i = 0; i < root->n; i++) {
        if (!root->leaf) saveInvToTxt(root->child[i], fp);
        fprintf(fp, "ITEM %d %d %.2f %s\n",
                root->keys[i].item_id,
                root->keys[i].quantity,
                root->keys[i].price,
                root->keys[i].item_name);
    }
    if (!root->leaf) saveInvToTxt(root->child[root->n], fp);
}
 //  save stores to stores.txt

static void saveStoreNode(StoreBTNode *root, FILE *fp) {
    if (!root) return;
    for (int i = 0; i < root->n; i++) {
        if (!root->leaf) saveStoreNode(root->child[i], fp);
        DarkStore *s = &root->keys[i];
        fprintf(fp, "---STORE---\n");
        fprintf(fp, "STORE_ID %d\n", s->store_id);
        fprintf(fp, "NAME %s\n", s->name);
        fprintf(fp, "LOCATION %s\n", s->location);
        fprintf(fp, "PINCODE %s\n", s->pincode);
        fprintf(fp, "INVENTORY_COUNT %d\n", invBTCount(s->inventory));
        fprintf(fp, "---INVENTORY---\n");
        saveInvToTxt(s->inventory, fp);
        fprintf(fp, "---COMPLETED---\n");
        writeOrdersToTxt(s->completed_orders, fp);
        fprintf(fp, "---PENDING---\n");
        writeOrdersToTxt(s->pending_orders, fp);
        fprintf(fp, "---END_STORE---\n\n");
    }
    if (!root->leaf) saveStoreNode(root->child[root->n], fp);
}

void saveStoresToFile() {
    FILE *fp = fopen("stores.txt", "w");
    if (!fp) { printf("Cannot open stores.txt!\n"); return; }
    fprintf(fp, "# Blinkit Dark Stores Database\n");
    fprintf(fp, "TOTAL %d\n", storeBTCount(storeDB));
    fprintf(fp, "NEXT_ID %d\n\n", next_store_id);
    saveStoreNode(storeDB, fp);
    fclose(fp);
}

void loadStoresFromFile() {
    FILE *fp = fopen("stores.txt", "r");
    if (!fp) return;

    char line[512];
    DarkStore s;
    int inStore = 0, readingInv = 0, readingCompleted = 0, readingPending = 0;
    memset(&s, 0, sizeof(DarkStore));

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "NEXT_ID", 7) == 0) {
            sscanf(line, "NEXT_ID %d", &next_store_id);
        }
        else if (strcmp(line, "---STORE---") == 0) {
            memset(&s, 0, sizeof(DarkStore));
            s.inventory = NULL;
            s.pending_orders = NULL;
            s.completed_orders = NULL;
            s.riders = NULL;
            inStore = 1;
            readingInv = readingCompleted = readingPending = 0;
        }
        else if (strcmp(line, "---INVENTORY---") == 0) {
            readingInv = 1; readingCompleted = readingPending = 0;
        }
        else if (strcmp(line, "---COMPLETED---") == 0) {
            readingInv = 0; readingCompleted = 1; readingPending = 0;
        }
        else if (strcmp(line, "---PENDING---") == 0) {
            readingInv = 0; readingCompleted = 0; readingPending = 1;
        }
        else if (strcmp(line, "---END_STORE---") == 0) {
            if (inStore) {
                storeDB = storeBTInsert(storeDB, s);
                distAddPincode(s.pincode);
            }
            inStore = 0;
        }
        else if (inStore) {
            if (readingInv && strncmp(line, "ITEM ", 5) == 0) {
                InventoryItem item;
                memset(&item, 0, sizeof(InventoryItem));
                int consumed = 0;
                sscanf(line, "ITEM %d %d %f%n", &item.item_id, &item.quantity, &item.price, &consumed);
                if (consumed > 0 && line[consumed] != '\0') {
                    char *nameStart = line + consumed;
                    while (*nameStart == ' ') nameStart++;
                    strncpy(item.item_name, nameStart, MAX_ITEM_NAME - 1);
                    item.item_name[MAX_ITEM_NAME - 1] = '\0';
                    int len = strlen(item.item_name);
                    while (len > 0 && (item.item_name[len-1] == '\n' || item.item_name[len-1] == '\r'))
                        item.item_name[--len] = '\0';
                }
                s.inventory = invBTInsert(s.inventory, item);
            }
            else if (readingCompleted && strncmp(line, "ORDER ", 6) == 0) {
                Order o = readOrderFromLine(line);
                s.completed_orders = ordBTInsert(s.completed_orders, o);
            }
            else if (readingPending && strncmp(line, "ORDER ", 6) == 0) {
                Order o = readOrderFromLine(line);
                s.pending_orders = ordBTInsert(s.pending_orders, o);
            }
            else if (strncmp(line, "STORE_ID", 8) == 0) sscanf(line, "STORE_ID %d", &s.store_id);
            else if (strncmp(line, "NAME", 4) == 0) sscanf(line, "NAME %[^\n]", s.name);
            else if (strncmp(line, "LOCATION", 8) == 0) sscanf(line, "LOCATION %[^\n]", s.location);
            else if (strncmp(line, "PINCODE", 7) == 0) sscanf(line, "PINCODE %s", s.pincode);
        }
    }
    fclose(fp);
}
 //  sistance table to distances.txt
   
void distSaveToFile(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { printf("Cannot open %s!\n", filename); return; }
    fprintf(fp, "# Blinkit Distance Table\n");
    fprintf(fp, "COUNT %d\n\n", distTable.count);
    for (int i = 0; i < distTable.count; i++) {
        for (int j = i + 1; j < distTable.count; j++) {
            if (distTable.dist[i][j] != INF) {
                fprintf(fp, "DIST %s %s %d\n",
                        distTable.pincodes[i],
                        distTable.pincodes[j],
                        distTable.dist[i][j]);
            }
        }
    }
    fclose(fp);
}

void distLoadFromFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;
        if (strncmp(line, "DIST ", 5) == 0) {
            char p1[MAX_PINCODE], p2[MAX_PINCODE];
            int d;
            sscanf(line, "DIST %s %s %d", p1, p2, &d);
            distSetDistance(p1, p2, d);
        }
    }
    fclose(fp);
}
//   distance table in-memory operations

void distInit() {
    distTable.count = 0;
    for (int i = 0; i < MAX_PINCODES; i++)
        for (int j = 0; j < MAX_PINCODES; j++)
            distTable.dist[i][j] = (i == j) ? 0 : INF;
}

int distGetIndex(const char *pin) {
    for (int i = 0; i < distTable.count; i++)
        if (strcmp(distTable.pincodes[i], pin) == 0) return i;
    return -1;
}

int distAddPincode(const char *pin) {
    int idx = distGetIndex(pin);
    if (idx != -1) return idx;
    if (distTable.count >= MAX_PINCODES) {
        printf("Distance table full!\n");
        return -1;
    }
    idx = distTable.count++;
    strncpy(distTable.pincodes[idx], pin, MAX_PINCODE - 1);
    for (int i = 0; i < distTable.count; i++) {
        distTable.dist[idx][i] = INF;
        distTable.dist[i][idx] = INF;
    }
    distTable.dist[idx][idx] = 0;
    return idx;
}

void distSetDistance(const char *pin1, const char *pin2, int d) {
    int i = distAddPincode(pin1);
    int j = distAddPincode(pin2);
    if (i < 0 || j < 0) return;
    distTable.dist[i][j] = d;
    distTable.dist[j][i] = d;
}

int distGetDistance(const char *pin1, const char *pin2) {
    int i = distGetIndex(pin1);
    int j = distGetIndex(pin2);
    if (i < 0 || j < 0) return INF;
    return distTable.dist[i][j];
}
// save/ load all

void saveAllData() {
    saveCustomersToFile();
    saveRidersToFile();
    saveStoresToFile();
    distSaveToFile("distances.txt");

    FILE *fp = fopen("counters.txt", "w");
    if (fp) {
        fprintf(fp, "# Blinkit ID Counters\n");
        fprintf(fp, "NEXT_ORDER_ID %d\n", next_order_id);
        fprintf(fp, "NEXT_CUSTOMER_ID %d\n", next_customer_id);
        fprintf(fp, "NEXT_RIDER_ID %d\n", next_rider_id);
        fprintf(fp, "NEXT_STORE_ID %d\n", next_store_id);
        fclose(fp);
    }
    printf("All data saved to text files.\n");
}

void loadAllData() {
    distInit();
    distLoadFromFile("distances.txt");
    loadCustomersFromFile();
    loadRidersFromFile();
    loadStoresFromFile();

    FILE *fp = fopen("counters.txt", "r");
    if (fp) {
        char line[128];
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == '#') continue;
            if (strncmp(line, "NEXT_ORDER_ID", 13) == 0) sscanf(line, "NEXT_ORDER_ID %d", &next_order_id);
            else if (strncmp(line, "NEXT_CUSTOMER_ID", 16) == 0) sscanf(line, "NEXT_CUSTOMER_ID %d", &next_customer_id);
            else if (strncmp(line, "NEXT_RIDER_ID", 13) == 0) sscanf(line, "NEXT_RIDER_ID %d", &next_rider_id);
            else if (strncmp(line, "NEXT_STORE_ID", 13) == 0) sscanf(line, "NEXT_STORE_ID %d", &next_store_id);
        }
        fclose(fp);
    }
    printf("Data loaded from text files.\n");
    markCustomerCacheDirty();
    markRiderCacheDirty();
}