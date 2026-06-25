#include "blinkit.h"

CustBTNode  *customerDB = NULL;
RiderBTNode *riderDB    = NULL;
StoreBTNode *storeDB    = NULL;
DistanceTable distTable;
int next_order_id    = 1;
int next_customer_id = 1001;
int next_rider_id    = 2001;
int next_store_id    = 3001;

int maxInt(int a, int b) { return (a > b) ? a : b; }

CustomerCache custCache = {0};
RiderCache    riderCache = {0};

void markCustomerCacheDirty(void) {
    custCache.byExpenseDirty  = 1;
    custCache.byPincodeDirty  = 1;
}
void markRiderCacheDirty(void) {
    riderCache.by3MonthDirty    = 1;
    riderCache.byOrdersDirty    = 1;
    riderCache.byPinEarningDirty = 1;
}

//CUSTOMER B-TREE  key = account_id
CustBTNode *custBTNewNode(int leaf) {
    CustBTNode *n = (CustBTNode *)calloc(1, sizeof(CustBTNode));
    n->leaf = leaf;
    n->n    = 0;
    return n;
}

void custBTSplitChild(CustBTNode *x, int i) {
    CustBTNode *y = x->child[i];
    CustBTNode *z = custBTNewNode(y->leaf);
    z->n = BT_MIN;

    for (int j = 0; j < BT_MIN; j++)
        z->keys[j] = y->keys[j + BT_T];

    if (!y->leaf)
        for (int j = 0; j <= BT_MIN; j++)
            z->child[j] = y->child[j + BT_T];

    y->n = BT_MIN;

    for (int j = x->n; j >= i + 1; j--)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;

    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[BT_T - 1];
    x->n++;
}

void custBTInsertNonFull(CustBTNode *x, Customer c) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && c.account_id < x->keys[i].account_id) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        if (i >= 0 && c.account_id == x->keys[i].account_id) {
            x->keys[i] = c;
            return;
        }
        x->keys[i + 1] = c;
        x->n++;
    } else {
        while (i >= 0 && c.account_id < x->keys[i].account_id) i--;
        if (i >= 0 && c.account_id == x->keys[i].account_id) {
            x->keys[i] = c;
            return;
        }
        i++;
        if (x->child[i]->n == BT_MAX) {
            custBTSplitChild(x, i);
            if (c.account_id > x->keys[i].account_id) i++;
            if (i > 0 && c.account_id == x->keys[i - 1].account_id) {
                x->keys[i - 1] = c;
                return;
            }
        }
        custBTInsertNonFull(x->child[i], c);
    }
}

CustBTNode *custBTInsert(CustBTNode *root, Customer c) {
    if (!root) {
        root = custBTNewNode(1);
        root->keys[0] = c;
        root->n = 1;
        return root;
    }
    if (root->n == BT_MAX) {
        CustBTNode *s = custBTNewNode(0);
        s->child[0] = root;
        custBTSplitChild(s, 0);
        custBTInsertNonFull(s, c);
        return s;
    }
    custBTInsertNonFull(root, c);
    return root;
}

Customer *custBTSearch(CustBTNode *root, int id) {
    if (!root) return NULL;
    int i = 0;
    while (i < root->n && id > root->keys[i].account_id) i++;
    if (i < root->n && id == root->keys[i].account_id) return &root->keys[i];
    if (root->leaf) return NULL;
    return custBTSearch(root->child[i], id);
}

//Deletion helpers
static int custFindKey(CustBTNode *node, int id) {
    int i = 0;
    while (i < node->n && node->keys[i].account_id < id) i++;
    return i;
}

static Customer custGetPred(CustBTNode *node, int i) {
    CustBTNode *cur = node->child[i];
    while (!cur->leaf) cur = cur->child[cur->n];
    return cur->keys[cur->n - 1];
}

static Customer custGetSucc(CustBTNode *node, int i) {
    CustBTNode *cur = node->child[i + 1];
    while (!cur->leaf) cur = cur->child[0];
    return cur->keys[0];
}

static void custBorrowFromPrev(CustBTNode *node, int i) {
    CustBTNode *child  = node->child[i];
    CustBTNode *sibling = node->child[i - 1];

    for (int j = child->n - 1; j >= 0; j--)
        child->keys[j + 1] = child->keys[j];

    if (!child->leaf)
        for (int j = child->n; j >= 0; j--)
            child->child[j + 1] = child->child[j];

    child->keys[0] = node->keys[i - 1];
    if (!child->leaf)
        child->child[0] = sibling->child[sibling->n];

    node->keys[i - 1] = sibling->keys[sibling->n - 1];
    child->n++;
    sibling->n--;
}

