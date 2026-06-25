#include "blinkit.h"

extern CustomerCache custCache;
extern RiderCache riderCache;
//MERGE SORT IMPLEMENTATIONS
  
static void mergeCustExpense(Customer **arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    Customer **L = (Customer **)malloc(n1 * sizeof(Customer *));
    Customer **R = (Customer **)malloc(n2 * sizeof(Customer *));
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i]->total_spent >= R[j]->total_spent)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void mergeSortCustExpense(Customer **arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortCustExpense(arr, l, m);
        mergeSortCustExpense(arr, m + 1, r);
        mergeCustExpense(arr, l, m, r);
    }
}

static void mergeCustPincode(Customer **arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    Customer **L = (Customer **)malloc(n1 * sizeof(Customer *));
    Customer **R = (Customer **)malloc(n2 * sizeof(Customer *));
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (strcmp(L[i]->pincode, R[j]->pincode) <= 0)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void mergeSortCustPincode(Customer **arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortCustPincode(arr, l, m);
        mergeSortCustPincode(arr, m + 1, r);
        mergeCustPincode(arr, l, m, r);
    }
}

static float rider3MonthEarning(Rider *r) {
    return r->monthly_income[0] + r->monthly_income[1] + r->monthly_income[2];
}

static void mergeRider3Month(Rider **arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    Rider **L = (Rider **)malloc(n1 * sizeof(Rider *));
    Rider **R = (Rider **)malloc(n2 * sizeof(Rider *));
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (rider3MonthEarning(L[i]) >= rider3MonthEarning(R[j]))
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void mergeSortRider3Month(Rider **arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortRider3Month(arr, l, m);
        mergeSortRider3Month(arr, m + 1, r);
        mergeRider3Month(arr, l, m, r);
    }
}

static void mergeRiderOrders(Rider **arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    Rider **L = (Rider **)malloc(n1 * sizeof(Rider *));
    Rider **R = (Rider **)malloc(n2 * sizeof(Rider *));
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i]->orders_this_month >= R[j]->orders_this_month)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void mergeSortRiderOrders(Rider **arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortRiderOrders(arr, l, m);
        mergeSortRiderOrders(arr, m + 1, r);
        mergeRiderOrders(arr, l, m, r);
    }
}

static void mergeRiderPinEarning(Rider **arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    Rider **L = (Rider **)malloc(n1 * sizeof(Rider *));
    Rider **R = (Rider **)malloc(n2 * sizeof(Rider *));
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        int cmp = strcmp(L[i]->pincode, R[j]->pincode);
        if (cmp < 0)
            arr[k++] = L[i++];
        else if (cmp > 0)
            arr[k++] = R[j++];
        else {
            if (rider3MonthEarning(L[i]) >= rider3MonthEarning(R[j]))
                arr[k++] = L[i++];
            else
                arr[k++] = R[j++];
        }
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void mergeSortRiderPinEarning(Rider **arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortRiderPinEarning(arr, l, m);
        mergeSortRiderPinEarning(arr, m + 1, r);
        mergeRiderPinEarning(arr, l, m, r);
    }
}

//   Collect all records, sort using merge sort, and populate cache
void rebuildCustExpenseCache(void) {
    Customer *all[MAX_RECORDS];
    int cnt = 0;
    custBTCollect(customerDB, all, &cnt);
    if (cnt > 0)
        mergeSortCustExpense(all, 0, cnt - 1);
    for (int i = 0; i < cnt; i++)
        custCache.byExpense[i] = all[i];
    custCache.byExpenseCount = cnt;
    custCache.byExpenseDirty = 0;
}

void rebuildCustPincodeCache(void) {
    Customer *all[MAX_RECORDS];
    int cnt = 0;
    custBTCollect(customerDB, all, &cnt);
    if (cnt > 0)
        mergeSortCustPincode(all, 0, cnt - 1);
    for (int i = 0; i < cnt; i++)
        custCache.byPincode[i] = all[i];
    custCache.byPincodeCount = cnt;
    custCache.byPincodeDirty = 0;
}

void rebuildRider3MonthCache(void) {
    Rider *all[MAX_RECORDS];
    int cnt = 0;
    riderBTCollect(riderDB, all, &cnt);
    if (cnt > 0)
        mergeSortRider3Month(all, 0, cnt - 1);
    for (int i = 0; i < cnt; i++)
        riderCache.by3Month[i] = all[i];
    riderCache.by3MonthCount = cnt;
    riderCache.by3MonthDirty = 0;
}

void rebuildRiderOrdersCache(void) {
    Rider *all[MAX_RECORDS];
    int cnt = 0;
    riderBTCollect(riderDB, all, &cnt);
    if (cnt > 0)
        mergeSortRiderOrders(all, 0, cnt - 1);
    for (int i = 0; i < cnt; i++)
        riderCache.byOrders[i] = all[i];
    riderCache.byOrdersCount = cnt;
    riderCache.byOrdersDirty = 0;
}

void rebuildRiderPinEarnCache(void) {
    Rider *all[MAX_RECORDS];
    int cnt = 0;
    riderBTCollect(riderDB, all, &cnt);
    if (cnt > 0)
        mergeSortRiderPinEarning(all, 0, cnt - 1);
    for (int i = 0; i < cnt; i++)
        riderCache.byPinEarning[i] = all[i];
    riderCache.byPinEarningCount = cnt;
    riderCache.byPinEarningDirty = 0;
}


void collectCustomers(CustBTNode *root, Customer **arr, int *cnt) {
    custBTCollect(root, arr, cnt);
}

void collectRiders(RiderBTNode *root, Rider **arr, int *cnt) {
    riderBTCollect(root, arr, cnt);
}