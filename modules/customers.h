#ifndef CUSTOMERS_H
#define CUSTOMERS_H

#include <gtk/gtk.h>

// Struct để truyền vào hàm addCustomers
typedef struct 
{
    GtkWidget *main_window;
    GtkListStore *store;
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
} FindIterOfSearch;

void addCustomers(GtkWidget *widget, gpointer user_data);
static void addData(GtkListStore *store, const char *filename, ...);
void deleteCustomers(GtkWidget *widget, gpointer user_data);
void editCustomers(GtkWidget *widget, gpointer user_data);
void historyCustomers(GtkWidget *widget, gpointer user_data);

#endif