static void custBorrowFromNext(CustBTNode *node, int i) {
    CustBTNode *child   = node->child[i];
    CustBTNode *sibling = node->child[i + 1];

    child->keys[child->n] = node->keys[i];
    if (!child->leaf)
        child->child[child->n + 1] = sibling->child[0];

    node->keys[i] = sibling->keys[0];

    for (int j = 1; j < sibling->n; j++)
        sibling->keys[j - 1] = sibling->keys[j];
    if (!sibling->leaf)
        for (int j = 1; j <= sibling->n; j++)
            sibling->child[j - 1] = sibling->child[j];

    child->n++;
    sibling->n--;
}

static void custMerge(CustBTNode *node, int i) {
    CustBTNode *child   = node->child[i];
    CustBTNode *sibling = node->child[i + 1];

    child->keys[BT_MIN] = node->keys[i];

    for (int j = 0; j < sibling->n; j++)
        child->keys[j + BT_MIN + 1] = sibling->keys[j];
    if (!child->leaf)
        for (int j = 0; j <= sibling->n; j++)
            child->child[j + BT_MIN + 1] = sibling->child[j];

    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    for (int j = i + 2; j <= node->n; j++)
        node->child[j - 1] = node->child[j];

    child->n += sibling->n + 1;
    node->n--;
    free(sibling);
}

static void custFill(CustBTNode *node, int i) {
    if (i != 0 && node->child[i - 1]->n >= BT_T)
        custBorrowFromPrev(node, i);
    else if (i != node->n && node->child[i + 1]->n >= BT_T)
        custBorrowFromNext(node, i);
    else {
        if (i != node->n) custMerge(node, i);
        else              custMerge(node, i - 1);
    }
}

static void custBTDeleteFromNode(CustBTNode *node, int id);

static void custBTDeleteFromLeaf(CustBTNode *node, int i) {
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    node->n--;
}

static void custBTDeleteFromInternal(CustBTNode *node, int i) {
    int id = node->keys[i].account_id;
    if (node->child[i]->n >= BT_T) {
        Customer pred = custGetPred(node, i);
        node->keys[i] = pred;
        custBTDeleteFromNode(node->child[i], pred.account_id);
    } else if (node->child[i + 1]->n >= BT_T) {
        Customer succ = custGetSucc(node, i);
        node->keys[i] = succ;
        custBTDeleteFromNode(node->child[i + 1], succ.account_id);
    } else {
        custMerge(node, i);
        custBTDeleteFromNode(node->child[i], id);
    }
}

static void custBTDeleteFromNode(CustBTNode *node, int id) {
    int i = custFindKey(node, id);
    if (i < node->n && node->keys[i].account_id == id) {
        if (node->leaf) custBTDeleteFromLeaf(node, i);
        else            custBTDeleteFromInternal(node, i);
    } else {
        if (node->leaf) return;
        int last = (i == node->n);
        if (node->child[i]->n < BT_T) custFill(node, i);
        if (last && i > node->n) custBTDeleteFromNode(node->child[i - 1], id);
        else                     custBTDeleteFromNode(node->child[i],     id);
    }
}

CustBTNode *custBTDelete(CustBTNode *root, int id) {
    if (!root) return root;
    custBTDeleteFromNode(root, id);
    if (root->n == 0) {
        CustBTNode *tmp = root;
        if (root->leaf) root = NULL;
        else            root = root->child[0];
        free(tmp);
    }
    return root;
}

void custBTInorder(CustBTNode *root) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) custBTInorder(root->child[i]);
        displayCustomer(&root->keys[i]);
    }
    if (!root->leaf) custBTInorder(root->child[i]);
}

void custBTRangeSearch(CustBTNode *root, int b1, int b2) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (root->keys[i].account_id > b1 && !root->leaf)
            custBTRangeSearch(root->child[i], b1, b2);
        if (root->keys[i].account_id >= b1 && root->keys[i].account_id <= b2)
            displayCustomer(&root->keys[i]);
    }
    if (!root->leaf && root->keys[root->n - 1].account_id < b2)
        custBTRangeSearch(root->child[root->n], b1, b2);
}

int custBTCount(CustBTNode *root) {
    if (!root) return 0;
    int count = root->n;
    if (!root->leaf)
        for (int i = 0; i <= root->n; i++)
            count += custBTCount(root->child[i]);
    return count;
}

void custBTCollect(CustBTNode *root, Customer **arr, int *cnt) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) custBTCollect(root->child[i], arr, cnt);
        if (*cnt < MAX_RECORDS) arr[(*cnt)++] = &root->keys[i];
    }
    if (!root->leaf) custBTCollect(root->child[i], arr, cnt);
}

