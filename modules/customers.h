#ifndef CUSTOMERS_H
#define CUSTOMERS_H

#include <gtk/gtk.h>

// Struct để truyền vào hàm addCustomers
typedef struct 
{
    GtkWidget *main_window;
    GtkListStore *store; // Liststore của customerList
    GtkListStore *serviceList;
    GtkListStore *billingList;

} CustomerData;

// Struct để truyền vào hàm on_save_clicked
typedef struct {
    GtkWidget *id_entry;
    GtkWidget *name_entry;
    GtkWidget *phone_entry;
    GtkWidget *plate_entry;
    GtkWidget *type_entry;
    GtkListStore *store;
} AddCustomerData;

typedef struct 
{
    GtkListStore *list_store;
    gint search_column; 
    GtkTreeIter *result_iter;
    GtkWidget *grid;
    // Các entry cần cho chức năng chỉnh sửa khách hàng
    GtkWidget *id_entry;
    GtkWidget *name_entry;
    GtkWidget *numberphone_entry;
    GtkWidget *numberplate_entry;
    GtkWidget *cartype_entry;
    // Các label cần cho việc hiện thông tin trong các cửa sổ con
    GtkWidget *id_label;       
    GtkWidget *name_label;
    GtkWidget *numberphone_label;
    GtkWidget *numberplate_label;
    GtkWidget *cartype_label;
} FindIterOfSearch;

typedef struct {
    GtkListStore *billingList;
    GtkListStore *serviceList;
    GtkGrid *grid_history;
    GtkWidget *entry;
} HistoryData;


void addCustomers(GtkWidget *widget, gpointer user_data);
static void addData(GtkListStore *store, const char *filename, ...);
void deleteCustomers(GtkWidget *widget, gpointer user_data);
void editCustomers(GtkWidget *widget, gpointer user_data);
void historyCustomers(GtkWidget *widget, gpointer user_data);

#endif
