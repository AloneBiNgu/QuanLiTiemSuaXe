#ifndef SERVICES_H
#define SERVICES_H

#include <gtk/gtk.h>

// Struct để truyền vào hàm addCustomers
typedef struct 
{
    GtkWidget *main_window;
    GtkListStore *store;
    GtkListStore *customerList;
    GtkListStore *billingList;
} ServiceData;

// Struct để truyền vào hàm on_save_clicked
typedef struct {
    GtkWidget *id_entry;
    GtkWidget *name_entry;
    GtkWidget *cost_entry;
    GtkListStore *store;
} AddServiceData;

typedef struct 
{
    GtkListStore *list_store;
    gint search_column; 
    GtkTreeIter *result_iter;
    GtkWidget *grid;
    // Các entry cần cho chức năng chỉnh sửa khách hàng
    GtkWidget *id_entry;
    GtkWidget *name_entry;
    GtkWidget *cost_entry;
    // Các label cần cho việc hiện thông tin trong các cửa sổ con
    GtkWidget *id_label;       
    GtkWidget *name_label;
    GtkWidget *cost_label;
} FindIterOfSearch_service;

void addServices(GtkWidget *widget, gpointer user_data);
static void addData(GtkListStore *store, const char *filename, ...);
void deleteServices(GtkWidget *widget, gpointer user_data);
void editServices(GtkWidget *widget, gpointer user_data);

#endif