// RIDER B-TREE key = rider_id
RiderBTNode *riderBTNewNode(int leaf) {
    RiderBTNode *n = (RiderBTNode *)calloc(1, sizeof(RiderBTNode));
    n->leaf = leaf;
    return n;
}

void riderBTSplitChild(RiderBTNode *x, int i) {
    RiderBTNode *y = x->child[i];
    RiderBTNode *z = riderBTNewNode(y->leaf);
    z->n = BT_MIN;

    for (int j = 0; j < BT_MIN; j++)
        z->keys[j] = y->keys[j + BT_T];
    if (!y->leaf)
        for (int j = 0; j <= BT_MIN; j++)
            z->child[j] = y->child[j + BT_T];
    y->n = BT_MIN;

    for (int j = x->n; j >= i + 1; j--)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[BT_T - 1];
    x->n++;
}

void riderBTInsertNonFull(RiderBTNode *x, Rider r) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && r.rider_id < x->keys[i].rider_id) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        if (i >= 0 && r.rider_id == x->keys[i].rider_id) {
            x->keys[i] = r;
            return;
        }
        x->keys[i + 1] = r;
        x->n++;
    } else {
        while (i >= 0 && r.rider_id < x->keys[i].rider_id) i--;
        if (i >= 0 && r.rider_id == x->keys[i].rider_id) {
            x->keys[i] = r;
            return;
        }
        i++;
        if (x->child[i]->n == BT_MAX) {
            riderBTSplitChild(x, i);
            if (r.rider_id > x->keys[i].rider_id) i++;
        }
        riderBTInsertNonFull(x->child[i], r);
    }
}

RiderBTNode *riderBTInsert(RiderBTNode *root, Rider r) {
    if (!root) {
        root = riderBTNewNode(1);
        root->keys[0] = r;
        root->n = 1;
        return root;
    }
    if (root->n == BT_MAX) {
        RiderBTNode *s = riderBTNewNode(0);
        s->child[0] = root;
        riderBTSplitChild(s, 0);
        riderBTInsertNonFull(s, r);
        return s;
    }
    riderBTInsertNonFull(root, r);
    return root;
}

Rider *riderBTSearch(RiderBTNode *root, int id) {
    if (!root) return NULL;
    int i = 0;
    while (i < root->n && id > root->keys[i].rider_id) i++;
    if (i < root->n && id == root->keys[i].rider_id) return &root->keys[i];
    if (root->leaf) return NULL;
    return riderBTSearch(root->child[i], id);
}

//Rider deletion
static int riderFindKey(RiderBTNode *node, int id) {
    int i = 0;
    while (i < node->n && node->keys[i].rider_id < id) i++;
    return i;
}

static Rider riderGetPred(RiderBTNode *node, int i) {
    RiderBTNode *cur = node->child[i];
    while (!cur->leaf) cur = cur->child[cur->n];
    return cur->keys[cur->n - 1];
}

static Rider riderGetSucc(RiderBTNode *node, int i) {
    RiderBTNode *cur = node->child[i + 1];
    while (!cur->leaf) cur = cur->child[0];
    return cur->keys[0];
}

static void riderBorrowFromPrev(RiderBTNode *node, int i) {
    RiderBTNode *child = node->child[i], *sib = node->child[i - 1];
    for (int j = child->n - 1; j >= 0; j--)
        child->keys[j + 1] = child->keys[j];
    if (!child->leaf)
        for (int j = child->n; j >= 0; j--)
            child->child[j + 1] = child->child[j];
    child->keys[0] = node->keys[i - 1];
    if (!child->leaf)
        child->child[0] = sib->child[sib->n];
    node->keys[i - 1] = sib->keys[sib->n - 1];
    child->n++;
    sib->n--;
}

static void riderBorrowFromNext(RiderBTNode *node, int i) {
    RiderBTNode *child = node->child[i], *sib = node->child[i + 1];
    child->keys[child->n] = node->keys[i];
    if (!child->leaf)
        child->child[child->n + 1] = sib->child[0];
    node->keys[i] = sib->keys[0];
    for (int j = 1; j < sib->n; j++)
        sib->keys[j - 1] = sib->keys[j];
    if (!sib->leaf)
        for (int j = 1; j <= sib->n; j++)
            sib->child[j - 1] = sib->child[j];
    child->n++;
    sib->n--;
}

