#include "blinkit.h"

void printMainMenu() {
    printf("\n");
    printf("        BLINKIT MANAGEMENT SYSTEM        \n");
    printf("\n");
    printf("  --- Customer Operations ---             \n");
    printf("   1. Register Customer                   \n");
    printf("   2. Update Customer                     \n");
    printf("   3. Delete Customer                     \n");
    printf("   4. Display Customers by Expense        \n");
    printf("   5. Display Customers by PinCode        \n");
    printf("  --- Rider Operations ---                \n");
    printf("   6. Register Delivery Rider             \n");
    printf("   7. Delete Delivery Rider               \n");
    printf("   8. Display Riders by 3-Month Earning   \n");
    printf("   9. Display Riders by Orders (1 Month)  \n");
    printf("  10. Display Riders by PinCode+Earning   \n");
    printf("  --- Order Operations ---                \n");
    printf("  11. Place Order                         \n");
    printf("  12. Deliver Order                       \n");
    printf("  13. Range Search on Customers           \n");
    printf("  --- Dark Store Management ---           \n");
    printf("  14. Add Dark Store                      \n");
    printf("  15. Display All Dark Stores             \n");
    printf("  16. Add Inventory Item to Store         \n");
    printf("  --- Distance Table ---                  \n");
    printf("  17. Add Distance Between PinCodes       \n");
    printf("  --- Data Persistence ---                \n");
    printf("  18. Save All Data to Files              \n");
    printf("  19. Load All Data from Files            \n");
    printf("   0. Exit                                \n");
    printf("\n");
    printf("Enter choice: ");
}

int main() {
    distInit();
    printf("Loading saved data (if any)...\n");
    loadAllData();

    int choice;
    do {
        printMainMenu();
        if (scanf("%d", &choice) != 1) 
            { 
                while (getchar() != '\n'); 
                continue; 
            }

        switch (choice) {

    case 1:
        registerCustomer();
        saveAllData();
        break;

    case 2:
        updateCustomer();
        saveAllData();
        break;

    case 3:
        deleteCustomer();
        saveAllData();
        break;

    case 4:
        displayCustomersByExpense();
        break;

    case 5:
        displayCustomersByPincode();
        break;

    case 6:
        registerRider();
        saveAllData();
        break;

    case 7:
        deleteRider();
        saveAllData();
        break;

    case 8:
        displayRidersByEarning3Months();
        break;

    case 9:
        displayRidersByOrdersLastMonth();
        break;

    case 10:
        displayRidersByPincodeEarning();
        break;

    case 11:
        placeOrder();
        saveAllData();
        break;

    case 12:
        deliverOrder();
        saveAllData();
        break;

    case 13:
        rangeSearchCustomers();
        break;

    case 14:
        addDarkStore();
        saveAllData();
        break;

    case 15:
        displayDarkStores();
        break;

    case 16:
        addInventoryItem();
        saveAllData();
        break;

    case 17:
        addDistance();
        saveAllData();
        break;

    case 18:
        saveAllData();
        printf("All data saved successfully.\n");
        break;

    case 19:
        loadAllData();
        printf("All data loaded successfully.\n");
        break;

    case 0:
        printf("Saving data before exit...\n");
        saveAllData();
        printf("Goodbye!\n");
        break;

    default:
        printf("Invalid choice. Try again.\n");
        }
    } while (choice != 0);

    return 0;
}