static void riderMerge(RiderBTNode *node, int i) {
    RiderBTNode *child = node->child[i], *sib = node->child[i + 1];
    child->keys[BT_MIN] = node->keys[i];
    for (int j = 0; j < sib->n; j++)
        child->keys[j + BT_MIN + 1] = sib->keys[j];
    if (!child->leaf)
        for (int j = 0; j <= sib->n; j++)
            child->child[j + BT_MIN + 1] = sib->child[j];
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    for (int j = i + 2; j <= node->n; j++)
        node->child[j - 1] = node->child[j];
    child->n += sib->n + 1;
    node->n--;
    free(sib);
}

static void riderFill(RiderBTNode *node, int i) {
    if (i != 0 && node->child[i - 1]->n >= BT_T)
        riderBorrowFromPrev(node, i);
    else if (i != node->n && node->child[i + 1]->n >= BT_T)
        riderBorrowFromNext(node, i);
    else {
        if (i != node->n) riderMerge(node, i);
        else              riderMerge(node, i - 1);
    }
}

static void riderBTDeleteFromNode(RiderBTNode *node, int id);

static void riderBTDeleteFromLeaf(RiderBTNode *node, int i) {
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    node->n--;
}

static void riderBTDeleteFromInternal(RiderBTNode *node, int i) {
    int id = node->keys[i].rider_id;
    if (node->child[i]->n >= BT_T) {
        Rider pred = riderGetPred(node, i);
        node->keys[i] = pred;
        riderBTDeleteFromNode(node->child[i], pred.rider_id);
    } else if (node->child[i + 1]->n >= BT_T) {
        Rider succ = riderGetSucc(node, i);
        node->keys[i] = succ;
        riderBTDeleteFromNode(node->child[i + 1], succ.rider_id);
    } else {
        riderMerge(node, i);
        riderBTDeleteFromNode(node->child[i], id);
    }
}

static void riderBTDeleteFromNode(RiderBTNode *node, int id) {
    int i = riderFindKey(node, id);
    if (i < node->n && node->keys[i].rider_id == id) {
        if (node->leaf) riderBTDeleteFromLeaf(node, i);
        else            riderBTDeleteFromInternal(node, i);
    } else {
        if (node->leaf) return;
        int last = (i == node->n);
        if (node->child[i]->n < BT_T) riderFill(node, i);
        if (last && i > node->n) riderBTDeleteFromNode(node->child[i - 1], id);
        else                     riderBTDeleteFromNode(node->child[i],     id);
    }
}

RiderBTNode *riderBTDelete(RiderBTNode *root, int id) {
    if (!root) return root;
    riderBTDeleteFromNode(root, id);
    if (root->n == 0) {
        RiderBTNode *tmp = root;
        root = root->leaf ? NULL : root->child[0];
        free(tmp);
    }
    return root;
}

void riderBTInorder(RiderBTNode *root) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) riderBTInorder(root->child[i]);
        displayRider(&root->keys[i]);
    }
    if (!root->leaf) riderBTInorder(root->child[i]);
}

int riderBTCount(RiderBTNode *root) {
    if (!root) return 0;
    int count = root->n;
    if (!root->leaf)
        for (int i = 0; i <= root->n; i++)
            count += riderBTCount(root->child[i]);
    return count;
}

void riderBTCollect(RiderBTNode *root, Rider **arr, int *cnt) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) riderBTCollect(root->child[i], arr, cnt);
        if (*cnt < MAX_RECORDS) arr[(*cnt)++] = &root->keys[i];
    }
    if (!root->leaf) riderBTCollect(root->child[i], arr, cnt);
}

// STORE B-TREE   key = store_id
StoreBTNode *storeBTNewNode(int leaf) {
    StoreBTNode *n = (StoreBTNode *)calloc(1, sizeof(StoreBTNode));
    n->leaf = leaf;
    return n;
}

void storeBTSplitChild(StoreBTNode *x, int i) {
    StoreBTNode *y = x->child[i];
    StoreBTNode *z = storeBTNewNode(y->leaf);
    z->n = BT_MIN;
    for (int j = 0; j < BT_MIN; j++)
        z->keys[j] = y->keys[j + BT_T];
    if (!y->leaf)
        for (int j = 0; j <= BT_MIN; j++)
            z->child[j] = y->child[j + BT_T];
    y->n = BT_MIN;
    for (int j = x->n; j >= i + 1; j--)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[BT_T - 1];
    x->n++;
}

void storeBTInsertNonFull(StoreBTNode *x, DarkStore s) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && s.store_id < x->keys[i].store_id) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        if (i >= 0 && s.store_id == x->keys[i].store_id) {
            x->keys[i] = s;
            return;
        }
        x->keys[i + 1] = s;
        x->n++;
    } else {
        while (i >= 0 && s.store_id < x->keys[i].store_id) i--;
        if (i >= 0 && s.store_id == x->keys[i].store_id) {
            x->keys[i] = s;
            return;
        }
        i++;
        if (x->child[i]->n == BT_MAX) {
            storeBTSplitChild(x, i);
            if (s.store_id > x->keys[i].store_id) i++;
        }
        storeBTInsertNonFull(x->child[i], s);
    }
}

StoreBTNode *storeBTInsert(StoreBTNode *root, DarkStore s) {
    if (!root) {
        root = storeBTNewNode(1);
        root->keys[0] = s;
        root->n = 1;
        return root;
    }
    if (root->n == BT_MAX) {
        StoreBTNode *sr = storeBTNewNode(0);
        sr->child[0] = root;
        storeBTSplitChild(sr, 0);
        storeBTInsertNonFull(sr, s);
        return sr;
    }
    storeBTInsertNonFull(root, s);
    return root;
}

DarkStore *storeBTSearch(StoreBTNode *root, int id) {
    if (!root) return NULL;
    int i = 0;
    while (i < root->n && id > root->keys[i].store_id) i++;
    if (i < root->n && id == root->keys[i].store_id) return &root->keys[i];
    if (root->leaf) return NULL;
    return storeBTSearch(root->child[i], id);
}

DarkStore *storeBTSearchByPin(StoreBTNode *root, const char *pin) {
    if (!root) return NULL;
    for (int i = 0; i < root->n; i++) {
        if (strcmp(root->keys[i].pincode, pin) == 0) return &root->keys[i];
    }
    if (root->leaf) return NULL;
    for (int i = 0; i <= root->n; i++) {
        DarkStore *res = storeBTSearchByPin(root->child[i], pin);
        if (res) return res;
    }
    return NULL;
}

void storeBTInorder(StoreBTNode *root) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) storeBTInorder(root->child[i]);
        displayStore(&root->keys[i]);
    }
    if (!root->leaf) storeBTInorder(root->child[i]);
}

int storeBTCount(StoreBTNode *root) {
    if (!root) return 0;
    int count = root->n;
    if (!root->leaf)
        for (int i = 0; i <= root->n; i++)
            count += storeBTCount(root->child[i]);
    return count;
}

void storeBTCollect(StoreBTNode *root, DarkStore **arr, int *cnt) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) storeBTCollect(root->child[i], arr, cnt);
        if (*cnt < MAX_RECORDS) arr[(*cnt)++] = &root->keys[i];
    }
    if (!root->leaf) storeBTCollect(root->child[i], arr, cnt);
}
//INVENTORY B-TREE   key = item_id

InvBTNode *invBTNewNode(int leaf) {
    InvBTNode *n = (InvBTNode *)calloc(1, sizeof(InvBTNode));
    n->leaf = leaf;
    return n;
}

void invBTSplitChild(InvBTNode *x, int i) {
    InvBTNode *y = x->child[i];
    InvBTNode *z = invBTNewNode(y->leaf);
    z->n = BT_MIN;
    for (int j = 0; j < BT_MIN; j++)
        z->keys[j] = y->keys[j + BT_T];
    if (!y->leaf)
        for (int j = 0; j <= BT_MIN; j++)
            z->child[j] = y->child[j + BT_T];
    y->n = BT_MIN;
    for (int j = x->n; j >= i + 1; j--)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[BT_T - 1];
    x->n++;
}

void invBTInsertNonFull(InvBTNode *x, InventoryItem item) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && item.item_id < x->keys[i].item_id) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        if (i >= 0 && item.item_id == x->keys[i].item_id) {
            x->keys[i] = item;
            return;
        }
        x->keys[i + 1] = item;
        x->n++;
    } else {
        while (i >= 0 && item.item_id < x->keys[i].item_id) i--;
        if (i >= 0 && item.item_id == x->keys[i].item_id) {
            x->keys[i] = item;
            return;
        }
        i++;
        if (x->child[i]->n == BT_MAX) {
            invBTSplitChild(x, i);
            if (item.item_id > x->keys[i].item_id) i++;
        }
        invBTInsertNonFull(x->child[i], item);
    }
}

InvBTNode *invBTInsert(InvBTNode *root, InventoryItem item) {
    if (!root) {
        root = invBTNewNode(1);
        root->keys[0] = item;
        root->n = 1;
        return root;
    }
    if (root->n == BT_MAX) {
        InvBTNode *s = invBTNewNode(0);
        s->child[0] = root;
        invBTSplitChild(s, 0);
        invBTInsertNonFull(s, item);
        return s;
    }
    invBTInsertNonFull(root, item);
    return root;
}

InventoryItem *invBTSearch(InvBTNode *root, int id) {
    if (!root) return NULL;
    int i = 0;
    while (i < root->n && id > root->keys[i].item_id) i++;
    if (i < root->n && id == root->keys[i].item_id) return &root->keys[i];
    if (root->leaf) return NULL;
    return invBTSearch(root->child[i], id);
}

//Inventory deletion
static int invFindKey(InvBTNode *node, int id) {
    int i = 0;
    while (i < node->n && node->keys[i].item_id < id) i++;
    return i;
}

static InventoryItem invGetPred(InvBTNode *node, int i) {
    InvBTNode *cur = node->child[i];
    while (!cur->leaf) cur = cur->child[cur->n];
    return cur->keys[cur->n - 1];
}

static InventoryItem invGetSucc(InvBTNode *node, int i) {
    InvBTNode *cur = node->child[i + 1];
    while (!cur->leaf) cur = cur->child[0];
    return cur->keys[0];
}

static void invBorrowFromPrev(InvBTNode *node, int i) {
    InvBTNode *child = node->child[i], *sib = node->child[i - 1];
    for (int j = child->n - 1; j >= 0; j--)
        child->keys[j + 1] = child->keys[j];
    if (!child->leaf)
        for (int j = child->n; j >= 0; j--)
            child->child[j + 1] = child->child[j];
    child->keys[0] = node->keys[i - 1];
    if (!child->leaf)
        child->child[0] = sib->child[sib->n];
    node->keys[i - 1] = sib->keys[sib->n - 1];
    child->n++;
    sib->n--;
}

static void invBorrowFromNext(InvBTNode *node, int i) {
    InvBTNode *child = node->child[i], *sib = node->child[i + 1];
    child->keys[child->n] = node->keys[i];
    if (!child->leaf)
        child->child[child->n + 1] = sib->child[0];
    node->keys[i] = sib->keys[0];
    for (int j = 1; j < sib->n; j++)
        sib->keys[j - 1] = sib->keys[j];
    if (!sib->leaf)
        for (int j = 1; j <= sib->n; j++)
            sib->child[j - 1] = sib->child[j];
    child->n++;
    sib->n--;
}

static void invMerge(InvBTNode *node, int i) {
    InvBTNode *child = node->child[i], *sib = node->child[i + 1];
    child->keys[BT_MIN] = node->keys[i];
    for (int j = 0; j < sib->n; j++)
        child->keys[j + BT_MIN + 1] = sib->keys[j];
    if (!child->leaf)
        for (int j = 0; j <= sib->n; j++)
            child->child[j + BT_MIN + 1] = sib->child[j];
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    for (int j = i + 2; j <= node->n; j++)
        node->child[j - 1] = node->child[j];
    child->n += sib->n + 1;
    node->n--;
    free(sib);
}

static void invFill(InvBTNode *node, int i) {
    if (i != 0 && node->child[i - 1]->n >= BT_T)
        invBorrowFromPrev(node, i);
    else if (i != node->n && node->child[i + 1]->n >= BT_T)
        invBorrowFromNext(node, i);
    else {
        if (i != node->n) invMerge(node, i);
        else              invMerge(node, i - 1);
    }
}

static void invBTDeleteFromNode(InvBTNode *node, int id);

static void invBTDeleteFromLeaf(InvBTNode *node, int i) {
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    node->n--;
}

static void invBTDeleteFromInternal(InvBTNode *node, int i) {
    int id = node->keys[i].item_id;
    if (node->child[i]->n >= BT_T) {
        InventoryItem pred = invGetPred(node, i);
        node->keys[i] = pred;
        invBTDeleteFromNode(node->child[i], pred.item_id);
    } else if (node->child[i + 1]->n >= BT_T) {
        InventoryItem succ = invGetSucc(node, i);
        node->keys[i] = succ;
        invBTDeleteFromNode(node->child[i + 1], succ.item_id);
    } else {
        invMerge(node, i);
        invBTDeleteFromNode(node->child[i], id);
    }
}

static void invBTDeleteFromNode(InvBTNode *node, int id) {
    int i = invFindKey(node, id);
    if (i < node->n && node->keys[i].item_id == id) {
        if (node->leaf) invBTDeleteFromLeaf(node, i);
        else            invBTDeleteFromInternal(node, i);
    } else {
        if (node->leaf) return;
        int last = (i == node->n);
        if (node->child[i]->n < BT_T) invFill(node, i);
        if (last && i > node->n) invBTDeleteFromNode(node->child[i - 1], id);
        else                     invBTDeleteFromNode(node->child[i],     id);
    }
}

InvBTNode *invBTDelete(InvBTNode *root, int id) {
    if (!root) return root;
    invBTDeleteFromNode(root, id);
    if (root->n == 0) {
        InvBTNode *tmp = root;
        root = root->leaf ? NULL : root->child[0];
        free(tmp);
    }
    return root;
}

void invBTInorder(InvBTNode *root) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) invBTInorder(root->child[i]);
        printf("  Item ID: %d | Name: %-20s | Qty: %d | Price: Rs.%.2f\n",
               root->keys[i].item_id, root->keys[i].item_name,
               root->keys[i].quantity, root->keys[i].price);
    }
    if (!root->leaf) invBTInorder(root->child[i]);
}

int invBTCount(InvBTNode *root) {
    if (!root) return 0;
    int count = root->n;
    if (!root->leaf)
        for (int i = 0; i <= root->n; i++)
            count += invBTCount(root->child[i]);
    return count;
}

void invBTCollect(InvBTNode *root, InventoryItem **arr, int *cnt) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) invBTCollect(root->child[i], arr, cnt);
        if (*cnt < MAX_RECORDS) arr[(*cnt)++] = &root->keys[i];
    }
    if (!root->leaf) invBTCollect(root->child[i], arr, cnt);
}

//   ORDER B-TREE   (key = order_id)

OrdBTNode *ordBTNewNode(int leaf) {
    OrdBTNode *n = (OrdBTNode *)calloc(1, sizeof(OrdBTNode));
    n->leaf = leaf;
    return n;
}

void ordBTSplitChild(OrdBTNode *x, int i) {
    OrdBTNode *y = x->child[i];
    OrdBTNode *z = ordBTNewNode(y->leaf);
    z->n = BT_MIN;
    for (int j = 0; j < BT_MIN; j++)
        z->keys[j] = y->keys[j + BT_T];
    if (!y->leaf)
        for (int j = 0; j <= BT_MIN; j++)
            z->child[j] = y->child[j + BT_T];
    y->n = BT_MIN;
    for (int j = x->n; j >= i + 1; j--)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[BT_T - 1];
    x->n++;
}

void ordBTInsertNonFull(OrdBTNode *x, Order o) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && o.order_id < x->keys[i].order_id) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        if (i >= 0 && o.order_id == x->keys[i].order_id) {
            x->keys[i] = o;
            return;
        }
        x->keys[i + 1] = o;
        x->n++;
    } else {
        while (i >= 0 && o.order_id < x->keys[i].order_id) i--;
        if (i >= 0 && o.order_id == x->keys[i].order_id) {
            x->keys[i] = o;
            return;
        }
        i++;
        if (x->child[i]->n == BT_MAX) {
            ordBTSplitChild(x, i);
            if (o.order_id > x->keys[i].order_id) i++;
        }
        ordBTInsertNonFull(x->child[i], o);
    }
}

OrdBTNode *ordBTInsert(OrdBTNode *root, Order o) {
    if (!root) {
        root = ordBTNewNode(1);
        root->keys[0] = o;
        root->n = 1;
        return root;
    }
    if (root->n == BT_MAX) {
        OrdBTNode *s = ordBTNewNode(0);
        s->child[0] = root;
        ordBTSplitChild(s, 0);
        ordBTInsertNonFull(s, o);
        return s;
    }
    ordBTInsertNonFull(root, o);
    return root;
}

Order *ordBTSearch(OrdBTNode *root, int id) {
    if (!root) return NULL;
    int i = 0;
    while (i < root->n && id > root->keys[i].order_id) i++;
    if (i < root->n && id == root->keys[i].order_id) return &root->keys[i];
    if (root->leaf) return NULL;
    return ordBTSearch(root->child[i], id);
}

//Order deletion
static int ordFindKey(OrdBTNode *node, int id) {
    int i = 0;
    while (i < node->n && node->keys[i].order_id < id) i++;
    return i;
}

static Order ordGetPred(OrdBTNode *node, int i) {
    OrdBTNode *cur = node->child[i];
    while (!cur->leaf) cur = cur->child[cur->n];
    return cur->keys[cur->n - 1];
}

static Order ordGetSucc(OrdBTNode *node, int i) {
    OrdBTNode *cur = node->child[i + 1];
    while (!cur->leaf) cur = cur->child[0];
    return cur->keys[0];
}

static void ordBorrowFromPrev(OrdBTNode *node, int i) {
    OrdBTNode *child = node->child[i], *sib = node->child[i - 1];
    for (int j = child->n - 1; j >= 0; j--)
        child->keys[j + 1] = child->keys[j];
    if (!child->leaf)
        for (int j = child->n; j >= 0; j--)
            child->child[j + 1] = child->child[j];
    child->keys[0] = node->keys[i - 1];
    if (!child->leaf)
        child->child[0] = sib->child[sib->n];
    node->keys[i - 1] = sib->keys[sib->n - 1];
    child->n++;
    sib->n--;
}

static void ordBorrowFromNext(OrdBTNode *node, int i) {
    OrdBTNode *child = node->child[i], *sib = node->child[i + 1];
    child->keys[child->n] = node->keys[i];
    if (!child->leaf)
        child->child[child->n + 1] = sib->child[0];
    node->keys[i] = sib->keys[0];
    for (int j = 1; j < sib->n; j++)
        sib->keys[j - 1] = sib->keys[j];
    if (!sib->leaf)
        for (int j = 1; j <= sib->n; j++)
            sib->child[j - 1] = sib->child[j];
    child->n++;
    sib->n--;
}

static void ordMerge(OrdBTNode *node, int i) {
    OrdBTNode *child = node->child[i], *sib = node->child[i + 1];
    child->keys[BT_MIN] = node->keys[i];
    for (int j = 0; j < sib->n; j++)
        child->keys[j + BT_MIN + 1] = sib->keys[j];
    if (!child->leaf)
        for (int j = 0; j <= sib->n; j++)
            child->child[j + BT_MIN + 1] = sib->child[j];
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    for (int j = i + 2; j <= node->n; j++)
        node->child[j - 1] = node->child[j];
    child->n += sib->n + 1;
    node->n--;
    free(sib);
}

static void ordFill(OrdBTNode *node, int i) {
    if (i != 0 && node->child[i - 1]->n >= BT_T)
        ordBorrowFromPrev(node, i);
    else if (i != node->n && node->child[i + 1]->n >= BT_T)
        ordBorrowFromNext(node, i);
    else {
        if (i != node->n) ordMerge(node, i);
        else              ordMerge(node, i - 1);
    }
}

static void ordBTDeleteFromNode(OrdBTNode *node, int id);

static void ordBTDeleteFromLeaf(OrdBTNode *node, int i) {
    for (int j = i + 1; j < node->n; j++)
        node->keys[j - 1] = node->keys[j];
    node->n--;
}

static void ordBTDeleteFromInternal(OrdBTNode *node, int i) {
    int id = node->keys[i].order_id;
    if (node->child[i]->n >= BT_T) {
        Order pred = ordGetPred(node, i);
        node->keys[i] = pred;
        ordBTDeleteFromNode(node->child[i], pred.order_id);
    } else if (node->child[i + 1]->n >= BT_T) {
        Order succ = ordGetSucc(node, i);
        node->keys[i] = succ;
        ordBTDeleteFromNode(node->child[i + 1], succ.order_id);
    } else {
        ordMerge(node, i);
        ordBTDeleteFromNode(node->child[i], id);
    }
}

static void ordBTDeleteFromNode(OrdBTNode *node, int id) {
    int i = ordFindKey(node, id);
    if (i < node->n && node->keys[i].order_id == id) {
        if (node->leaf) ordBTDeleteFromLeaf(node, i);
        else            ordBTDeleteFromInternal(node, i);
    } else {
        if (node->leaf) return;
        int last = (i == node->n);
        if (node->child[i]->n < BT_T) ordFill(node, i);
        if (last && i > node->n) ordBTDeleteFromNode(node->child[i - 1], id);
        else                     ordBTDeleteFromNode(node->child[i],     id);
    }
}

OrdBTNode *ordBTDelete(OrdBTNode *root, int id) {
    if (!root) return root;
    ordBTDeleteFromNode(root, id);
    if (root->n == 0) {
        OrdBTNode *tmp = root;
        root = root->leaf ? NULL : root->child[0];
        free(tmp);
    }
    return root;
}

void ordBTInorder(OrdBTNode *root) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) ordBTInorder(root->child[i]);
        printf("  Order#%d | Item:%s x%d | Rs.%.2f | %s\n",
               root->keys[i].order_id, root->keys[i].item_name,
               root->keys[i].quantity, root->keys[i].amount,
               root->keys[i].is_completed ? "Completed" : "Pending");
    }
    if (!root->leaf) ordBTInorder(root->child[i]);
}

int ordBTCount(OrdBTNode *root) {
    if (!root) return 0;
    int count = root->n;
    if (!root->leaf)
        for (int i = 0; i <= root->n; i++)
            count += ordBTCount(root->child[i]);
    return count;
}

void ordBTCollect(OrdBTNode *root, Order **arr, int *cnt) {
    if (!root) return;
    int i;
    for (i = 0; i < root->n; i++) {
        if (!root->leaf) ordBTCollect(root->child[i], arr, cnt);
        if (*cnt < MAX_RECORDS) arr[(*cnt)++] = &root->keys[i];
    }
    if (!root->leaf) ordBTCollect(root->child[i], arr, cnt);
}